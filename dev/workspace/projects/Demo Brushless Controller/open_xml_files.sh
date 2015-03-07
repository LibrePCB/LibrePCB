#!/bin/sh

find . \( -name '*.xml' -o -name '*.e4u' \) -exec gedit --new-window {} +
