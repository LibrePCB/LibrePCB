#!/bin/bash

if [[ ! -z "$@" ]]; then
  docker start librepcb
  docker exec -it librepcb $@
  docker stop -t 0 librepcb
else
  docker start -i librepcb
fi
