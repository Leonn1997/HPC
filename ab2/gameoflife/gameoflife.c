#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>

#define calcIndex(width, x,y)  ((y)*(width) + (x))

long TimeSteps = 100;

void writeVTK2(long timestep, double *data, char prefix[1024], long w, long h) {
  char filename[2048];  
  int x,y; 
  
  long offsetX=0;
  long offsetY=0;
  float deltax=1.0;
  float deltay=1.0;
  long  nxy = w * h * sizeof(float);  

  snprintf(filename, sizeof(filename), "%s-%05ld%s", prefix, timestep, ".vti");
  FILE* fp = fopen(filename, "w");

  fprintf(fp, "<?xml version=\"1.0\"?>\n");
  fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
  fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", offsetX, offsetX + w, offsetY, offsetY + h, 0, 0, deltax, deltax, 0.0);
  fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
  fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
  fprintf(fp, "</CellData>\n");
  fprintf(fp, "</ImageData>\n");
  fprintf(fp, "<AppendedData encoding=\"raw\">\n");
  fprintf(fp, "_");
  fwrite((unsigned char*)&nxy, sizeof(long), 1, fp);

  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      float value = data[calcIndex(h, x,y)];
      fwrite((unsigned char*)&value, sizeof(float), 1, fp);
    }
  }
  
  fprintf(fp, "\n</AppendedData>\n");
  fprintf(fp, "</VTKFile>\n");
  fclose(fp);
}

void writeVTK2Piece(long timestep, double *data, char prefix[1024],int xStart, int xEnd, int yStart, int yEnd, long w, long h, int thread_num) {
char filename[2048];
int x,y;

long offsetX=0;
long offsetY=0;
float deltax=1.0;
float deltay=1.0;
long nxy = w * h * sizeof(float);

snprintf(filename, sizeof(filename), "%s-%05ld-%02d%s", prefix, timestep, thread_num, ".vti");
FILE* fp = fopen(filename, "w");

fprintf(fp, "<?xml version=\"1.0\"?>\n");
fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d 0 0\" Origin=\"0 0 0\" Spacing=\"1 1 0\">\n",
offsetX, offsetX + w, offsetY, offsetY + h);
fprintf(fp, "<Piece Extent=\"%d %d %d %d 0 0\">\n", xStart, xEnd + 1, yStart, yEnd + 1);
fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
fprintf(fp, "</CellData>\n");
fprintf(fp, "</Piece>\n");
fprintf(fp, "</ImageData>\n");
fprintf(fp, "<AppendedData encoding=\"raw\">\n");
fprintf(fp, "_");
fwrite((unsigned char*)&nxy, sizeof(long), 1, fp);

for (y = yStart; y <= yEnd; y++) {
for (x = xStart; x <= xEnd; x++) {
float value = data[calcIndex(w,x,y)];
fwrite((unsigned char*)&value, sizeof(float), 1, fp);
}
}

fprintf(fp, "\n</AppendedData>\n");
fprintf(fp, "</VTKFile>\n");
fclose(fp);
}

void writeVTK2Container(long timestep, double *data, char prefix[1024], long w, long h, int *areaBounds, int num_threads) {
char filename[2048];
int x,y;

long offsetX=0;
long offsetY=0;
float deltax=1.0;
float deltay=1.0;
long nxy = w * h * sizeof(float);

snprintf(filename, sizeof(filename), "%s-%05ld%s", prefix, timestep, ".pvti");
FILE* fp = fopen(filename, "w");

fprintf(fp,"<?xml version=\"1.0\"?>\n");
fprintf(fp,"<VTKFile type=\"PImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
fprintf(fp,"<PImageData WholeExtent=\"%d %d %d %d 0 0\" Origin=\"0 0 0\" Spacing =\"1 1 0\" GhostLevel=\"0\">\n", offsetX, offsetX + w, offsetY, offsetY + h);
fprintf(fp,"<PCellData Scalars=\"%s\">\n", prefix);
fprintf(fp,"<PDataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
fprintf(fp,"</PCellData>\n");

for(int i = 0; i < num_threads; i++) {
fprintf(fp, "<Piece Extent=\"%d %d %d %d 0 0\" Source=\"%s-%05ld-%02d%s\"/>\n",
areaBounds[i * 4], areaBounds[i * 4 + 1] + 1, areaBounds[i * 4 + 2], areaBounds[i * 4 + 3] + 1, prefix, timestep, i, ".vti");
}

fprintf(fp,"</PImageData>\n");
fprintf(fp, "</VTKFile>\n");
fclose(fp);
}


void show(double* currentfield, int w, int h) {
  printf("\033[H");
  int x,y;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) printf(currentfield[calcIndex(w, x,y)] ? "\033[07m  \033[m" : "  ");
    //printf("\033[E");
    printf("\n");
  }
  fflush(stdout);
}

int countNeighbours(double* currentfield, int x, int y, int width, int height) {
  int counter = 0;

  // iterate over the 9 neighbouring fields including the current field itself
  for (int neighbourX = (x-1); neighbourX <= (x+1); neighbourX++) {
    for (int neighbourY = (y-1); neighbourY <= (y+1); neighbourY++) {
      // modulo for periodic borders: width = 3; x+1 = 3 (index) => currentfield[(3 + 3) % 3] = 0;
      counter += currentfield[calcIndex(width, (neighbourX + width) % width, (neighbourY + height) % height)];
    }
  }

  counter -= currentfield[calcIndex(width, x, y)];

  return counter;
} 
 
void evolve(double* currentfield, double* newfield, int w, int h, int x1, int x2, int y1, int y2) {
  int x,y;
  for (y = y1; y < y2; y++) {
    for (x = x1; x < x2; x++) {
      int neighbours = countNeighbours(currentfield, x, y, w, h);
      int index = calcIndex(w, x, y);


        if (neighbours < 2) {
          newfield[index] = 0;
        }
        else if (neighbours == 2 ) {
          newfield[index] = currentfield[index];
        }
        else if(neighbours == 3) {
          newfield[index] = 1;
        }
        else if (neighbours > 3) {
          newfield[index] = 0;
        }
      
    
    }
  }
}
 
void filling(double* currentfield, int w, int h) {
  int i;
  for (i = 0; i < h*w; i++) {
    currentfield[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
  }
}
 
void game(int w, int h) {
  double *currentfield = calloc(w*h, sizeof(double));
  double *newfield     = calloc(w*h, sizeof(double));
  
  int numberOfAreas = 2; //LUL
  int x1, x2, y1, y2;

  int *areaBounds = calloc(numberOfAreas * 4, sizeof(int));
  int areaWidth = (w/numberOfAreas) + (w % numberOfAreas > 0 ? 1 : 0);
  int areaHeight = (h/numberOfAreas) + (h % numberOfAreas > 0 ? 1 : 0);

  filling(currentfield, w, h);
  long t;
  for (t=0;t<TimeSteps;t++) {
    show(currentfield, w, h);

    #pragma omp parallel private(x1, x2, y1, y2) firstprivate(areaHeight, areaWidth, w, h, numberOfAreas) num_threads(numberOfAreas) 
    {

    // 0 or 1
    int threadId = omp_get_thread_num();

    x1 = areaWidth * (threadId % numberOfAreas);
    x2 = areaWidth * ((threadId % numberOfAreas) +1) -1;

    y1 = areaHeight * (threadId % numberOfAreas);
    y2 = areaHeight * ((threadId % numberOfAreas) +1) -1;

    evolve(currentfield, newfield, w, h, x1, x2, y1, y2);
    writeVTK2Piece(t, currentfield, "gol", x1, x2, y1, y2, w, h, threadId);

    areaBounds[threadId * 4 + 0] = x1;
    areaBounds[threadId * 4 + 1] = x2;

    areaBounds[threadId * 4 + 2] = y1;
    areaBounds[threadId * 4 + 3] = y2;

    }

    writeVTK2Container(t, currentfield, "gol", w, h, areaBounds, numberOfAreas);
    printf("%ld timestep\n",t);
    usleep(200000);

    //SWAP
    double *temp = currentfield;
    currentfield = newfield;
    newfield = temp;
  }
  
  free(currentfield);
  free(newfield);
  free(areaBounds);
  
}
 
int main(int c, char **v) {
  int w = 0, h = 0;
  if (c > 1) w = atoi(v[1]); ///< read width
  if (c > 2) h = atoi(v[2]); ///< read height
  if (w <= 0) w = 30; ///< default width
  if (h <= 0) h = 30; ///< default height
  game(w, h);
}
