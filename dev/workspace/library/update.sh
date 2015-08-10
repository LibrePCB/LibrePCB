#!/bin/bash

echo "revert changes..."
git clean -dfq -e update.sh 
git checkout -- .

echo "update library..."
for i in $(find . -maxdepth 3 -mindepth 3 -type d) # foreach library element's root directory
do
    type=`echo $i | cut -d / -f 3`
    mkdir -p "$i.$type/v0"
    mv $i/* "$i.$type/v0/"
    mv "$i.$type/v0/${type}_v0.xml" "$i.$type/v0/$type.xml"
    rmdir $i
done

mkdir -p "LibrePCB_Base_{ad523ae0-9493-48bc-86b7-049a13cb35e2}/repo/v0"
mv "LibrePCB_Base_{ad523ae0-9493-48bc-86b7-049a13cb35e2}/repository.png" "LibrePCB_Base_{ad523ae0-9493-48bc-86b7-049a13cb35e2}/repo/v0/image.png"
mv "LibrePCB_Base_{ad523ae0-9493-48bc-86b7-049a13cb35e2}/repo_v0.xml" "LibrePCB_Base_{ad523ae0-9493-48bc-86b7-049a13cb35e2}/repo/v0/repo.xml"

rm local/*
touch local/.empty

echo "finished!"
