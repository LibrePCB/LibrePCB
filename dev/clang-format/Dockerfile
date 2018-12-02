# Usage:
#
#   $ docker build . -t librepcb/clang-format:6
#
# Then from the main directory:
#
#   $ docker run --rm -t -i -v $(pwd):/code librepcb/clang-format:6 /bin/bash -c "cd /code && dev/format_code.sh"

FROM ubuntu:bionic

RUN apt-get -q update \
 && apt-get -qy install \
  clang-format-6.0 \
  git \
 && apt-get clean

RUN update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-6.0 50
