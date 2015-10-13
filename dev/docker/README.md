# LibrePCB Dockerfile

With the files provided in this directory, you are able to build and run LibrePCB in a [docker](https://www.docker.com/) container. Try it out!

### Install Docker

See installation instructions of docker: https://docs.docker.com/installation/

### Get all required files

If you already have cloned the LibrePCB repository with git, you do not need to download these files again. Just skip this step.

```bash
mkdir librepcb-docker && cd librepcb-docker
wget https://raw.githubusercontent.com/LibrePCB/LibrePCB/master/dev/docker/Dockerfile
wget https://raw.githubusercontent.com/LibrePCB/LibrePCB/master/dev/docker/build.sh
wget https://raw.githubusercontent.com/LibrePCB/LibrePCB/master/dev/docker/run.sh
```

### Build the container

Build the docker container "librepcb:git":

```bash
./build.sh
```

### Run the container

Different examples to run the docker container "librepcb:git":

```bash
./run.sh                          # enter the bash
./run.sh qtcreator librepcb.pro   # open Qt Creator
./run.sh librepcb                 # run LibrePCB
```

