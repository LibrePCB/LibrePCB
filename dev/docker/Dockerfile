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

# create config file to set the workspace path
RUN mkdir -p /root/.config/LibrePCB && echo "[workspaces]\nmost_recently_used=/opt/LibrePCB/dev/demo-workspace" > /root/.config/LibrePCB/LibrePCB_git-master.conf

# create a startup shell script to show some information
RUN echo "#!/bin/bash"                                                                                                  > startup.sh \
  && echo "echo 'Welcome to LibrePCB! Check out the following commands:'"                                               >> startup.sh \
  && echo "echo ' - git pull                                                      : update git working copy'"           >> startup.sh \
  && echo "echo ' - cd build && qmake -r ../librepcb.pro && make && make install  : compile and install LibrePCB'"      >> startup.sh \
  && echo "echo ' - qtcreator librepcb.pro                                        : open the project with Qt Creator'"  >> startup.sh \
  && echo "echo ' - librepcb                                                      : run LibrePCB'"                      >> startup.sh

# set working directory and default command to execute
WORKDIR /opt/LibrePCB
CMD bash -C "/startup.sh";"bash"
