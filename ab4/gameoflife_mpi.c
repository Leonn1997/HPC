#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#include "mpi.h"

#define calcIndex(width, x,y)  ((y)*(width) + (x))

long TimeSteps = 100;

void writeVTK2Piece(long timestep, double *data, char prefix[1024], int w, int h, int overallWidth, int processRank) {
  char filename[2048];  
  int x,y; 
  
  long offsetX = w * processRank;
  long  nxy = w * h * sizeof(float);  

  snprintf(filename, sizeof(filename), "%s-%05ld-%02d%s", prefix, timestep, processRank, ".vti");
  FILE* fp = fopen(filename, "w");

  fprintf(fp, "<?xml version=\"1.0\"?>\n");
  fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
  fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d 0 0\" Origin=\"0 0 0\" Spacing=\"1 1 0\">\n", 0, overallWidth, 0, h);
  fprintf(fp, "<Piece Extent=\"%d %d %d %d 0 0\">\n", offsetX, offsetX + w, 0, h);
  fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
  fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
  fprintf(fp, "</CellData>\n");
  fprintf(fp, "</Piece>\n");
  fprintf(fp, "</ImageData>\n");
  fprintf(fp, "<AppendedData encoding=\"raw\">\n");
  fprintf(fp, "_");
  fwrite((unsigned char*)&nxy, sizeof(long), 1, fp);

  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      float value = data[calcIndex(w,x,y)];
      fwrite((unsigned char*)&value, sizeof(float), 1, fp);
    }
  }
  
  fprintf(fp, "\n</AppendedData>\n");
  fprintf(fp, "</VTKFile>\n");
  fclose(fp);
}

void writeVTK2Container(long timestep, char prefix[1024], long w, long h, int partialWidth, int numberOfProcesses) {
  char filename[2048];

  snprintf(filename, sizeof(filename), "%s-%05ld%s", prefix, timestep, ".pvti");
  FILE* fp = fopen(filename, "w");

  fprintf(fp,"<?xml version=\"1.0\"?>\n");
  fprintf(fp,"<VTKFile type=\"PImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
  fprintf(fp,"<PImageData WholeExtent=\"%d %d %d %d 0 0\" Origin=\"0 0 0\" Spacing =\"1 1 0\" GhostLevel=\"1\">\n", 0, w, 0, h);
  fprintf(fp,"<PCellData Scalars=\"%s\">\n", prefix);
  fprintf(fp,"<PDataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
  fprintf(fp,"</PCellData>\n");

  for(int i = 0; i < numberOfProcesses; i++) {
    fprintf(fp, "<Piece Extent=\"%d %d %d %d 0 0\" Source=\"%s-%05ld-%02d%s\"/>\n", i * partialWidth, (i + 1) * partialWidth, 0, h, prefix, timestep, i, ".vti");
  }

  fprintf(fp,"</PImageData>\n");
  fprintf(fp, "</VTKFile>\n");
  fclose(fp);
}

void printToFile(double* field,char prefix[1024], int w, int h, int rank) {
  char filename[2048]; 
  snprintf(filename, sizeof(filename), "%s-%d%s", prefix, rank, ".txt");
  FILE* fp = fopen(filename, "w");

  int x,y;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) fprintf(fp, field[calcIndex(w, x,y)] ? "X" : "_");
    fprintf(fp, "\n");
  }

  fclose(fp);
}

void show(double* currentfield, int w, int h) {
  int x,y;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) printf(currentfield[calcIndex(w, x,y)] ? "X" : "_");
    printf("\n");
  }
  fflush(stdout);
}


int evolve(double* currentfield, double* newfield, double* leftGhostLayer, double* rightGhostLayer, int w, int h) {
  int x,y;
  int changed = 0;

  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      
      int n = countNeighbours(currentfield, leftGhostLayer, rightGhostLayer, x, y, w, h);
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
      
      changed += (currentfield[index] != newfield[index]);
    }
  }

  return changed;
}

int countNeighbours(double* currentfield, double* leftGhostLayer, double* rightGhostLayer, int x, int y, int w, int h) {
  int n = 0;

  for (int neighbourX = (x-1); neighbourX <= (x+1); neighbourX++) {
    for (int neighbourY = (y-1); neighbourY <= (y+1); neighbourY++) {
      if (neighbourX == -1) {
          n += leftGhostLayer[calcIndex(1, 0, neighbourY % h)];
      }
      else if (neighbourX == w) {
          n += rightGhostLayer[calcIndex(1, 0, neighbourY % h)];
      }
      else {
        n += currentfield[calcIndex(w, (neighbourX + w) % w, (neighbourY + h) % h)];
      }
    }
  }

  n -= currentfield[calcIndex(w, x, y)];
  return n;
}

void filling(double* currentfield, int w, int h) {
  int i;
  for (i = 0; i < h*w; i++) {
    currentfield[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
  }
}

void printProcessResponsibility(int rank, int w, int h) {
  int startX, endX, startY, endY;
  startX = rank * w;
  endX = (startX + w) - 1;
  startY = 0;
  endY = h -1;
  printf("Hallihallo ich bin Prozess %i. Ich berechne folgendes rechteck:\n", rank);
  printf("startX=%i endX=%i startY=%i endY=%i\n", startX, endX, startY, endY);
  printf("Mein Gebiet ist %i * %i grooooß!\n\n", w,h);
}

void buildGhostLayers(int h, int w, double *leftGhostLayer, double *rightGhostLayer, double *currentfield) {
  for (int row = 0; row < h; row++) {
      int index = calcIndex(w, 0, row);
      leftGhostLayer[row] = currentfield[index];
      index = calcIndex(w, (w - 1), row);
      rightGhostLayer[row] = currentfield[index];
    }
}

void printCurrentTimestep(int rank, int timestep) {
    if (rank == 0) {
      printf("Timestep %d\n", timestep);
    }
}

void persistentTimeStep(double* currentfield, int rank,  int timestep, int width, int height, int overallWidth, int overallHeight, int numberOfProcesses ) {
   writeVTK2Piece(timestep, currentfield, "gol", width, height, overallWidth, rank);
    if (rank == 0) {
      writeVTK2Container(timestep, "gol", overallWidth, overallHeight, width, numberOfProcesses);
    }
}

void game(int overallWidth, int overallHeight, double initialfield[], MPI_Comm communicator, int rank, int numberOfProcesses) {
  int w = (overallWidth / numberOfProcesses);
  int h = overallHeight;
  double* currentfield = (double*)calloc(w * h, sizeof(double));
  memcpy(currentfield, initialfield, w * h * sizeof(double));
  double* newfield = (double*)calloc(w * h, sizeof(double));
  double* leftGhostLayer = (double*)calloc(h, sizeof(double));
  double* rightGhostLayer = (double*)calloc(h, sizeof(double));
  int changed = 1;

  //Aufgabe d)
  printProcessResponsibility(rank, w, h);

  for (int t = 0; t < TimeSteps && changed; t++) {
    printCurrentTimestep(rank, t);

    buildGhostLayers(h, w, leftGhostLayer, rightGhostLayer, currentfield);
    shareGhostlayers(leftGhostLayer, rightGhostLayer, h, rank, numberOfProcesses, communicator);

    changed = evolve(currentfield, newfield, leftGhostLayer, rightGhostLayer, w, h);
    
    persistentTimeStep(currentfield, rank, t, w, h, overallWidth, overallHeight, numberOfProcesses);
    
    double *temp = currentfield;
    currentfield = newfield;
    newfield = temp;

    MPI_Allreduce(MPI_IN_PLACE, &changed, 1, MPI_INT, MPI_SUM, communicator);
  }

  free(currentfield);
  free(newfield);
  free(leftGhostLayer);
  free(rightGhostLayer);
}

void copyGhostLayerIntoSendbuffer(double* ghostLayer, double* sendbuffer, int amountOfFields) {
  memcpy(sendbuffer, ghostLayer, amountOfFields * sizeof(double));
}

void updateGhostLayer(double* buffer, double* ghost, int amount) {
  memcpy(ghost, buffer, amount * sizeof(double));
}

void exchangeIntermediateGhostLayers(double* sendbuffer, double* receivebuffer, double* leftGhostLayer, double* rightGhostLayer, int amountOfFieldsPerProcess, int rank, int numberOfProcesses, MPI_Comm communicator){
  if (rank % 2 == 0) {
    if (rank < numberOfProcesses-1) {
      copyGhostLayerIntoSendbuffer(rightGhostLayer, sendbuffer, amountOfFieldsPerProcess);
      MPI_Send(sendbuffer, amountOfFieldsPerProcess, MPI_DOUBLE, (rank+1), 1, communicator);
      MPI_Recv(receivebuffer, amountOfFieldsPerProcess, MPI_DOUBLE, (rank+1), 2, communicator, MPI_STATUS_IGNORE);
      updateGhostLayer(receivebuffer, rightGhostLayer, amountOfFieldsPerProcess);
    }
    if (rank > 0) {
      copyGhostLayerIntoSendbuffer(leftGhostLayer, sendbuffer, amountOfFieldsPerProcess);
      MPI_Send(sendbuffer, amountOfFieldsPerProcess, MPI_DOUBLE, (rank-1), 3, communicator);      
      MPI_Recv(receivebuffer, amountOfFieldsPerProcess, MPI_DOUBLE, (rank-1), 4, communicator, MPI_STATUS_IGNORE);
      updateGhostLayer(receivebuffer, leftGhostLayer, amountOfFieldsPerProcess);
    }
  } else {
    if (rank > 0) {
      copyGhostLayerIntoSendbuffer(leftGhostLayer, sendbuffer, amountOfFieldsPerProcess);
      MPI_Recv(receivebuffer, amountOfFieldsPerProcess, MPI_DOUBLE, (rank-1), 1, communicator, MPI_STATUS_IGNORE);
      updateGhostLayer(receivebuffer, leftGhostLayer, amountOfFieldsPerProcess);
      MPI_Send(sendbuffer, amountOfFieldsPerProcess, MPI_DOUBLE, (rank-1), 2, communicator);
    }
    if (rank < numberOfProcesses-1) {
      copyGhostLayerIntoSendbuffer(rightGhostLayer, sendbuffer, amountOfFieldsPerProcess);
      MPI_Recv(receivebuffer, amountOfFieldsPerProcess, MPI_DOUBLE, (rank+1), 3, communicator, MPI_STATUS_IGNORE);
      updateGhostLayer(receivebuffer, rightGhostLayer, amountOfFieldsPerProcess);
      MPI_Send(sendbuffer, amountOfFieldsPerProcess, MPI_DOUBLE, (rank+1), 4, communicator);
    }
  }
}

void exchangeEdgeGhostLayers(double* sendbuffer, double* receivebuffer, double* leftGhostLayer, double* rightGhostLayer, int amountOfFieldsPerProcess, int rank, int numberOfProcesses, MPI_Comm communicator){
  int leftEdge = rank == 0;
  int rightEdge = rank == numberOfProcesses- 1;
  if (leftEdge) {
    int leftPeriodicNeighbour, rightNeighbour;
    MPI_Cart_shift(communicator, 0, 1, &leftPeriodicNeighbour, &rightNeighbour);
    copyGhostLayerIntoSendbuffer(leftGhostLayer, sendbuffer, amountOfFieldsPerProcess);
    MPI_Send(sendbuffer, amountOfFieldsPerProcess, MPI_DOUBLE, leftPeriodicNeighbour, 1, communicator);      
    MPI_Recv(receivebuffer, amountOfFieldsPerProcess, MPI_DOUBLE, leftPeriodicNeighbour, 2, communicator, MPI_STATUS_IGNORE);
    updateGhostLayer(receivebuffer, leftGhostLayer, amountOfFieldsPerProcess);
  }

  if (rightEdge) {
    int leftNeighbour, rightPeriodicNeighbour;
    MPI_Cart_shift(communicator, 0, 1, &leftNeighbour, &rightPeriodicNeighbour);
    copyGhostLayerIntoSendbuffer(rightGhostLayer, sendbuffer, amountOfFieldsPerProcess);
    MPI_Recv(receivebuffer, amountOfFieldsPerProcess, MPI_DOUBLE, rightPeriodicNeighbour, 1, communicator, MPI_STATUS_IGNORE);
    updateGhostLayer(receivebuffer, rightGhostLayer, amountOfFieldsPerProcess);
    MPI_Send(sendbuffer, amountOfFieldsPerProcess, MPI_DOUBLE, rightPeriodicNeighbour, 2, communicator);
  }
}

void releaseBuffers(double* sendbuffer, double* receivebuffer){
  free(sendbuffer);
  free(receivebuffer);
}

void shareGhostlayers(double* leftGhostLayer, double* rightGhostLayer, int amountOfFieldsPerProcess, int rank, int numberOfProcesses, MPI_Comm communicator) {
  double* sendbuffer = (double*)calloc(amountOfFieldsPerProcess, sizeof(double));
  double* receivebuffer = (double*)calloc(amountOfFieldsPerProcess, sizeof(double));

  exchangeIntermediateGhostLayers(sendbuffer, receivebuffer, leftGhostLayer, rightGhostLayer, amountOfFieldsPerProcess, rank, numberOfProcesses, communicator);
  exchangeEdgeGhostLayers(sendbuffer, receivebuffer, leftGhostLayer, rightGhostLayer, amountOfFieldsPerProcess, rank, numberOfProcesses, communicator);

  releaseBuffers(sendbuffer, receivebuffer);
}

double* initializeField(int w, int h) {
  double* field = calloc(w * h, sizeof(double));

  int i;
  for (i = 0; i < h*w; i++) {
    field[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
  }

  return field;
}

void computeSendBuffer(double* field, double* buffer, int w, int h, int numberOfProcesses, MPI_Comm communicator) {
  int partialWidth = w / numberOfProcesses;
  int amountOfFieldsPerProcess = partialWidth * h;
  int index;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      // Maps the <w * h> domain to a <(w / #processes) * #processes> domain.
      // The <(w / #processes) * h> sub domain for each process is literraly flattened into a connected block in the array
      //
      //  |   P0    |   P1    |   P2    |
      //  |_____|_____|_____|
      //  |  1 |  2 |  3 |  4 |  5 |  6 |       | 1 | 2 |  7 |  8 | 13 | 14 | 19 | 20 |   <- P0
      //  |  7 |  8 |  9 | 10 | 11 | 12 |  =>   | 3 | 4 |  9 | 10 | 15 | 16 | 21 | 22 |   <- P1
      //  | 13 | 14 | 15 | 16 | 17 | 18 |       | 5 | 6 | 11 | 12 | 17 | 18 | 23 | 24 |   <- P2
      //  | 19 | 20 | 21 | 22 | 23 | 24 |
      index = calcIndex(amountOfFieldsPerProcess, (partialWidth * y) + (x % partialWidth), x / partialWidth);
      buffer[index] = field[calcIndex(w, x, y)];
    }
  }
}

int main(int argc, char *argv[]) {
  srand(time(NULL));

  int w = 0, h = 0;
  if (argc > 1) w = atoi(argv[1]); ///< read width
  if (argc > 2) h = atoi(argv[2]); ///< read height
  if (w <= 0) w = 21; ///< default width
  if (h <= 0) h = 13; ///< default height

  int rank, numberOfProcesses;
  MPI_Comm world = MPI_COMM_WORLD;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(world, &numberOfProcesses);
  MPI_Comm topologyCommunicator;
  // Aufgabe b: Hier ordnen wir die Processe in eine 1D Topologie
  int processesPerDimension[1] = { numberOfProcesses };
  int periodic[1] = { 1 }; 
  MPI_Cart_create(world, 1, processesPerDimension, periodic, 1, &topologyCommunicator); 
  MPI_Comm_rank(topologyCommunicator, &rank);
  MPI_Comm_size(topologyCommunicator, &numberOfProcesses);
  
  int amountOfFieldsPerProcess = (w / numberOfProcesses) * h;
  double* sendBuffer = (double *)calloc(w * h,sizeof(double));
  double* receiveBuffer = (double *)malloc(amountOfFieldsPerProcess * sizeof(double));
  
/*
 AUFGABE 1
  zeilen drüber defnieren die 1D Topologie
  Untere Zeilen dinen zur Verfikation wie gefordert
*/
 int id;
 if(rank != 0) {
  int myRank = rank;
  MPI_Recv(&id, 1, MPI_INT,(rank-1), 1, topologyCommunicator, MPI_STATUS_IGNORE); 
  printf("Ich bin Prozess %i und mein linker Nachbar ist %i \n", myRank, id);
 }
 if (rank < numberOfProcesses-1) { 
  int myRank = rank;
  MPI_Send(&myRank, 1, MPI_INT,(rank+1), 1, topologyCommunicator);
  MPI_Recv(&id, 1, MPI_INT,(rank+1), 2, topologyCommunicator, MPI_STATUS_IGNORE);
  printf("Ich bin Prozess %i und mein rechter Nachbar ist %i \n", myRank, id);
 }
  if (rank != 0) { 
  int myRank = rank; 
  MPI_Send(&myRank, 1, MPI_INT,(rank-1), 2, topologyCommunicator); 
}
  

  
  if (rank == 0) {  
    printf("Number of processes: %d\n", numberOfProcesses);
    printf("amountOfFieldsPerProcess = %d\n", amountOfFieldsPerProcess);
    double* field = initializeField(w, h);
    printf("Field:\n");
    show(field, w, h);

    computeSendBuffer(field, sendBuffer, w, h, numberOfProcesses, topologyCommunicator);
    printf("\nSendBuffer:\n");
    show(sendBuffer, amountOfFieldsPerProcess, numberOfProcesses);
  }
  
  // MPI_Scatter to distribute to all other processes
  MPI_Scatter(sendBuffer, amountOfFieldsPerProcess, MPI_DOUBLE, receiveBuffer, amountOfFieldsPerProcess, MPI_DOUBLE, 0, topologyCommunicator);

  game(w, h, receiveBuffer, topologyCommunicator, rank, numberOfProcesses);

  free(sendBuffer);
  free(receiveBuffer);
  MPI_Finalize();
  return 0;
}