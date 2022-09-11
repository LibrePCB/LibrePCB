Building LibrePCB {#doc_building}
=================================

[TOC]

This page is aimed both at developers as well as packagers. If you're a
packager and have any question regarding the LibrePCB build system, please join
our [chat or forum](https://librepcb.org/discuss/)!


# Building with Qt Creator {#doc_building_qtcreator}

If you want to contribute a new feature or a bugfix to LibrePCB, the best way
is to use [Qt Creator](https://doc.qt.io/qtcreator/). To get started, simply
open the top-level `CMakeLists.txt` file with Qt Creator.

## Configuring the Desktop Kit

When opening a project in Qt Creator for the first time, you need to configure
the Desktop kit:

![qtcreator_desktopkit](qtcreator_desktopkit.png)

Click on the "Configure Project" button to get started.

## Starting LibrePCB

Select the run configuration `librepcb` and click on the `Run` button:

![qtcreator_run](qtcreator_run.png)

## Parallel Builds

If [ninja](https://ninja-build.org/) is installed on your system, Qt Creator
will use it by default, resulting in parallel builds (utilizing all your CPU
cores) out of the box. If the fallback to GNU Make is being used, you may need
to [set make flags](https://stackoverflow.com/questions/8860712/setting-default-make-options-for-qt-creator)
in order to achieve parallel builds.


# Building on the Command Line {#doc_building_command_line}

You can also build LibrePCB from the command line using cmake:

    mkdir build && cd build
    cmake ..
    make -j8

Then run the `librepcb` binary in the `build/apps/librepcb/` directory.

To install LibrePCB on your system, run `make install`.

## Faster Rebuilds with ccache

If you regularly rebuild parts of LibrePCB, you can speed up that process by
installing [ccache](https://ccache.dev/) and passing
`-DCMAKE_CXX_COMPILER_LAUNCHER=ccache` to CMake.

    cmake .. -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

## Changing the Install Prefix

By passing in the `CMAKE_INSTALL_PREFIX` parameter, you can change the install
prefix. For example, if you want to install LibrePCB to `/usr/local`, use

    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local

## Debug and Release Builds

By default CMake will create a release build without debug symbols.

To customize the build type, pass the `CMAKE_BUILD_TYPE` parameter to CMake.
For example, to create a debug build:

    cmake .. -DCMAKE_BUILD_TYPE=Debug

Available build types:

- `Debug`: Debug build
- `Release`: Standard release build
- `RelWithDebInfo`: Release build with debug symbols
- `MinSizeRel`: Release build optimized for minimal size

## Compiler Warnings

By default, our own libraries and applications are compiled with `-Wall`. If
you want to treat warnings as errors (advisable in CI, or before committing),
pass the `BUILD_DISALLOW_WARNINGS` parameter to CMake:

  cmake .. -DBUILD_DISALLOW_WARNINGS=1

## Shared Resources Path

The share directory contains icons, fonts, templates and more resources that
LibrePCB needs at runtime. On a Linux system, it is usually located at
`/usr/share/librepcb`.

By default (unless `LIBREPCB_REPRODUCIBLE` is set), the build process will
embed the absolute path to the `share` directory located within the source tree
into the binary. When starting LibrePCB, it will first check whether the binary
is running from within `CMAKE_BINARY_DIR`. If that is the case, the `share`
directory in the source tree will be used. Otherwise, LibrePCB will fall back
to `../share/librepcb` relative to the application binary.

This will work out-of-the-box in many cases. However, when packaging LibrePCB,
you should explicitly set the share directory path (either an absolute or a
relative path).

    cmake .. -DLIBREPCB_SHARE=/usr/share/librepcb

## Reproducible Builds

So far, LibrePCB does not officially support reproducible builds. However, we
allow turning off some features that make the binary non-reproducible (like
embedded source directory paths, or embedded git commit hashes). This should
make it easier to achieve reproducible builds in the future.

    cmake .. -DLIBREPCB_REPRODUCIBLE=1

# Dynamic Linking / Unbundling {#doc_building_unbundling}

By default, all dependencies (except for Qt) will be linked statically using
vendored git submodules. If you prefer to unbundle some libraries, set the
`UNBUNDLE_xxx` variable:

    cmake .. -DUNBUNDLE_FONTOBENE_QT5=1 -DUNBUNDLE_POLYCLIPPING=1

To unbundle all dependencies that support it, use `-DUNBUNDLE_ALL=1`.

Right now, the following libraries can be unbundled:

| Library | Parameter | Search Methods |
|-|-|-|
| [dxflib] | `UNBUNDLE_DXFLIB` | `pkg-config` |
| [fontobene-qt5] | `UNBUNDLE_FONTOBENE_QT5` | `pkg-config`, `find_path` |
| [googletest] | `UNBUNDLE_GTEST` | `cmake`, `pkg-config` |
| [hoedown] ¹ | `UNBUNDLE_HOEDOWN` | `pkg-config` |
| [muparser] | `UNBUNDLE_MUPARSER` | `cmake` |
| [polyclipping] | `UNBUNDLE_POLYCLIPPING` | `pkg-config` |
| [quazip] ² | `UNBUNDLE_QUAZIP` | `cmake` |

[dxflib]: https://www.qcad.org/en/90-dxflib
[fontobene-qt5]: https://github.com/fontobene/fontobene-qt5/
[googletest]: https://github.com/google/googletest
[hoedown]: https://github.com/hoedown/hoedown
[muparser]: https://github.com/beltoforion/muparser
[polyclipping]: https://sourceforge.net/projects/polyclipping/
[quazip]: https://github.com/stachenov/quazip

¹ Note: Hoedown is only needed on Qt <5.14.<br>
² Due to packaging issues with QuaZip 0.x, we only support QuaZip 1.x when unbundling.
  Using QuaZip 0.9 should work as well, but then you'll have to patch the
  `cmake/FindQuaZip.cmake` find script as well as potentially the include paths
  (quazip -> quazip5) yourself.
