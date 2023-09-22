#!/usr/bin/env bash

rm -r svg
mkdir svg

# convert all *.dia files to *.svg files
find . -type f -name "*.dia" | while read f
do
  mkdir -p "svg/"$(dirname "$f")
  outfile="svg/${f%.dia}.svg"
  dia -e "$outfile" "$f"
done

# update images in doxygen documentation
inkscape -z -w 1000 --export-background=white --export-type=png \
  -o ../doxygen/images/architecture_overview.png svg/architecture_overview.svg
inkscape -z -w 1200 --export-background=white --export-type=png \
  -o ../doxygen/images/attributes_system.png svg/attributes_system.svg
inkscape -z -w 1200 --export-background=white --export-type=png \
  -o ../doxygen/images/library_structure.png svg/library_structure.svg
inkscape -z -w 800 --export-background=white --export-type=png \
  -o ../doxygen/images/library_structure_overview.png svg/library_structure_overview.svg
inkscape -z -w 1800 --export-background=white --export-type=png \
  -o ../doxygen/images/library_structure_examples.png svg/library_structure_examples.svg
