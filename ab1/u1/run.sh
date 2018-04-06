cc -Wall -std=c99 -D _BSD_SOURCE -fopenmp -o helloWorld helloWorld.c

OPEN_NUM_THREADS=4 ./helloWorld
