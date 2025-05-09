#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CXX_IN = 'libs/librepcb/editor/editorcommandset.h'
CXX_OUT = 'libs/librepcb/editor/editorcommandsetupdater.h'
SLINT = 'libs/librepcb/editor/ui/api/editorcommandset.slint'

CXX_OUT_HEADER = """// GENERATED FILE! DO NOT MODIFY!
// Run dev/generate_editorcommandset.py to re-generate.

#ifndef LIBREPCB_EDITOR_EDITORCOMMANDSETUPDATER_H
#define LIBREPCB_EDITOR_EDITORCOMMANDSETUPDATER_H

#include "appwindow.h"
#include "editorcommandset.h"
#include "utils/uihelpers.h"

namespace librepcb {
namespace editor {

class EditorCommandSetUpdater {
public:
  static void update(const ui::EditorCommandSet& out) noexcept {
    EditorCommandSet& cmd = EditorCommandSet::instance();
    // clang-format off
"""

CXX_OUT_COMMAND_PATTERN = \
    "    out.set_{slint}(l2s(cmd.{cxx}, out.get_{slint}()));"

CXX_OUT_FOOTER = """
    // clang-format on
  }
};

}  // namespace editor
}  // namespace librepcb

#endif
"""

SLINT_HEADER = """// GENERATED FILE! DO NOT MODIFY!
// Run dev/generate_editorcommandset.py to re-generate.

import { EditorCommand } from "types.slint";

export global EditorCommandSet {
"""

SLINT_COMMAND_PATTERN = """    in property <EditorCommand> {name}: {{
        icon: @image-url("{icon}"),
        text: "{text}",
        status-tip: "{statustip}",
        shortcut: "{shortcut}",
        modifiers: {{ alt: {alt}, control: {ctrl}, shift: {shift}, meta: {meta} }},
        key: {key},
    }};
"""

SLINT_FOOTER = "}\n"


class StringParser:
    def __init__(self, str):
        self.str = str

    def take_until(self, pattern):
        pos = self.str.find(pattern)
        substr = self.str[:pos]
        self.str = self.str[pos + 1:]
        return substr


class EditorCommand:
    def __init__(self, cxx):
        parser = StringParser(cxx)
        parser.take_until(' ')
        self.variable = parser.take_until('{')
        assert self.variable
        parser.take_until('"')
        self.key = parser.take_until('"')
        assert self.key
        parser.take_until('"')
        self.text_raw = parser.take_until('"')
        assert self.text_raw
        parser.take_until('"')
        self.description = parser.take_until('"')
        assert self.description
        parser.take_until(',')
        self.image_raw = parser.take_until(',')
        assert self.image_raw
        self.flags_raw = parser.take_until(',')
        assert self.flags_raw
        self.key_sequences_raw = parser.take_until(',')
        assert self.key_sequences_raw

    def icon(self):
        s = self.image_raw.replace('QString()', '').replace('"', '')
        if not s.endswith('.svg'):
            return ''
        s = s.replace(':/fa/', '../../../../font-awesome/svgs/')
        s = s.replace(':/bi/', '../../../../bootstrap-icons/icons/')
        s = s.replace(':/img/', '../../../../../img/')
        return s

    def text(self):
        s = self.text_raw.replace('&&', '&')
        if 'OpensPopup' in self.flags_raw:
            s += '...'
        return s

    def keyboard_shortcut(self):
        s = self._key_sequence_0()
        s = s.replace('Qt::Key_', '')
        s = s.replace('Qt::', '')
        s = s.replace('ALT', 'Alt')
        s = s.replace('CTRL', 'Ctrl')
        s = s.replace('SHIFT', 'Shift')
        s = s.replace('META', 'Meta')
        s = s.replace('Plus', '+')
        s = s.replace('Minus', '-')
        s = s.replace('Comma', ',')
        s = s.replace(' | ', '+')
        return s

    def to_cxx(self):
        return CXX_OUT_COMMAND_PATTERN.format(
            slint=self.key,
            cxx=self.variable,
        )

    def to_slint(self):
        shortcut = self._key_sequence_0()
        key = shortcut.replace('Qt::ALT', '')
        key = key.replace('Qt::CTRL', '')
        key = key.replace('Qt::SHIFT', '')
        key = key.replace('Qt::META', '')
        key = key.replace('Qt::Key_', '')
        key = key.replace('|', '')
        key = key.replace(' ', '')
        key = key.replace('Period', '.')
        key = key.replace('Comma', ',')
        key = key.replace('Plus', '+')
        key = key.replace('Minus', '-')
        key = key.replace('Asterisk', '*')
        key = key.replace('Slash', '/')
        key = key.replace('Left', 'LeftArrow')
        key = key.replace('Right', 'RightArrow')
        key = key.replace('Up', 'UpArrow')
        key = key.replace('Down', 'DownArrow')
        key = key.replace('PageUpArrow', 'PageUp')
        key = key.replace('PageDownArrow', 'PageDown')
        if len(key) > 1:
            key = 'Key.{}'.format(key)
        else:
            key = '"{}"'.format(key.lower())
        return SLINT_COMMAND_PATTERN.format(
            icon=self.icon(),
            name=self.key.replace('_', '-'),
            text=self.text(),
            statustip=self.description,
            shortcut=self.keyboard_shortcut(),
            alt='true' if 'ALT' in shortcut else 'false',
            ctrl='true' if 'CTRL' in shortcut else 'false',
            shift='true' if 'SHIFT' in shortcut else 'false',
            meta='true' if 'META' in shortcut else 'false',
            key=key,
        )

    def _key_sequence_0(self):
        s = self.key_sequences_raw
        s = s.replace('{', '')
        s = s.replace('}', '')
        s = s.split(',')[0]
        s = s.replace('QKeySequence(', '')
        s = s.replace(')', '')
        return s


def parse_cxx():
    commands = list()
    current_command = None
    with open(os.path.join(ROOT, CXX_IN), 'r') as f:
        for line in f.readlines():
            line = re.search(r'(.*?)\s*(//.*)?$', line).group(1).strip()
            if not line:
                continue
            # Note: Skip commands starting with underscore.
            if re.match(r'EditorCommand [^_]\w+{', line):
                current_command = line
            elif current_command is not None:
                current_command += line
                if line == '};':
                    commands.append(EditorCommand(current_command))
                    current_command = None
    return commands


def write_file(path, content):
    if '--check' in sys.argv:
        print('Check {}'.format(path))
        with open(os.path.join(ROOT, path), 'r') as f:
            if f.read() != content:
                print('Generated file is outdated, please run the generator!')
                sys.exit(1)
    else:
        print('Writing {}'.format(path))
        with open(os.path.join(ROOT, path), 'w') as f:
            f.write(content)


def write_cxx(commands):
    s = CXX_OUT_HEADER
    s += '\n'.join([cmd.to_cxx() for cmd in commands])
    s += CXX_OUT_FOOTER
    write_file(CXX_OUT, s)


def write_slint(commands):
    s = SLINT_HEADER
    s += '\n'.join([cmd.to_slint() for cmd in commands])
    s += SLINT_FOOTER
    write_file(SLINT, s)


if __name__ == '__main__':
    commands = parse_cxx()
    write_cxx(commands)
    write_slint(commands)
    print('Done')
