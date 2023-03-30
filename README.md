# LibrePCB

[![Become a Patron](https://img.shields.io/badge/Patreon-donate-orange.svg)](https://www.patreon.com/librepcb)
[![Discourse](https://img.shields.io/badge/Discourse-discuss-blueviolet.svg)](https://librepcb.discourse.group/)
[![Telegram](https://img.shields.io/badge/Telegram-chat-blue.svg)](https://telegram.me/LibrePCB_dev)
[![Website](https://img.shields.io/badge/Website-librepcb.org-29d682.svg)](https://librepcb.org/)
[![Docs](https://img.shields.io/badge/Docs-read-yellow.svg)](https://librepcb.org/docs/)

[![Azure Build Status](https://dev.azure.com/LibrePCB/LibrePCB/_apis/build/status/LibrePCB.LibrePCB?branchName=master)](https://dev.azure.com/LibrePCB/LibrePCB/_build/latest?definitionId=2&branchName=master)
[![OpenSSF Best Practices](https://bestpractices.coreinfrastructure.org/projects/1652/badge)](https://bestpractices.coreinfrastructure.org/projects/1652)

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

To compile LibrePCB, you need the following software components:

- g++ >= 4.8, MinGW >= 4.8, or Clang >= 3.3 (C++11 support is required)
- [Qt](http://www.qt.io/download-open-source/) >= 5.5
- [zlib](http://www.zlib.net/)
- [OpenSSL](https://www.openssl.org/)
- [CMake](https://cmake.org/) 3.5 or newer

#### Prepared Docker Image

Instead of installing the dependencies manually on your system (see instructions
below), you can also use one of our
[Docker images](https://hub.docker.com/r/librepcb/librepcb-dev) with all
dependencies pre-installed (except GUI tools like QtCreator). These images are
actually used for CI, but are also useful to build LibrePCB locally.

#### Installation on Debian/Ubuntu/Mint

*Note: For Ubuntu older than 22.04, replace `qtbase5-dev` by `qt5-default`.*

```bash
sudo apt-get install git build-essential qtbase5-dev qttools5-dev-tools qttools5-dev \
     libglu1-mesa-dev openssl zlib1g zlib1g-dev libqt5opengl5-dev libqt5svg5-dev cmake
sudo apt-get install qt5-doc qtcreator # optional
```

#### Installation on Arch Linux

```bash
sudo pacman -S git base-devel qt5-base qt5-svg qt5-tools desktop-file-utils shared-mime-info \
     openssl zlib cmake
sudo pacman -S qt5-doc qtcreator # optional
```

*Note: Instead of installing the dependencies and building LibrePCB manually,
you could install the package
[librepcb-git](https://aur.archlinux.org/packages/librepcb-git/) from the AUR.
The package clones and builds the latest version of the `master` branch from
GitHub.*

#### Installation on Mac OS X

1. Install Xcode through the app store and start it at least once (for the license)
2. Install [homebrew](https://github.com/Homebrew/brew) (**the** package manager)
3. Install *qt5* and *cmake*: `brew update && brew install qt5 cmake`
4. Make the toolchain available: `brew unlink qt && brew link --force qt5`

#### Installation on Windows

Download and run the
[Qt for Windows (MinGW) installer](http://download.qt.io/official_releases/qt/5.8/5.8.0/qt-opensource-windows-x86-mingw530-5.8.0.exe)
from [here](https://www.qt.io/download-open-source/). LibrePCB does not compile
with MSVC, so you must install following components with the Qt installer:

- The MinGW compiler itself
- The Qt libraries for MinGW
- cmake

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
