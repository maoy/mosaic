#!/bin/bash
for FILE in `ls build/*.exe`
do
  echo running $FILE
  $FILE 127.0.0.1 1234
done

