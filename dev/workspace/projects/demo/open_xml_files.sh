#!/bin/sh

find . \( -name '*.xml' -o -name '*.lpp' \) -exec gedit --new-window {} +
