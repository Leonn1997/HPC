#!/usr/bin/env bash
cc -Wall -std=c99 -D _BSD_SOURCE -fopenmp -o hpcu1 pi.4.c

OPEN_NUM_THREADS=6 ./hpcu1
