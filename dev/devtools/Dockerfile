# Container with clang-format 15. Used in the `dev/format_code.sh` script.

FROM ubuntu:22.04

RUN apt-get -q update \
 && apt-get -qy install \
  clang-format-15 \
  git \
  python3 \
  python3-pip \
  qtdeclarative5-dev-tools \
  libxml-filter-sort-perl \
 && apt-get clean

RUN update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-15 50

RUN pip3 install cmakelang==0.6.13
