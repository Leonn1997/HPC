#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>

#define calcIndex(width, x,y)  ((y)*(width) + (x))

long TimeSteps = 40;

void filling(double* currentfield, int w, int h) {
  int i;
  for (i = 0; i < h*w; i++) {
    currentfield[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
  }
}

void writeVTK2Piece(long timestep, double *data, char prefix[1024],int xStart, int xEnd, int yStart, int yEnd, long w, long h, int thread_num) {
  char filename[2048];  
  int x,y; 
  
  long offsetX=0;
  long offsetY=0;
  float deltax=1.0;
  float deltay=1.0;
  long  nxy = w * h * sizeof(float);  

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

void writeVTK2Container(long timestep, double *data, char prefix[1024], long w, long h, int *areaVertices, int num_threads) {
  char filename[2048];  
  int x,y; 
  
  long offsetX=0;
  long offsetY=0;
  float deltax=1.0;
  float deltay=1.0;
  long  nxy = w * h * sizeof(float);  

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
      areaVertices[i * 4], areaVertices[i * 4 + 1] + 1, areaVertices[i * 4 + 2], areaVertices[i * 4 + 3] + 1, prefix, timestep, i, ".vti");
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
    printf("\n");
  }
  fflush(stdout);
}
 
 
void evolve(double* currentfield, double* newfield, int startX, int endX, int startY, int endY, int w, int h) {
  int x,y;
  for (y = startY; y <= endY; y++) {
    for (x = startX; x <= endX; x++) {
      
      int n = countNeighbours(currentfield, x, y, w, h);
      int index = calcIndex(w, x, y);

      // dead or alive and 3 neighbours => come alive or stay alive
      if (n == 3) {
        newfield[index] = 1;
      }
      // dead or alive and 2 neighbours => stay dead or alive
      else if (n == 2) {
        newfield[index] = currentfield[index];
      }
      // less than 2 or more than 3 neighbours => DIE (even if you're already dead)!
      else {
        newfield[index] = 0;
      }
    }
  }
}

int countNeighbours(double* currentfield, int x, int y, int width, int height) {
  int n = 0;
  for (int neighbourX = (x-1); neighbourX <= (x+1); neighbourX++) {
    for (int neighbourY = (y-1); neighbourY <= (y+1); neighbourY++) {
      n += currentfield[calcIndex(width, (neighbourX + width) % width, (neighbourY + height) % height)];
    }
  }

  n -= currentfield[calcIndex(width, x, y)];

  return n;
}

void game(int w, int h) {
  double *currentfield = calloc(w*h, sizeof(double));
  double *newfield     = calloc(w*h, sizeof(double));
  filling(currentfield, w, h);

  long timestep;
  int startX, startY, endX, endY;
  int xSections = 16;
  int ySections = 16;
  int numberOfAreas = xSections * ySections;
  int *areaVertices = calloc(numberOfAreas * 4, sizeof(int));
  int areaWidth = (w / xSections) + (w % xSections > 0 ? 1 : 0);
  int areaHeight = (h / ySections) + (h % ySections > 0 ? 1 : 0);

  for (timestep=0;t<TimeSteps;t++) {
    show(currentfield, w, h);
        
    #pragma omp parallel private(startX, startY, endX, endY) firstprivate(areaWidth, areaHeight, xSections, ySections, w, h) shared(areaVertices) num_threads(numberOfAreas)
    {

      int threadID = omp_get_thread_num();
      int line = (int)floor((threadID / xSections));
      int column = threadID % xSections;
      int lastColumn = (column == (xSections -1));
      int lastLine = (line == (ySections -1));

      // different calculation of sections if ySections is odd!
      if (ySections % 2 != 0) {
        startX = areaWidth * (threadID % xSections);
        endX = (areaWidth * ((threadID % xSections) + 1)) - 1;
        startY = areaHeight * (threadID % ySections);
        endY = (areaHeight * ((threadID % ySections) + 1)) - 1;
      }
      else {
        startX = areaWidth * (threadID % xSections);
        endX = (areaWidth * ((threadID % xSections) + 1)) - 1;
        startY = areaHeight * line;
        endY = (areaHeight * (line + 1)) - 1;
      }
    
      //right edge
      if (lastColumn) {
        endX = w -1;
      }

      // upper edge
      if (lastLine) {
        endY = h - 1;
      }
      
      evolve(currentfield, newfield, startX, endX, startY, endY, w, h);
      writeVTK2Piece(timestep, currentfield, "gol", startX, endX, startY, endY, w, h, threadID);
      areaVertices[threadID * 4 + 0] = startX;
      areaVertices[threadID * 4 + 1] = endX;
      areaVertices[threadID * 4 + 2] = startY;
      areaVertices[threadID * 4 + 3] = endY;
    }

    writeVTK2Container(timestep, currentfield, "gol", w, h, areaVertices, numberOfAreas);
    
    
    printf("%ld timestep\n",timestep);
    usleep(200000);

    //SWAP
    double *temp = currentfield;
    currentfield = newfield;
    newfield = temp;
  }
  
  free(currentfield);
  free(newfield);
  free(areaVertices);
  
}
 
int main(int c, char **v) {
  int w = 0, h = 0;
  if (c > 1) w = atoi(v[1]); ///< read width
  if (c > 2) h = atoi(v[2]); ///< read height
  if (w <= 0) w = 30; ///< default width
  if (h <= 0) h = 30; ///< default height
  game(w, h);
}