#!/bin/bash

rm -r svg
mkdir svg

# convert all *.dia files to *.svg files
find . -type f -name "*.dia" | while read f
do
  mkdir -p "svg/"`dirname "$f"`
  outfile="svg/${f%.dia}.svg"
  dia -e "$outfile" "$f"
  
  # ugly workaround for missing spaces in SVG texts:
  # http://stackoverflow.com/questions/27499032/svg-text-element-with-whitespace-not-preserved-in-ie#27499096
  sed -i -e 's/<text font-size=/<text xml:space="preserve" font-size=/g' "$outfile"
done
