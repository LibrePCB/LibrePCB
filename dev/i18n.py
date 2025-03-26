#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Collects all translatable strings in source files and writes them to
.tx/librepcb.ts, ready to be uploaded to Transifex (to be done manually
with the Transifex client).

In addition, it updates special generated files in the i18n/ directory from
the translated strings available in the i18n/*.ts files.

Recommended to be run using the Docker image docker.io/librepcb/librepcb-dev
as it contains all required tools.
"""

import configparser
import io
import os
import sys
from defusedxml import ElementTree
from subprocess import check_output as run
from xml.sax.saxutils import escape as escape_xml

REPO_DIR = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
I18N_DIR = os.path.join(REPO_DIR, 'i18n')
SOURCES_FILE = '.tx/sources.txt'
UI_PO_FILE = '.tx/librepcb_ui.po'
TS_FILE = '.tx/librepcb.ts'


def get_desktop_files():
    return run(['git', 'ls-files', '--', ':/share/applications/*.desktop'],
               cwd=REPO_DIR).decode("utf-8").splitlines()


def po_str_to_qs(s):
    s = s.replace('%', '%%')
    s = s.replace('{n}', '%n')
    index = 1
    while '{}' in s:
        s = s.replace('{}', '{{{}}}'.format(index - 1), 1)
        index += 1
    index = 0
    while '{}'.format(index) in s:
        s = s.replace('{{{}}}'.format(index), '%{}'.format(index + 1))
        index += 1
    return s


def update():
    sources = []

    # Prepare *.desktop.i18n files
    for desktop_file in get_desktop_files():
        print("Processing desktop file {}...".format(desktop_file))
        cfg = configparser.ConfigParser()
        cfg.read(os.path.join(REPO_DIR, desktop_file))
        s = ""
        for key in ['Comment', 'Description', 'GenericName']:
            if key in cfg['Desktop Entry']:
                s += 'QT_TRANSLATE_NOOP3("{}", "{}", "{}");\n'.format(
                    desktop_file.split('/')[-1],
                    cfg['Desktop Entry'][key],
                    '{} key of *.desktop file'.format(key),
                )
        if len(s) > 0:
            desktop_file += '.i18n'
            print(" => Translations written to {}".format(desktop_file))
            with open(os.path.join(REPO_DIR, desktop_file), 'w') as f:
                f.write(s)
            sources.append(desktop_file)

    # Create *.ts file of C++/Qt sources
    print("Writing lupdate input to {}...".format(SOURCES_FILE))
    sources += run(['git', 'ls-files', '--', '*.cpp', '*.h', '*.ui'],
                   cwd=REPO_DIR).decode("utf-8").splitlines()
    with open(os.path.join(REPO_DIR, SOURCES_FILE), 'w') as f:
        f.write('\n'.join(sources))
    print("Collected {} source files, exporting translations to {}..."
          .format(len(sources), TS_FILE))
    output = run(['lupdate', '-no-obsolete', '-locations', 'absolute',
                  '-source-language', 'en', '-target-language', 'en',
                  '@' + SOURCES_FILE, '-ts', TS_FILE],
                 cwd=REPO_DIR)
    for line in output.decode("utf-8").splitlines():
        print(' lupdate: ' + line)

    # Create *.po file for Slint sources
    sources = run(['git', 'ls-files', '--', '*.slint'],
                  cwd=REPO_DIR).decode("utf-8").splitlines()
    print("Collected {} Slint files, exporting translations to {}..."
          .format(len(sources), UI_PO_FILE))
    output = run(['slint-tr-extractor', '-o', UI_PO_FILE] + sources,
                 cwd=REPO_DIR)
    for line in output.decode("utf-8").splitlines():
        print(' slint-tr-extractor: ' + line)

    # Parse Slint strings
    slint_strings = dict()
    with open(os.path.join(REPO_DIR, UI_PO_FILE), 'r') as f:
        source = None
        key = None
        values = dict()
        for line in f.readlines():
            line = line.strip()
            if line.startswith('#: '):
                source = line.replace('#: ', '')
                values = dict()
            elif line.startswith('msgctxt "'):
                key = 'msgctxt'
                values[key] = line.replace('msgctxt "', '')[:-1]
            elif line.startswith('msgid "'):
                key = 'msgid'
                values[key] = line.replace('msgid "', '')[:-1]
            elif line.startswith('"') and key is not None:
                values[key] += line[1:-1]
            elif source and key and values:
                msgctxt = values['msgctxt']
                msgid = values['msgid']
                if msgctxt not in slint_strings:
                    slint_strings[msgctxt] = list()
                slint_strings[msgctxt].append((source, msgid))
                source = None
                key = None
                values = dict()
    total_count = sum([len(ctx) for ctx in slint_strings])
    print("Found {} translations in {} Slint sources".format(
        total_count, len(slint_strings)))
    assert total_count > 0

    # Append Slint strings to librepcb.ts
    print("Merging Slint translations into {}...".format(TS_FILE))
    with open(os.path.join(REPO_DIR, TS_FILE), 'r') as f:
        ts = f.readlines()
    assert ts[-1] == '</TS>\n'
    for context, strings in sorted(slint_strings.items()):
        ts.insert(-1, '<context>\n')
        ts.insert(-1, '    <name>ui::{}</name>\n'.format(context))
        for source, string in sorted(strings):
            ts.insert(-1, '    <message>\n')
            ts.insert(-1, '        <location filename="{}" line="{}"/>\n'
                          .format(*source.split(':')))
            ts.insert(-1, '        <source>{}</source>\n'
                          .format(escape_xml(po_str_to_qs(string))))
            ts.insert(-1, '        <translation type="{}"></translation>\n'
                          .format('unfinished'))
            ts.insert(-1, '    </message>\n')
        ts.insert(-1, '</context>\n')
    with open(os.path.join(REPO_DIR, TS_FILE), 'w') as f:
        f.write(''.join(ts)
                .replace('filename="../', 'filename="')
                .replace('\r', ''))

    # Check validity of *.ts file
    print("Validating {}...".format(TS_FILE))
    run(['lconvert', '-i', TS_FILE], cwd=REPO_DIR)
    print("Update Done")


def get_desktop_translations(context):
    result = {}
    files = run(['git', 'ls-files', '--', ':/librepcb_*.ts'],
                cwd=I18N_DIR).decode("utf-8").splitlines()
    for ts_file in files:
        root = ElementTree.parse(os.path.join(I18N_DIR, ts_file))
        lang = list(root.iter('TS'))[0].get('language')
        result[lang] = {}
        for ctx in root.iter('context'):
            if ctx.findtext('name') == context:
                for m in ctx.iter('message'):
                    source = m.findtext("source")
                    translation = m.findtext("translation")
                    if translation:
                        result[lang][source] = translation
    return result


def sanitize_desktop_str(s):
    s = s.replace('\r', '').replace('\t', '').splitlines()[0]
    return ''.join([c for c in s if c.isprintable()])


def translate():
    for desktop_file in get_desktop_files():
        print("Translating desktop file {}...".format(desktop_file))
        context = desktop_file.split('/')[-1]
        translations = get_desktop_translations(context)
        cfg = configparser.ConfigParser(strict=False, interpolation=None)
        cfg.optionxform = lambda option: option
        cfg.read(os.path.join(REPO_DIR, desktop_file))
        ini = cfg['Desktop Entry']
        for key in ['Comment', 'Description', 'GenericName']:
            if key in ini:
                for lang, strings in translations.items():
                    source = ini[key]
                    if source in strings:
                        ini['{}[{}]'.format(key, lang)] = \
                            sanitize_desktop_str(strings[source])
        outfile = os.path.join(I18N_DIR, desktop_file)
        os.makedirs(os.path.dirname(outfile), exist_ok=True)
        outstream = io.StringIO()
        cfg.write(outstream, space_around_delimiters=False)
        with open(outfile, 'w') as f:
            f.write(outstream.getvalue().strip() + "\n")
        print(" => Written to {}".format(
            os.path.relpath(outfile, REPO_DIR)))
    print("Translations Done")


def check(actual, expected):
    assert actual == expected, '"{}" != "{}"'.format(actual, expected)


if __name__ == '__main__':
    # Unit tests ;-)
    check(po_str_to_qs(''), '')
    check(po_str_to_qs('Foo {}'), 'Foo %1')
    check(po_str_to_qs('Foo {} Bar {} {}'), 'Foo %1 Bar %2 %3')
    check(po_str_to_qs('Foo {2} Bar {1} {0}'), 'Foo %3 Bar %2 %1')
    check(po_str_to_qs('Foo {1} Bar {n} {0}'), 'Foo %2 Bar %n %1')
    check(po_str_to_qs('Foo {} Bar {n} {}'), 'Foo %1 Bar %n %2')
    check(po_str_to_qs('Foo % Bar {}'), 'Foo %% Bar %1')

    handled = False
    if '--update' in sys.argv:
        update()
        handled = True
    if '--translate' in sys.argv:
        translate()
        handled = True

    if not handled:
        print(f'Usage: {sys.argv[0]} [--update] [--translate]')
        print()
        print('Options:')
        print('  --update      Generate .tx/librepcb.ts source file')
        print('  --translate   Update special output files in i18n/ directory')
        sys.exit(1)
