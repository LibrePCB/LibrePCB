# Usage:
#
# $ docker build . -t librepcb/clang-format:6
#
# Then from the main directory:
#
# $ docker build . -t librepcb/clang-format:6
# $ docker run --rm -t -i -v $(pwd):/code librepcb/clang-format:6 /bin/bash -c "cd /code && dev/format_code.sh"
FROM ubuntu:bionic

RUN apt-get -q update \
 && apt-get -qy install \
  clang-format \
  git \
 && apt-get clean
