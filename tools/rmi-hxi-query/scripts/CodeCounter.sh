#!/bin/sh

COUNTER="/tmp/counter.$$"
C_LINES=0
C_BYTES=0
C_FILES=0
M_LINES=0
M_BYTES=0
M_FILES=0
T_LINES=0
T_BYTES=0
T_FILES=0

find | grep "\.cxx$" > $COUNTER
find | grep "\.hxx$" >> $COUNTER
find | grep "\.c$" >> $COUNTER
find | grep "\.h$" >> $COUNTER
find | grep "\.S$" >> $COUNTER
while read FILE
do
    B=$(cat $FILE | wc -c)
    L=$(cat $FILE | wc -l)
    C_FILES=$((C_FILES+1))
    C_BYTES=$((B+C_BYTES))
    C_LINES=$((L+C_LINES))
done < $COUNTER
rm $COUNTER

find | grep "CMakeLists.txt$" > $COUNTER
find | grep "\.sh$" >> $COUNTER
while read FILE
do
    B=$(cat $FILE | wc -c)
    L=$(cat $FILE | wc -l)
    M_FILES=$((M_FILES+1))
    M_BYTES=$((B+M_BYTES))
    M_LINES=$((L+M_LINES))
done < $COUNTER
rm $COUNTER

find | grep "\.ts$" > $COUNTER
while read FILE
do
    B=$(cat $FILE | wc -c)
    L=$(cat $FILE | wc -l)
    T_FILES=$((T_FILES+1))
    T_BYTES=$((B+T_BYTES))
    T_LINES=$((L+T_LINES))
done < $COUNTER
rm $COUNTER

echo "--- REPOSITORY SUMMARY ---"
echo "source files: $C_FILES"
echo "source lines: $C_LINES"
echo "source bytes: $C_BYTES"
echo "--------------------------"
echo "cmake files:  $M_FILES"
echo "cmake lines:  $M_LINES"
echo "cmake bytes:  $M_BYTES"
echo "--------------------------"
echo "i18n files:   $T_FILES"
echo "i18n lines:   $T_LINES"
echo "i18n bytes:   $T_BYTES"
