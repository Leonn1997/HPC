#!/usr/bin/env bash
cc -Wall -std=c99 -D _BSD_SOURCE -fopenmp -o pi1 pi.c
cc -Wall -std=c99 -D _BSD_SOURCE -fopenmp -o pi2 pi.1.c
cc -Wall -std=c99 -D _BSD_SOURCE -fopenmp -o pi3 pi.2.c
cc -Wall -std=c99 -D _BSD_SOURCE -fopenmp -o pi4 pi.3.c
cc -Wall -std=c99 -D _BSD_SOURCE -fopenmp -o pi5 pi.4.c



OPEN_NUM_THREADS=4 ./pi1

echo ""
echo ""
echo "pi 1 finished!"
echo "---------------------------------------------------------------------------------------------------"
echo ""
echo ""

OPEN_NUM_THREADS=4 ./pi2

echo ""
echo ""
echo "pi 2 finished!"
echo "---------------------------------------------------------------------------------------------------"
echo ""
echo ""


OPEN_NUM_THREADS=4 ./pi3

echo ""
echo ""
echo "pi 3 finished!"
echo "---------------------------------------------------------------------------------------------------"
echo ""
echo ""

OPEN_NUM_THREADS=4 ./pi4

echo ""
echo ""
echo "pi 4 finished!"
echo "---------------------------------------------------------------------------------------------------"
echo ""
echo ""

OPEN_NUM_THREADS=4 ./pi5

echo ""
echo ""
echo "pi 5 finished!"
echo "---------------------------------------------------------------------------------------------------"
echo ""
echo ""



