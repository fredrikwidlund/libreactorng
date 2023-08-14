#!/bin/bash

make -C .. && make
ulimit -n 10000

echo start server
taskset -c 0 ./server &

echo start load
taskset -c 1 pounce -d 5 -t 1 -c 128 http://127.0.0.1

kill $(jobs -p)
