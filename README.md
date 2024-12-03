# LibrePCB

[![Become a Patron](https://img.shields.io/badge/Patreon-donate-orange.svg)](https://www.patreon.com/librepcb)
[![Discourse](https://img.shields.io/badge/Discourse-discuss-blueviolet.svg)](https://librepcb.discourse.group/)
[![Telegram](https://img.shields.io/badge/Telegram-chat-blue.svg)](https://telegram.me/LibrePCB_dev)
[![Website](https://img.shields.io/badge/Website-librepcb.org-29d682.svg)](https://librepcb.org/)
[![Docs](https://img.shields.io/badge/Docs-read-yellow.svg)](https://librepcb.org/docs/)

[![Azure Build Status](https://dev.azure.com/LibrePCB/LibrePCB/_apis/build/status/LibrePCB.LibrePCB?branchName=master)](https://dev.azure.com/LibrePCB/LibrePCB/_build/latest?definitionId=2&branchName=master)
[![OpenSSF Best Practices](https://bestpractices.coreinfrastructure.org/projects/1652/badge)](https://bestpractices.coreinfrastructure.org/projects/1652)

[![Packaging status](https://repology.org/badge/vertical-allrepos/librepcb.svg?columns=3&header=LibrePCB)](https://repology.org/project/librepcb/versions)

## About LibrePCB

LibrePCB is a free
[EDA](https://en.wikipedia.org/wiki/Electronic_design_automation) suite to
develop printed circuit boards on Windows, Linux and MacOS. More information
and screenshots are available at [librepcb.org](https://librepcb.org).

## Installation & Usage

**Official stable releases are provided at our
[download page](https://librepcb.org/download/).**

**Please read our [user manual](https://librepcb.org/docs/) to see how you can
install and use LibrePCB.** The
[quickstart tutorial](https://librepcb.org/docs/quickstart/) provides a
step-by-step guide through the whole process of designing a PCB.

## Contributing

Contributions are welcome! See
[librepcb.org/contribute](https://librepcb.org/contribute/) and
[`CONTRIBUTING.md`](CONTRIBUTING.md) for details.

For internal details take a look at the
[developers documentation](https://developers.librepcb.org/).

## Development

***WARNING: The `master` branch always contains the latest UNSTABLE version of
LibrePCB. Everything you do with this unstable version could break your
workspace, libraries or projects, so you should not use it productively! For
productive use, please install an official release as described in the
[user manual](https://librepcb.org/docs/). For development, please read details
[here](https://developers.librepcb.org/df/d30/doc_developers.html#doc_developers_unstable_versions).***

### Requirements

To compile and run LibrePCB, you need the following software components:

- Compiler: g++, MinGW or Clang (any version with C++20 support should work)
- [Qt](http://www.qt.io/download-open-source/) >= 6.2 (make
  sure the [imageformats](https://doc.qt.io/qt-6/qtimageformats-index.html)
  plugin is installed too as it will be needed at runtime!).
- [OpenCASCADE](https://www.opencascade.com/) OCCT or OCE (optional,
  OCCT highly preferred)
- [OpenGL Utility Library](https://en.wikipedia.org/wiki/OpenGL_Utility_Library)
  GLU (optional)
- [zlib](http://www.zlib.net/)
- [OpenSSL](https://www.openssl.org/)
- [CMake](https://cmake.org/) 3.16 or newer

#### Prepared Docker Image

Instead of installing the dependencies manually on your system (see instructions
below), you can also use one of our
[Docker images](https://hub.docker.com/r/librepcb/librepcb-dev) with all
dependencies pre-installed (except GUI tools like QtCreator). These images are
actually used for CI, but are also useful to build LibrePCB locally.

#### Installation on Debian/Ubuntu/Mint

##### Ubuntu >= 22.04

```bash
sudo apt-get install build-essential git cmake openssl zlib1g zlib1g-dev \
     qt6-base-dev qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools \
     libqt6core5compat6-dev qt6-declarative-dev libqt6opengl6-dev libqt6svg6-dev \
     qt6-image-formats-plugins libglu1-mesa-dev libtbb-dev libxi-dev \
     occt-misc libocct-*-dev
sudo apt-get install qtcreator # optional
```

##### Ubuntu 20.04

```bash
sudo apt-get install build-essential git cmake openssl zlib1g zlib1g-dev \
     qt5-default qtdeclarative5-dev qttools5-dev-tools qttools5-dev \
     qtquickcontrols2-5-dev libqt5opengl5-dev libqt5svg5-dev \
     qt5-image-formats-plugins libglu1-mesa-dev liboce-*-dev
sudo apt-get install qt5-doc qtcreator # optional
```

#### Installation on Arch Linux

```bash
sudo pacman -S base-devel git cmake openssl zlib desktop-file-utils shared-mime-info \
     qt6-base qt6-5compat qt6-declarative qt6-svg qt6-tools qt6-imageformats opencascade
sudo pacman -S qt6-doc qtcreator # optional
```

*Note: Instead of installing the dependencies and building LibrePCB manually,
you could install the package
[librepcb-git](https://aur.archlinux.org/packages/librepcb-git/) from the AUR.
The package clones and builds the latest version of the `master` branch from
GitHub.*

#### Installation on Mac OS X

1. Install Xcode through the app store and start it at least once (for the license)
2. Install [homebrew](https://github.com/Homebrew/brew) (**the** package manager)
3. Install dependencies: `brew update && brew install qt6 cmake opencascade`
4. Make the toolchain available: `brew unlink qt && brew link --force qt6`

#### Installation on Windows

Download and run the
[Qt for Windows installer](https://download.qt.io/official_releases/online_installers/qt-unified-windows-x64-online.exe)
from [here](https://www.qt.io/download-open-source/). LibrePCB does not compile
with MSVC, so you must install following components with the Qt installer:

- MinGW 11.2.0 64-bit compiler
- Qt binaries for MinGW 11.2.0 64-bit (use the latest 6.x version)
- Qt 5 Compatibility Module for MinGW 11.2.0 64-bit
- Qt Image Formats for MinGW 11.2.0 64-bit
- CMake

For the OpenCascade library the installation procedure is not that easy
unfortunately. Basically you have to build it by yourself, see instructions
[here](https://dev.opencascade.org/doc/overview/html/build_upgrade__building_occt.html).
However, to avoid this effort you could instead just set the CMake option
`USE_OPENCASCADE=0` (can be set in the QtCreator build config) to allow
compiling LibrePCB without OpenCascade.

### Cloning

It's important to clone the repository recursively to get all submodules too:

```bash
git clone --recursive https://github.com/LibrePCB/LibrePCB.git && cd LibrePCB
```

### Updating

When updating the repository, make sure to also update all the submodules
recursively. Otherwise you may get strange compilation errors:

```bash
git submodule update --init --recursive
```

### Building

You can either build LibrePCB using Qt Creator, or you can build on the command
line using cmake. To build LibrePCB using cmake/make:

```bash
mkdir build && cd build
cmake ..
make -j8
```

The binary can then be found in `build/apps/librepcb/`.

For more detailed instructions (including how to set up Qt Creator), see
https://developers.librepcb.org/d5/d96/doc_building.html

## Credits

- First of all, many thanks to all of our [contributors](AUTHORS.md)!
- A big thank you goes to all [our sponsors](https://librepcb.org/sponsors/)
  which help to keep this project alive!
- Special thanks also to [cloudscale.ch](https://www.cloudscale.ch/)
  for sponsoring our API server!

## License

LibrePCB is published under the
[GNU GPLv3](http://www.gnu.org/licenses/gpl-3.0.html) license.
