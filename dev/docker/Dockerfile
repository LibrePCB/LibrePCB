FROM ubuntu:14.04

# install packages
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
  && mkdir /opt/build-librepcb-Desktop-Debug \
  && cd /opt/build-librepcb-Desktop-Debug \
  && qmake -r ../LibrePCB/librepcb.pro \
  && make -j8 \
  && make install

# copy default config file
RUN cp /opt/LibrePCB/dev/docker/LibrePCB_git-master.conf /root/.config/LibrePCB/LibrePCB_git-master.conf

# set working directory and default command to execute
WORKDIR /opt/LibrePCB
CMD bash -C "/opt/LibrePCB/dev/docker/startup.sh";"bash"
