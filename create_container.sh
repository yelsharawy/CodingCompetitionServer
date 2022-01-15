#!/bin/sh

if [ ! $1 ]
then
    echo "requires directory name" > /dev/stderr
    exit -1
fi

mkdir $1

cd $1

unshare -mr ../setup_container.sh

cd ..
