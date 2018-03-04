FROM ubuntu:14.04

# install packages
RUN DEBIAN_FRONTEND=noninteractive \
     apt-get -q update \
  && apt-get -qy upgrade \
  && apt-get -qy install git g++ qt5-default qttools5-dev-tools qt5-doc qtcreator libglu1-mesa-dev dia \
  && apt-get clean

# checkout librepcb
RUN git clone --recursive https://github.com/LibrePCB/LibrePCB.git /opt/LibrePCB \
  && cd /opt/LibrePCB

# build and install librepcb
RUN /opt/LibrePCB/dev/docker/make_librepcb.sh

# copy default config file
RUN mkdir -p /root/.config/LibrePCB/ \
  && cp /opt/LibrePCB/dev/docker/LibrePCB.conf /root/.config/LibrePCB/LibrePCB.conf

# ensure that the USER env var is set appropriately
ARG DOCKER_USER=root
ENV USER=$DOCKER_USER

# set working directory and default command to execute
WORKDIR /opt/LibrePCB
CMD bash -C "/opt/LibrePCB/dev/docker/startup.sh";"bash"
