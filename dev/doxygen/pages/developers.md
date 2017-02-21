General Information for Developers {#doc_developers}
====================================================

[TOC]

This page contains some general information for LibrePCB developers. Please also read our
contribution guidelines: https://github.com/LibrePCB/LibrePCB/blob/master/CONTRIBUTING.md


# Git {#doc_developers_git}

- The `master` branch must always be stable (CI successful).
- Use separate branches to fix bugs or implement new features.
- Do not pack lot of changes in one commit. Make small, incremental commits instead.
- Write commit messages according this guideline: http://chris.beams.io/posts/git-commit/


# Internationalization (i18n) {#doc_developers_i18n}

- All files (\*.cpp, \*.h, \*.md, \*.xml, \*.ini,...) must be written in english. Only strings which
  are visible in the GUI should be translatable into other languages.
- Always use international date/time formats in files:
    - Use the [ISO 8601] format for date/time in all files.
    - Use UTC time in all files, e.g. `2014-12-20T14:42:30Z`.
    - In the GUI, you should normally use the local date/time format.
    - See also https://en.wikipedia.org/wiki/ISO_8601 and http://doc.qt.io/qt-5/qdatetime.html
- All numbers which are stored in files (e.g. XML or INI) must have a locale-independent format. We
  always use the point '.' as decimal separator, and no thousands separator. Example: `123456789.987654321`


# Licenses {#doc_developers_licenses}

- All 3rd-party modules and source code (e.g. from the internet) must be compatible with the GNU GPLv3
  license. This applies to all kinds of resources (images, symbols, text, sound, source code, ...).


# Exceptions {#doc_developers_exceptions}

- Use always our own exception types (librepcb::Exception and derivated classes), see [exceptions.h].
- Never use other exception types (like `std::exception`).
- Exceptions from 3rd-party libraries must be translated into our own exceptions in a wrapper class.


# Deployment {#doc_developers_deployment}

- Where to put our files on UNIX/Linux?: http://unix.stackexchange.com/questions/114407/deploying-my-application


[ISO 8601]: https://en.wikipedia.org/wiki/ISO_8601 "ISO 8601"
[exceptions.h]: @ref exceptions.h
