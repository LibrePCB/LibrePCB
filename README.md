# LibrePCB [![Build Status](https://travis-ci.org/LibrePCB/LibrePCB.svg?branch=master)](https://travis-ci.org/LibrePCB/LibrePCB)

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
sudo apt-get install g++ qt5-default libqt5webkit5 libqt5webkit5-dev qttools5-dev-tools qt5-doc qtcreator libglu1-mesa-dev
```

#### Installation in a docker container

To build and run LibrePCB in a [docker](https://www.docker.com/) container (which is pretty cool!), check out [these instructions](https://github.com/LibrePCB/LibrePCB/tree/master/dev/docker).

### Building

#### Using qtcreator

Building with [qtcreator](http://doc.qt.io/qtcreator/) is probably the easiest
way.  To keep build time as low as possible make sure to set the correct make
flags to use all available CPU cores to build. See this [stackoverflow
answer](https://stackoverflow.com/questions/8860712/setting-default-make-options-for-qt-creator).

#### Using qmake and make

Since qtcreator is also using qmake and make to build, it's easy to do the same
on the command line:

```bash
$ mkdir build && cd build
$ qmake -r ../librepcb.pro
$ make -j 8
```

Then grab a coffee, since the compilation takes about two minutes on a decent
laptop:
```
make -j8  819.54s user 44.92s system 726% cpu 1:58.93 total
```

### Documentation

- Doxygen: http://librepcb.github.io/LibrePCB-Doxygen/master/
- Some diagrams (partially outdated): https://github.com/LibrePCB/LibrePCB/tree/master/dev/diagrams/svg

### Workspace

At the first startup, LibrePCB asks for a workspace directory where the library
elements and projects will be saved.  For developers there is a demo workspace
inclusive library and projects in the submodule "dev/demo-workspace/".

## License

LibrePCB is published under the [GNU GPLv3](http://www.gnu.org/licenses/gpl-3.0.html) license.
