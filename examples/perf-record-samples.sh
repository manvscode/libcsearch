#!/bin/bash

PROCESS_ID=`ps | grep pathfinding | awk '{print $1}'`

echo "Recording samples on PathFinding Application (pid = $PROCESS_ID)..."
echo "Pres CTRL-C when done."

perf record -f -g -p $PROCESS_ID
perf report
