General Information for Developers {#doc_developers}
====================================================

[TOC]

This page contains some general information for LibrePCB developers. Please also read our
contribution guidelines: https://github.com/LibrePCB/LibrePCB/blob/master/CONTRIBUTING.md


# Unstable Version {#doc_developers_unstable_versions}

As explained in the @ref doc_release_workflow, the `master` branch is always
unstable. And unstable doesn't mean "If you have bad luck, something doesn't
work as expected". It rather means "Everything you save with an unstable
version can't be opened again with any other (stable or unstable) version!!!".
So it's highly recommended to use a dedicated workspace for testing purposes,
then the files in your productive workspace are safe. When working with real
projects or libraries, make a backup first or at least use a version control
system so any modifications can be reverted afterwards. This can also be achieved
by setting the environment variable `LIBREPCB_WORKSPACE` to the workspace
you want to use. If you use QtCreator, this variable can be set in the run
configuration, so that this workspace is only used when starting LibrePCB from
within QtCreator.

Unstable application versions show a warning message box on every startup to
protect users from (accidentally) beaking their files (for example if they are
trying out a nightly build). For developers this message box is very annoying,
so you can set the environment variable `LIBREPCB_DISABLE_UNSTABLE_WARNING=1` to
disable it. This variable can also be set in the run configuration, so the
warning is only disabled when starting LibrePCB from within QtCreator, similar
to the one above. But be careful with the disabled warning! :)


# Git {#doc_developers_git}

- The `master` branch must always compile and all tests have to pass (CI successful).
- Use separate branches to fix bugs or implement new features.
- Do not pack lot of changes in one commit. Make small, incremental commits instead.
- Write commit messages according this guideline: http://chris.beams.io/posts/git-commit/


# Pull Requests {#doc_developers_pullrequests}

To keep our Git history clear, simple and consistent, we use the following
strategy for working with branches and GitHub Pull Requests:

- If a branch is outdated (i.e. commits were added to `master` in the mean
  time), the branch should be updated by **rebasing** it. Please do not update
  branches by merging `master` into it (as the "Update Branch" button on the
  GitHub UI unfortunately does).
- When adding fixes or other new changes to a branch, the already pushed
  commits should be updated accordingly (e.g. modified or amended) and then
  force-pushed instead of adding new commits.

Generally it's fine to deviate from these rules as long as the branch gets
updated and cleaned up before it is ready to merge. But keep in mind that
LibrePCB maintainers might not do a detailed review before it is cleaned up,
since reviewing a clean Git history is easier.

If you are not familiar enough with Git to do the rebasing operations - no
problem! LibrePCB maintainers can help you in several ways:

- Explain you how to do the rebase operations, force-pushes etc.
- Take over your branch and do the rebase on our own, if you agree.
- For smaller changes, we might simply
  [squash-merge](https://github.blog/2016-04-01-squash-your-commits/)
  a Pull Request.


# Internationalization (i18n) {#doc_developers_i18n}

- All files (\*.cpp, \*.h, \*.md, \*.lp, \*.ini,...) must be written in english. Only strings which
  are visible in the GUI should be translatable into other languages.
- Always use international date/time formats in files:
    - Use the [ISO 8601] format for date/time in all files.
    - Use UTC time in all files, e.g. `2014-12-20T14:42:30Z`.
    - In the GUI, you should normally use the local date/time format.
    - See also https://en.wikipedia.org/wiki/ISO_8601 and http://doc.qt.io/qt-5/qdatetime.html
- All numbers which are stored in files (e.g. S-Expressions or INI) must have a
  locale-independent format. We always use the point '.' as decimal separator,
  and no thousands separator. Example: `123456789.987654321`


# Licenses {#doc_developers_licenses}

- All 3rd-party modules and source code (e.g. from the internet) must be compatible with the GNU GPLv3
  license. This applies to all kinds of resources (images, symbols, text, sound, source code, ...).


# Deployment {#doc_developers_deployment}

- Where to put our files on UNIX/Linux?: http://unix.stackexchange.com/questions/114407/deploying-my-application


[ISO 8601]: https://en.wikipedia.org/wiki/ISO_8601 "ISO 8601"
[exceptions.h]: @ref exceptions.h
