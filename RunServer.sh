#/bin/bash

prlimit --nofile=300000 ./httpserver
