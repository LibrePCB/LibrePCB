#!/bin/bash

# Note: the PDF files must be created "by hand" (e.g. with the print-to-pdf feature of LibreCAD)

mkdir -p png

for f in *.pdf
do
  convert -density 600 -trim -border 100x100 -bordercolor White "$f" "png/`basename "$f" .pdf`.png"
  # rm "$f"
done
