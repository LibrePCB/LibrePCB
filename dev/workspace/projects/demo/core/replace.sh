#!/bin/bash

while read l; do
  if test "$l" = "<position x=\"20\" y=\"20\" angle=\"0\"/>"; then
    echo "<position x=\"$(($RANDOM % 300))\" y=\"$(($RANDOM % 300))\" angle=\"0\"/>"
  else
    echo "$l"
  fi
done <circuit.xml >circuit2.xml 
