FROM ubuntu:14.04

# install repos and packages
RUN DEBIAN_FRONTEND=noninteractive \
     apt-get -q update \
  && apt-get -qy upgrade \
  && apt-get -qy install git g++ qt5-default libqt5webkit5 libqt5webkit5-dev qttools5-dev-tools qt5-doc qtcreator libglu1-mesa-dev \
  && apt-get clean

# checkout, build, and install librepcb
RUN git clone https://github.com/LibrePCB/LibrePCB.git /opt/LibrePCB \
  && cd /opt/LibrePCB \
  && git submodule init \
  && git submodule update \
  && mkdir /opt/LibrePCB/build \
  && cd /opt/LibrePCB/build \
  && qmake -r ../librepcb.pro \
  && make -j8 \
  && make install

# copy config file
COPY LibrePCB_git-master.conf /root/.config/LibrePCB/

WORKDIR /opt/LibrePCB
CMD /bin/bash
