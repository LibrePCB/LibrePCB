#!/bin/bash

# generate doxygen HTML output
doxygen Doxyfile

# generate Qt helpfile
qhelpgenerator ./output/html/index.qhp -o ./output/qt_helpfile.qch
