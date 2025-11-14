#!/bin/bash

set -xe

sudo apt update -q=2
sudo apt install -q=2 --yes --no-install-recommends --no-install-suggests wget

export MORELLOIE_PREFIX=${HOME}/morelloie
export MORELLOIE=${MORELLOIE_PREFIX}/bin/morelloie

# Install Morello IE
rm -rf ${HOME}/morelloie-* ${MORELLOIE_PREFIX}
wget -q ${MORELLOIE_DOWNLOAD_URL}/morelloie-${MORELLOIE_VERSION}.tgz.sh -O ${HOME}/morelloie-${MORELLOIE_VERSION}.tgz.sh
bash ${HOME}/morelloie-${MORELLOIE_VERSION}.tgz.sh --i-agree-to-the-contained-eula --prefix=${MORELLOIE_PREFIX}

# Install Morello GCC
export GCC_PREFIX=${HOME}/gcc
rm -rf ${HOME}/morello-gcc-*.tar.xz ${GCC_PREFIX}
wget -q ${MORELLO_GCC_DOWNLOAD_URL}/${MORELLO_GCC_VERSION}/binrel/arm-gnu-toolchain-${MORELLO_GCC_VERSION}-aarch64-aarch64-none-linux-gnu.tar.xz -O ${HOME}/morello-gcc-${MORELLO_GCC_VERSION}.tar.xz
mkdir -p ${GCC_PREFIX}
pushd ${GCC_PREFIX}
tar -xf ${HOME}/morello-gcc-${MORELLO_GCC_VERSION}.tar.xz --strip-components 1
popd

# Build and test:
touch config.make
make distclean
./configure CC=${GCC_PREFIX}/bin/aarch64-none-linux-gnu-gcc
make
make test TEST_RUNNER="${MORELLOIE} --"
