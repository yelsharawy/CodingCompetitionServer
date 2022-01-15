#!/bin/sh

for vital in bin usr lib lib64
do
    echo mounting /$vital/
    mkdir ./$vital/
    mount --bind -o ro /$vital/ ./$vital/
done

ls ./lib