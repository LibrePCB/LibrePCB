#!/bin/bash

rm -r svg
mkdir svg

for f in *.dia
do
  dia -e "svg/`basename "$f" .dia`.svg" "$f"
done
