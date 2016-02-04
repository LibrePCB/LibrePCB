#!/bin/bash

# build image "librepcb:git"
docker build -t librepcb:git .

# remove container "librepcb" if already existing
STATUS=$(docker inspect --format="{{ .State.Status }}" librepcb 2> /dev/null)
if [ $? -eq 0 ]; then
  echo "WARNING: The container 'librepcb' will be removed! Continue? [Y/n]"
  read answer
  if [ "$answer" == "n" ]; then
    echo "ABORTED"
	  exit 1
  fi
  echo "WARNING: The container 'librepcb' is running! Kill it? [Y/n]"
  read answer
  if [ "$answer" == "n" ]; then
    echo "ABORTED"
	  exit 1
  fi
  docker rm -f librepcb
fi

# create new container "librepcb"
XSOCK=/tmp/.X11-unix
XAUTH=/tmp/.docker.xauth
touch $XAUTH
xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f $XAUTH nmerge -
docker create -it --privileged \
  -v $XSOCK:$XSOCK:rw \
  -v $XAUTH:$XAUTH:rw \
  --device=/dev/dri/card0:/dev/dri/card0 \
  -e DISPLAY=$DISPLAY \
  -e XAUTHORITY=$XAUTH \
  --name librepcb \
  librepcb:git
