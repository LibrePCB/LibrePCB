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

## Author Information

For investigating bug reports, it's useful to know where the application binary
is coming from. Therefore this information can be compiled into the executable
with the `LIBREPCB_BUILD_AUTHOR` parameter. Its value is then shown in the
"About LibrePCB" dialog.

  cmake .. -DLIBREPCB_BUILD_AUTHOR="Flathub Buildbot"

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


## OpenGL Utility Library (GLU) Dependency

The OpenGL Utility Library (GLU) is needed for the 3D viewer of LibrePCB and
thus needs to be available both at built time and runtime. If this library
is not available for a particular platform, it is possible to compile without
it but the 3D viewer won't render all layers then.

    cmake .. -DUSE_GLU=0


# OpenCASCADE Dependency

Parts of the 3D features (e.g. reading/writing STEP files) depend on the
[OpenCASCADE](https://www.opencascade.com/) library, also known as OCCT or OCE.
As this library might not be available on any platform, or might lead to
packaging issues, it is possible to build LibrePCB without these 3D features.
The OpenCASCADE dependency is then not needed, while LibrePCB is still usable
without any issues, just without full 3D support.

    cmake .. -DUSE_OPENCASCADE=0

We recommend using the official OpenCASCADE library (OCCT), but generally
LibrePCB should also work with the Community Edition (OCE). CMake should
automatically detect the availability of both variants.


# QtQuick / Qt Declarative / QML Dependency

Starting with version 1.0, LibrePCB depends on the QtQuick / Qt Declarative
component for evaluation purposes. In future QML might be used for the GUI,
but we first need to do some evaluation and testing. To catch potential issues
as early as possible, the dependency has been added already but is not used
yet for productive features. If this dependency causes any issues during
packaging or during runtime, LibrePCB can currently be built without it:

    cmake .. -DBUILD_QTQUICK_TEST=0

**IMPORTANT**: Please report any problems in our
[issue tracker](https://github.com/LibrePCB/LibrePCB/issues)! Otherwise,
sooner or later the dependency might be mandatory without having a workaround
anymore for your issue!


# Dynamic Linking / Unbundling {#doc_building_unbundling}

By default, all dependencies except Qt and OpenCascade will be linked
statically using vendored git submodules. If you prefer to unbundle some
libraries, set the `UNBUNDLE_xxx` variable:

    cmake .. -DUNBUNDLE_FONTOBENE_QT=1 -DUNBUNDLE_POLYCLIPPING=1

To unbundle all dependencies that support it, use `-DUNBUNDLE_ALL=1`.

Right now, the following libraries can be unbundled:

| Library | Parameter | Search Methods |
|-|-|-|
| [dxflib] | `UNBUNDLE_DXFLIB` | `pkg-config` |
| [fontobene-qt] | `UNBUNDLE_FONTOBENE_QT` | `pkg-config`, `find_path` |
| [googletest] | `UNBUNDLE_GTEST` | `cmake`, `pkg-config` |
| [hoedown] ¹ | `UNBUNDLE_HOEDOWN` | `pkg-config` |
| [muparser] | `UNBUNDLE_MUPARSER` | `cmake`, `pkg-config` |
| [polyclipping] | `UNBUNDLE_POLYCLIPPING` | `pkg-config` |
| [quazip] ² | `UNBUNDLE_QUAZIP` | `cmake` |

[dxflib]: https://www.qcad.org/en/90-dxflib
[fontobene-qt]: https://github.com/fontobene/fontobene-qt/
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
