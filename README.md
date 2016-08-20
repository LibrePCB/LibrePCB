# LibrePCB

[![Travis Build Status](https://travis-ci.org/LibrePCB/LibrePCB.svg?branch=master)](https://travis-ci.org/LibrePCB/LibrePCB)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/3npw66djux4kv82f/branch/master?svg=true)](https://ci.appveyor.com/project/ubruhin/librepcb/branch/master)
[![Flattr this project](https://img.shields.io/badge/flattr-donate-yellow.svg)](https://flattr.com/submit/auto?user_id=LibrePCB&url=https://github.com/LibrePCB/LibrePCB&title=LibrePCB&language=&tags=github&category=software)
[![Donate with Bitcoin](https://img.shields.io/badge/bitcoin-donate-yellow.svg)](https://blockchain.info/address/1FiXZxoXe3px1nNuNygRb1NwcYr6U8AvG8)

## About LibrePCB

LibrePCB is a free Schematic / Layout Editor to develop printed circuit boards.
It runs on Linux, Windows and Mac. The project is still in a quite early
development stage (no stable release available).

![Screenshot](doc/screenshot.png)

### Features
- Cross-platform (UNIX/Linux, Mac OS X, Windows)
- Multilingual (application and library elements)
- All-In-One: project management + schematic editor + board editor
- Intuitive, modern and easy-to-use schematic/board editors
- Very powerful library design
- Uses XML as primary file format (allows version control of libraries and projects)
- Multi-PCB feature (different PCB variants of the same schematic)
- Automatic netlist synchronisation between schematic and PCB (no manual export/import)
- Up to 100 copper layers :-)

### Planned Features
- Integrated 3D PCB viewer + 3D file export
- Automatic SPICE netlist export
- Reverse engineering mode (from PCB to schematic)


## Development

### Requirements

To compile LibrePCB, you need the following software components:
- g++ >= 4.8, MinGW >= 4.8, or Clang >= 3.3 (C++11 support is required)
- Qt >= 5.2 (http://www.qt.io/download-open-source/)
- libglu1-mesa-dev (`sudo apt-get install libglu1-mesa-dev`)

#### Installation on Ubuntu 14.04 and later

```bash
sudo apt-get install g++ qt5-default qttools5-dev-tools qt5-doc qtcreator libglu1-mesa-dev
```

#### Installation on ArchLinux

You can install [librepcb-git](https://aur.archlinux.org/packages/librepcb-git/) from the AUR.

#### Installation in a docker container

To build and run LibrePCB in a [docker](https://www.docker.com/) container (which is pretty cool!), check out [these instructions](https://github.com/LibrePCB/LibrePCB/tree/master/dev/docker).

### Building

#### Using Qt Creator

Building with [qtcreator](http://doc.qt.io/qtcreator/) is probably the easiest
way.  To keep build time as low as possible make sure to set the correct make
flags to use all available CPU cores to build. See this [stackoverflow
answer](https://stackoverflow.com/questions/8860712/setting-default-make-options-for-qt-creator).

#### Using qmake and make

Since Qt Creator is also using qmake and make to build, it's easy to do the same
on the command line:

```bash
mkdir build && cd build
qmake -r ../librepcb.pro
make -j 8
```

### Run LibrePCB

#### From Qt Creator

Select the run configuration `librepcb` and click on the `Run` button:

![run_librepcb](https://cloud.githubusercontent.com/assets/5374821/11880865/82574916-a503-11e5-9ec1-ad79b0e2d0d5.png)

#### From Command Line

```bash
./generated/unix/librepcb
```

#### Workspace

At the first startup, LibrePCB asks for a workspace directory where the library
elements and projects will be saved.  For developers there is a demo workspace
inclusive library and projects in the submodule "dev/demo-workspace/".

*Note: As LibrePCB is still in developement, it's highly recommended to use this 
demo workspace. Otherwise you won't be able to create/use any components and some 
parts of the application will not work properly.*

### Installation

To install LibrePCB on a Linux/UNIX system, just execute the following command after building:

```bash
sudo make install
```

## Credits

- First of all, many thanks to all of our [constributors](AUTHORS.md)!
- Thanks to [cloudscale.ch](https://www.cloudscale.ch/) for sponsoring our API server!

## License

LibrePCB is published under the [GNU GPLv3](http://www.gnu.org/licenses/gpl-3.0.html) license.
