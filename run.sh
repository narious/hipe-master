#!/bin/bash

compile_server() {
    cd ./server/ || return
    if [ ! -f Makefile ]
    then
        echo "qmake server"
        qmake -makefile ./src/hipe.pro
        make hiped
    else
        echo "make server"
        make hiped
    fi
    cd ..
}

compile_testing() {
    cd ./api/ || return
    make && echo "make api"
    if [ ! -f /usr/local/include/hipe.h ]
    then
       sudo make install
    fi
    make testing
    cd ..
}

kill_all() {
        for pid in $(pgrep hiped); do
            kill -s 9 "$pid"
        done
        for pid in $(pgrep hipe-clock); do
            kill -s 9 "$pid"
        done
        echo "kill all"
}

compile_all() {
    compile_server & compile_testing
    echo "compile finish"
}

run_all() {
    ./server/hiped &
    sleep 1s
    ./api/test/hipe-clock &
    echo "run finish"
}

case "$1" in
    c) compile_all ;;
    r) run_all ;;
    k) kill_all ;;
    *) compile_all && run_all
       sleep 1s
       kill_all
       ;;
esac
