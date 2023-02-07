FROM ubuntu
COPY pico_setup.sh .

RUN echo \$@ > /bin/sudo && chmod a+x /bin/sudo
RUN DEBIAN_FRONTEND=noninteractive ./pico_setup.sh

RUN apt-get install -y python3 mc vim

COPY 0001-disable-RP2040-USB-Host-double-buffering.patch /0001-disable-RP2040-USB-Host-double-buffering.patch
COPY patch1.sh /
RUN ./patch1.sh

CMD bash

