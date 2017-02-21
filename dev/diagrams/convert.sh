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

# update images in doxygen documentation
inkscape -z -f svg/architecture_overview.svg -w 1000 -e ../doxygen/images/architecture_overview.png
inkscape -z -f svg/library_structure.svg -w 1200 -e ../doxygen/images/library_structure.png
inkscape -z -f svg/library_structure_overview.svg -w 800 -e ../doxygen/images/library_structure_overview.png
inkscape -z -f svg/library_structure_examples.svg -w 1800 -e ../doxygen/images/library_structure_examples.png
