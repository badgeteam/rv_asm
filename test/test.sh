#!/bin/sh

../build/rvasm inputfile outputfile
readelf -a outputfile

