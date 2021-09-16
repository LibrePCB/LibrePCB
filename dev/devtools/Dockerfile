# Container with clang-format 6. Used in the `dev/format_code.sh` script.

FROM ubuntu:20.04

RUN apt-get -q update \
 && apt-get -qy install \
  clang-format-6.0 \
  git \
  python3 \
 && apt-get clean

RUN update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-6.0 50
