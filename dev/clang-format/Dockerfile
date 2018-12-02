# Container with clang-format 6. Used in the `dev/format_code.sh` script.

FROM ubuntu:bionic

RUN apt-get -q update \
 && apt-get -qy install \
  clang-format-6.0 \
  git \
 && apt-get clean

RUN update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-6.0 50
