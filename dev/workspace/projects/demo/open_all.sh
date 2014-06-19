#!/bin/sh

gedit --new-window `find . -name *.xml | sort` `find ../../lib/ -name *.xml | sort`
