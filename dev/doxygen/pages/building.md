Building LibrePCB {#doc_building}
=================================

[TOC]

This page is aimed both at developers as well as packagers. If you're a
packager and have any question regarding the LibrePCB build system, please join
our [chat or forum](https://librepcb.org/discuss/)!


# Building with Qt Creator {#doc_building_qtcreator}

If you want to contribute a new feature or a bugfix to LibrePCB, the best way
is to use [Qt Creator](https://doc.qt.io/qtcreator/). To get started, simply
open the `librepcb.pro` file with Qt Creator.

## Configuring the Desktop Kit

When opening a project in Qt Creator for the first time, you need to configure
the Desktop kit:

![qtcreator_desktopkit](qtcreator_desktopkit.png)

Click on the "Configure Project" button to get started.

## Starting LibrePCB

Select the run configuration `librepcb` and click on the `Run` button:

![qtcreator_run](qtcreator_run.png)

## Parallel Builds

To keep build time as low as possible make sure to set the correct make flags
to use all available CPU cores to build. See this [stackoverflow
answer](https://stackoverflow.com/questions/8860712/setting-default-make-options-for-qt-creator).


# Building on the Command Line {#doc_building_command_line}

You can also build LibrePCB from the command line using qmake:

    mkdir build && cd build
    qmake -r ../librepcb.pro
    make -j8

Then run the `librepcb` binary in the `build/output/` directory.

To install LibrePCB on your system, run `make install`.

## Faster Rebuilds with ccache

If you regularly rebuild parts of LibrePCB, you can speed up that process by
installing and enabling [ccache](https://ccache.dev/) (this will work on Qt
5.9.2 and newer):

    qmake -r ../librepcb.pro CONFIG+=ccache

## Changing the Install Prefix

By passing in the `PREFIX` parameter, you can change the install prefix. For
example, if you want to install LibrePCB to `/usr/local`, use

    qmake -r ../librepcb.pro PREFIX=/usr/local

## Debug and Release Builds

By default qmake will create a release build without debug symbols.

To create a debug build, pass the `debug` config option:

    qmake -r ../librepcb.pro CONFIG+=debug

To explicitly create a release build:

    qmake -r ../librepcb.pro CONFIG+=release

If you want to create a release build that still contains debug info:

    qmake -r ../librepcb.pro CONFIG+=release CONFIG+=force_debug_info


# Dynamic Linking / Unbundling {#doc_building_unbundling}

By default, all binaries will be linked statically using vendored libraries. If
you prefer to unbundle some libraries, set the `UNBUNDLE` variable:

    qmake -r ../librepcb.pro UNBUNDLE+=quazip

You can either list the libraries one by one, or you can use `UNBUNDLE=all` to
unbundle all libraries that support dynamic linking.

Right now, the following libraries can be unbundled:

* `quazip`
* `fontobene-qt5`

Note: Unbundling is currently only supported on Unix systems with `pkg-config`
installed and where the library system packages provide the corresponding
pkg-config files.
