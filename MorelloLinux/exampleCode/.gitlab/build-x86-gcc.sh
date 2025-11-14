#!/bin/bash

set -xe

uname -a

sudo apt update -q=2
sudo apt install -q=2 --yes --no-install-recommends --no-install-suggests wget

# Install Morello GCC
export GCC_PREFIX=${HOME}/gcc
rm -rf ${HOME}/morello-gcc-*.tar.xz ${GCC_PREFIX}
wget -q ${MORELLO_GCC_DOWNLOAD_URL}/${MORELLO_GCC_VERSION}/binrel/arm-gnu-toolchain-${MORELLO_GCC_VERSION}-x86_64-aarch64-none-linux-gnu.tar.xz -O ${HOME}/morello-gcc-${MORELLO_GCC_VERSION}.tar.xz
mkdir -p ${GCC_PREFIX}
pushd ${GCC_PREFIX}
tar -xf ${HOME}/morello-gcc-${MORELLO_GCC_VERSION}.tar.xz --strip-components 1
popd

# Build:
touch config.make
make distclean
./configure CC=${GCC_PREFIX}/bin/aarch64-none-linux-gnu-gcc
make
