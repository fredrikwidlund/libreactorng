#!/bin/sh

#for file in data hash buffer list vector map mapi maps mapd string value encode decode
for file in data hash buffer list vector map mapi maps mapd string value encode
do
    echo [$file]
    test=`gcov -b src/value/libvalue_test_a-$file | grep -A4 File.*$file`
    echo "$test"
    echo "$test" | grep '% of' | grep '100.00%' >/dev/null || exit 1
    echo "$test" | grep '% of' | grep -v '100.00%' >/dev/null && exit 1
done
exit 0
