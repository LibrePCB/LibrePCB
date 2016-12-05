#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import os 
import glob
from shutil import copyfile


"""
This script sorts the entries of HEADERS, SOURCES and FORMS variables in
all qmake project files (*.pro) in alphabetical order (except those in 
submodules). This reduces the risk of conflicts when merging or rebasing
commits which modify *.pro files and makes those files cleaner and thus 
easier to read.

This script may be used as a pre-commit hook:
ln -rsT sort_qmake_file_entries.py ../.git/hooks/pre-commit
"""


def get_repository_root_dir(start_dir):
    if os.path.isdir(os.path.join(start_dir, '.git')):
        return start_dir
    else:
        parent_dir = os.path.dirname(start_dir)
        if parent_dir != start_dir:
            return get_repository_root_dir(parent_dir)
        else:
            return None


def sort_qmake_file(project_root, filepath):
    with open(filepath, 'r') as file:
        old_lines = list(file.readlines())
        new_lines = list(old_lines)
    block_start_index = None
    for line_index, line in enumerate(old_lines):
        if block_start_index is None:
            is_headers = line.startswith("HEADERS += \\")
            is_sources = line.startswith("SOURCES += \\")
            is_forms = line.startswith("FORMS += \\")
            if is_headers or is_sources or is_forms:
                block_start_index = line_index + 1
        else:
            if not line.strip():
                block_end_index = line_index
                block_lines = sorted(old_lines[block_start_index:block_end_index])
                for i, l in enumerate(block_lines):
                    adjusted_line = l.replace("\\", "").rstrip() + " \\\n"
                    new_lines[block_start_index+i] = adjusted_line
                block_start_index = None
    relative_path = os.path.relpath(filepath, project_root)
    if new_lines != old_lines:
        print("[M] {}".format(relative_path))
        copyfile(filepath, filepath + '~')
        with open(filepath, 'w') as file:
            file.writelines(new_lines)
        return 1
    else:
        print("[ ] {}".format(relative_path))
        return 0


def sort_qmake_files_in_dir(project_root, dir):
    modified_files = 0
    dot_git_file = os.path.join(dir, '.git')
    if not os.path.isfile(dot_git_file):
        has_qmake_file = False
        for entry in glob.glob(os.path.join(dir, '*.pro')):
            if os.path.isfile(entry):
                has_qmake_file = True
                modified_files += sort_qmake_file(project_root, entry)
        if has_qmake_file is True:
            for entry in glob.glob(os.path.join(dir, '*/')):
                filename = os.path.basename(entry)
                if not os.path.basename(entry).startswith('.'):
                    modified_files += sort_qmake_files_in_dir(project_root, entry)
    return modified_files


def main():
    # get the root directory of the project's repository
    script_dir = os.path.dirname(os.path.realpath(__file__))
    project_root_dir = get_repository_root_dir(script_dir)
    if not os.path.isfile(os.path.join(project_root_dir, 'librepcb.pro')):
        raise Exception("Could not find 'librepcb.pro' in the project's root directory!")

    # search all *.pro files and sort their HEADERS, SOURCES and FORMS entries
    print("Sort *.pro file entries...")
    modified_files = sort_qmake_files_in_dir(project_root_dir, project_root_dir)
    print("Finished: {} Files modified.".format(modified_files))
    return modified_files


if __name__ == "__main__":
    # exit with code != 0 if files were modified
    # -> aborts the commit when used as pre-commit hook!
    exit(main())

