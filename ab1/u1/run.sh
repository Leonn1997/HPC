cc -Wall -std=c99 -D _BSD_SOURCE -fopenmp -o hpcu1 helloWorld.c

OPEN_NUM_THREADS=4 ./hpcu1
