#!/bin/sh

FILES="/tmp/to_remove.$$"

find | grep "CMakeCache.txt" > $FILES
find | grep "CMakeFiles" >> $FILES
find | grep "Makefile" >> $FILES
find | grep "cmake_install.cmake" >> $FILES
find | grep ".depends$" >> $FILES
find | grep ".cpp_parameters$" >> $FILES
find | grep "autogen" >> $FILES

while read FILE
do
    rm -rf $FILE
done < $FILES
rm -f $FILES
rm -rf _deps

scripts/CodeCounter.sh
