#!/bin/bash
HOME=/home/mosaictest
PATH=$PATH:/bin:/usr/bin
LOG_DIR=$HOME/log
mkdir -p $LOG_DIR
LOGFILE=$LOG_DIR/nightly-tests-`date +%m-%d-%Y`.log
cd $HOME/tests
svn up
date >> $LOGFILE
python auto_build_test.py >> $LOGFILE 2>&1


