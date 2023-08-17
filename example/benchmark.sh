#!/bin/bash

make -j8 -C .. && make -j8 server clients
ulimit -n 10000


echo start server
taskset -c 0 ./server &

echo start load
taskset -c 1 ./clients

kill $(jobs -p)
sleep 2

echo start saved server
taskset -c 0 ./server.saved &

echo start load
taskset -c 1 ./clients

kill $(jobs -p)
sleep 2

echo start server
taskset -c 0 ./server &

echo start load
taskset -c 1 ./clients

kill $(jobs -p)
sleep 2

echo start saved server
taskset -c 0 ./server.saved &

echo start load
taskset -c 1 ./clients

kill $(jobs -p)
sleep 2
