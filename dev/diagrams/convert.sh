#!/bin/bash

rm -r svg
mkdir svg

find . -type f -name "*.dia" | while read f
do
  mkdir -p "svg/"`dirname "$f"`
  dia -e "svg/${f%.dia}.svg" "$f"
done
