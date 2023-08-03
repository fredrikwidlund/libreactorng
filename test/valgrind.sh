#!/bin/sh

if command -v valgrind; then
    for file in include data hash buffer list vector map mapi maps mapd string value encode pool timeout notify network
    do
        echo [$file]
        if ! valgrind --track-fds=yes --error-exitcode=1 --leak-check=full --show-leak-kinds=all test/$file; then
            exit 1
        fi
    done
fi
