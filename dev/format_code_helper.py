#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# USAGE: Do not use this script directly. Just use format_code.sh, which will
# then call this script automatically.

import re
import sys


def format_cxx(path, extension, content):
    # Derive include guard define name from file path.
    if extension == 'h':
        segments = path.upper().replace('-', '_').replace('.', '_').split('/')
        guard = segments[1] + '_' + segments[2]
        if len(segments) > 3:
            guard += '_' + segments[-1]
        content = re.sub(r'#ifndef \w+\n#define \w+\n\n',
                         r'#ifndef {0}\n#define {0}\n\n'.format(guard.upper()),
                         content)

    # Remove comment from closing include guard.
    content = re.sub(r'#endif  // \w+', r'#endif', content)

    # Remove empty lines in front of opening namespaces.
    content = re.sub(r'\*/\n\nnamespace', r'*/\nnamespace', content)

    # Remove superfluous empty lines between opening namespaces.
    content = re.sub(r'namespace (\w+) {\n\nnamespace (\w+) {',
                     r'namespace \1 {\nnamespace \2 {', content)

    # Remove superfluous empty lines between closing namespaces.
    content = re.sub(r'}  // namespace (\w+)\n\n}  // namespace (\w+)',
                     r'}  // namespace \1\n}  // namespace \2', content)

    # Sort forward declarations and ensure consistent empty lines.
    matches = list(re.finditer(r'{\n+((class|struct) \w+;\n+)+', content))
    matches.reverse()
    for m in matches:
        new_block = [x for x in m.group().replace('{', '').split('\n') if x]
        new_block.sort()
        separator = '\n\n' if content[m.end()] != '}' else '\n'
        new_block = '{' + separator + "\n".join(new_block) + separator
        content = content[:m.start()] + new_block + content[m.end():]

    # Add empty line before opening temporary namespaces.
    content = re.sub(r'namespace (\w+) {\nnamespace (\w+) {\n(class|struct)',
                     r'namespace \1 {\n\nnamespace \2 {\n\3', content)

    return content


def format_ui(content):
    # Remove unnecessary paths to *.qrc files.
    content = re.sub(r'\s*resource=".*\.qrc"', r'', content)
    content = re.sub(r'<resources>.*</resources>',
                     r'<resources/>', content, flags=(re.M | re.S))
    return content


def format_slint(content):
    # Sort import statements
    matches = list(re.finditer(r'(import|export) \{\n([^\}]*)', content, re.MULTILINE))
    matches.reverse()
    for m in matches:
        imports = [s.strip() for s in m.group(2).split(',') if s.strip()]
        sorted_imports = sorted(list(set(imports)))
        new_content = ',\n'.join(['    ' + s for s in sorted_imports]) + ',\n'
        content = content[:m.start(2)] + new_content + content[m.end(2):]
    return content


if __name__ == '__main__':
    # Get relative file path.
    path = sys.argv[1]
    extension = path.split('.')[-1]

    # Read file content from stdin.
    content = sys.stdin.read()

    # Format content
    if extension in ['cpp', 'h']:
        content = format_cxx(path, extension, content)
    elif extension == 'ui':
        content = format_ui(content)
    elif extension == 'slint':
        content = format_slint(content)

    # Write file content to stdout.
    sys.stdout.write(content)
