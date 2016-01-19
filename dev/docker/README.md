# LibrePCB Dockerfile

With the files provided in this directory, you are able to build and run LibrePCB in a [docker](https://www.docker.com/) container. Try it out!

### Install Docker

See installation instructions of docker: https://docs.docker.com/installation/

### Get all required files

If you already have cloned the LibrePCB repository with git, you do not need to download these files again. Just skip this step.

```bash
mkdir librepcb-docker && cd librepcb-docker
wget https://raw.githubusercontent.com/LibrePCB/LibrePCB/master/dev/docker/Dockerfile
wget https://raw.githubusercontent.com/LibrePCB/LibrePCB/master/dev/docker/build_container.sh
wget https://raw.githubusercontent.com/LibrePCB/LibrePCB/master/dev/docker/run_container.sh
```

### Build the container

Build the docker container "librepcb:git":

```bash
./build_container.sh
```

### Run the container

Different examples to run the docker container "librepcb:git":

```bash
./run_container.sh                          # enter the bash
./run_container.sh qtcreator librepcb.pro   # open Qt Creator
./run_container.sh librepcb                 # run LibrePCB
./run_container.sh dia                      # run Dia (diagram editor)
```

