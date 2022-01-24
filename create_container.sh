#!/bin/bash

if [ ! $1 ]
then
    echo "requires directory name" > /dev/stderr
    exit 1
fi


mkdir $1 2> /dev/null

# pushd $1

# shift 1

#env -i unshare -mCruin ../setup_container.sh $2
unshare -mCruin ./setup_container.sh $@

# popd

rm -rf $1