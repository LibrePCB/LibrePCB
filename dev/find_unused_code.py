#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Script which searches for unused code or files in the repository and
reports them on stdout.
"""

import glob
import os
import sys
from subprocess import CalledProcessError, check_output as run

REPO_DIR = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

WHITELIST = [
    'img/app/librepcb.svg',
    'img/norm/iec_60617.png',
    'img/norm/ieee_315.png',
    'libs/librepcb/core/serialization/fileformatmigrationv1.h',
    'libs/librepcb/core/workspace/workspacesettingsitem_genericvalue.h',
    'libs/librepcb/editor/editorcommandcategory.h',
    'libs/librepcb/editor/editorcommandsetupdater.h',
    'libs/librepcb/editor/utils/deriveduiobjectlistview.h',
    'tests/unittests/core/serialization/serializableobjectmock.h',
    'tests/unittests/editor/widgets/editabletablewidgetreceiver.h',
]

if __name__ == '__main__':
    warnings = 0

    # Entries in .reuse/dep5 which don't exist anymore.
    print("Check for stale entries in .reuse/dep5...")
    with open(os.path.join(REPO_DIR, '.reuse/dep5'), 'r') as f:
        for line in f.readlines():
            if line.startswith('Files: '):
                pattern = line.replace('Files: ', '').strip()
            elif line.startswith(' ') and not line.startswith('  '):
                pattern = line.strip()
            else:
                continue
            entries = glob.glob(pattern, root_dir=REPO_DIR)
            if len(entries) == 0:
                print(' - ' + pattern)
                warnings += 1

    # Images which are not used anywhere.
    print("Check for unused images...")
    files = run(['git', 'ls-files',
                 '--cached', '--others', '--exclude-standard',
                 '--', ':/img/**.png', ':/img/**.svg', ':/img/**.jpg',
                 ':/img/**.jpeg'],
                cwd=REPO_DIR).decode("utf-8").splitlines()
    for file in files:
        if file in WHITELIST:
            continue
        if not os.path.isfile(os.path.join(REPO_DIR, file)):
            continue
        try:
            references = run(['git', 'grep', '-IlF', file,
                              '--', '*.cpp', '*.h', '*.ui', '*.slint'],
                             cwd=REPO_DIR).decode("utf-8").splitlines()
        except CalledProcessError:
            print(' - ' + file)
            warnings += 1

    # Headed files not included anywhere.
    print("Check for unused header files...")
    files = run(['git', 'ls-files', '--cached', '--others',
                 '--exclude-standard', '--', '*.h'],
                cwd=REPO_DIR).decode("utf-8").splitlines()
    for file in files:
        if file in WHITELIST:
            continue
        if not os.path.isfile(os.path.join(REPO_DIR, file)):
            continue
        try:
            references = run(['git', 'grep', '-IlF', os.path.basename(file),
                              '--', '*.cpp', '*.h', '*.ui'],
                             cwd=REPO_DIR).decode("utf-8").splitlines()
            if len(references) < 2:
                print(' - ' + file + ' (only 1 usage)')
                warnings += 1
        except CalledProcessError:
            print(' - ' + file)
            warnings += 1

    # Report result.
    if warnings > 0:
        print("Failed with {} warnings".format(warnings))
        sys.exit(1)
    else:
        print("Done, no warnings")
