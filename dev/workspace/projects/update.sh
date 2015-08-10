#!/bin/bash

echo "revert changes..."
git clean -dfq -e update.sh 
git checkout -- .

echo "update projects..."
IFS=$'\n'
for i in $(find . -maxdepth 1 -mindepth 1 -type d) # foreach project
do
    for k in $(find "$i/library" -maxdepth 2 -mindepth 2 -type d) # foreach library element's root directory
    do
        type=`echo $k | cut -d / -f 4`
        mkdir -p "$k.$type/v0"
        mv $k/* "$k.$type/v0/"
        mv "$k.$type/v0/${type}_v0.xml" "$k.$type/v0/$type.xml"
        rmdir $k
    done
done

echo "finished!"
