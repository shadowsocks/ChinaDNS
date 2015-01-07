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

run_test ./autogen.sh
run_test ./configure --enable-debug
run_test make
run_test tests/test.py tests/google.com
run_test tests/test.py tests/facebook.com
run_test tests/test.py tests/twitter.com

gcov src/*.c
cd src && gcovr -r . --html  --html-details  -o index.html
gcovr -r . | grep TOTAL | rev | cut -d' ' -f 1 | rev  > /tmp/chinadns-coverage

exit $result
