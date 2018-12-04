#/bin/bash
make
prlimit --nofile=300000 ./httpserver
