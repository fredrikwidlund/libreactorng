for file in /root/build/JSONTestSuite/test_parsing/y_*
do
    ./validate $file > /dev/null || echo $file
done

for file in /root/build/JSONTestSuite/test_parsing/n_*
do
    ./validate $file > /dev/null && echo $file
done

for file in /root/build/JSONTestSuite/test_parsing/i_*
do
    out=$(./validate $file) && echo [$file] $out
done
