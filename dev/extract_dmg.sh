#!/usr/bin/env bash

FILENAME="$1"

mkdir -p ./extract_dmg
7z x "$FILENAME" -oextract_dmg -y
sudo mkdir -p ./extract_dmg/mount
sudo mount -t hfsplus -o loop ./extract_dmg/4.hfs ./extract_dmg/mount
rm -rf ./extract_dmg/hfs
cp -r ./extract_dmg/mount ./extract_dmg/hfs
sudo umount ./extract_dmg/mount
sudo rmdir ./extract_dmg/mount
