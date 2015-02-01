#!/bin/bash

result=0

function run_test {
    printf '\e[0;36m'
    echo "running test: $command $@"
    printf '\e[0m'

    $command "$@"
    status=$?
    if [ $status -ne 0 ]; then
        printf '\e[0;31m'
        echo "test failed: $command $@"
        printf '\e[0m'
        echo
        result=1
    else
        printf '\e[0;32m'
        echo OK
        printf '\e[0m'
        echo
    fi
    return 0
}

rm src/*.gc*
run_test ./autogen.sh
run_test ./configure --enable-debug
make clean
run_test make
run_test tests/test.py -a '-c chnroute.txt -l iplist.txt' -t tests/google.com
run_test tests/test.py -a '-c chnroute.txt -l iplist.txt' -t tests/facebook.com
run_test tests/test.py -a '-c chnroute.txt -l iplist.txt' -t tests/twitter.com
run_test tests/test.py -a '-d -c chnroute.txt -l iplist.txt' -t tests/twitter.com
run_test tests/test.py -a '-c chnroute.txt' -t tests/twitter.com
run_test tests/test.py -a '-m -c chnroute.txt' -t tests/twitter.com

gcov src/*.c
rm src/*.html
cd src && gcovr -r . --html  --html-details  -o index.html
gcovr -r . | grep TOTAL | rev | cut -d' ' -f 1 | rev  > /tmp/chinadns-coverage

exit $result
