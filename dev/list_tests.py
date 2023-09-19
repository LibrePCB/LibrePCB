#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Simple script to list all source files which do not have a corresponding
unit test file, to get an overview about the test coverage. Just call
this script with no arguments provided.

The matching of source files to test files is quite trivial. The script finds
all *.h files (e.g. "length.h") and checks if there's a *test.cpp file
(e.g. "lengthtest.cpp").
"""

import os
import glob


if __name__ == '__main__':
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    sources_dir = os.path.join(root, 'libs', 'librepcb')
    sources = [os.path.relpath(x, sources_dir)
               for x in glob.glob(sources_dir + '/**/*.h', recursive=True)]
    tests_dir = os.path.join(root, 'tests', 'unittests')
    tests = [os.path.relpath(x, tests_dir)
             for x in glob.glob(tests_dir + '/**/*.cpp', recursive=True)]

    matches = []
    missing = []
    for source in sources:
        test = source[:-2] + 'test.cpp'
        if test in tests:
            matches.append((source, test))
        else:
            missing.append(source)

    print("\nSources with test:")
    for source, test in sorted(matches):
        print("{} => {}".format(source, test))

    print("\nSources without test:")
    for source in sorted(missing):
        print(source)

    print("\nTests without source:")
    tests_without_sources = set(tests) - set([x[1] for x in matches])
    for test in sorted(tests_without_sources):
        print(test)

    print()
    print("Found {} sources and {} tests.".format(len(sources), len(tests)))
    print("Sources with test:    {:3}".format(len(matches)))
    print("Sources without test: {:3}".format(len(missing)))
    print("Tests without source: {:3}".format(len(tests_without_sources)))
