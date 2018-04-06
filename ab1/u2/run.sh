#!/usr/bin/env bash
cc -Wall -std=c99 -D _BSD_SOURCE -fopenmp -o error1 error1.c

cc -Wall -std=c99 -D _BSD_SOURCE -fopenmp -o error2 error2.c

OPEN_NUM_THREADS=4 ./error1

echo ""
echo ""
echo "error1 finished!"
echo "--------------------------------------------------------------------------------------------------------------"
echo ""
echo ""

OPEN_NUM_THREADS=4 ./error2

echo ""
echo ""
echo "error 2 finished!"
echo "--------------------------------------------------------------------------------------------------------------"
