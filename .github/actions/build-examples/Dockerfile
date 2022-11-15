#
# build image:
#       docker build . -t note-msp430
#
# run image with local working copy mounted:
#       docker run --entrypoint bash --interactive --rm --tty --volume "$(pwd)":/host-volume/ --workdir /host-volume/ note-msp430

FROM ubuntu:bionic

# Install base packages
RUN apt-get update && \
        apt-get install -y apt-utils ssh git wget build-essential zlib1g-dev \
        ca-certificates apt-transport-https gnupg software-properties-common \
        python3-pip unzip


# Install CMake
RUN wget -O - "https://apt.kitware.com/keys/kitware-archive-latest.asc" 2>/dev/null | apt-key add - && \
        apt-add-repository "deb https://apt.kitware.com/ubuntu/ bionic main" && \
        apt-get update && \
        apt-get install -y cmake

# Download, extract and run the msp gcc open source installer
ENV MSP430_GCC_VERSION 9.3.1.2
ENV MSP430_GCC_PATH /opt/ti/msp430-gcc

RUN wget https://dr-download.ti.com/software-development/ide-configuration-compiler-or-debugger/MD-LlCjWuAbzH/${MSP430_GCC_VERSION}/msp430-gcc-full-linux-x64-installer-${MSP430_GCC_VERSION}.7z \
 && apt-get install -y p7zip \
 && p7zip -d msp430-gcc-full-linux-x64-installer-${MSP430_GCC_VERSION}.7z \
 && ./msp430-gcc-full-linux-x64-installer-${MSP430_GCC_VERSION}.run --prefix ${MSP430_GCC_PATH} --mode unattended \
 && rm ./msp430-gcc-full-linux-x64-installer-${MSP430_GCC_VERSION}.run

