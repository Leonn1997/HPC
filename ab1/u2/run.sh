#!/usr/bin/env bash
cc -Wall -std=c99 -D _BSD_SOURCE -fopenmp -o hpcu1 error1.c

cc -Wall -std=c99 -D _BSD_SOURCE -fopenmp -o hpcu2 error2.c

OPEN_NUM_THREADS=4 ./hpcu1

echo ""
echo ""
echo "error1 finished!"
echo "--------------------------------------------------------------------------------------------------------------"
echo ""
echo ""

OPEN_NUM_THREADS=4 ./hpcu2

echo ""
echo ""
echo "error 2 finished!"
echo "--------------------------------------------------------------------------------------------------------------"
