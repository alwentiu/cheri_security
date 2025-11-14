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

# Install Morello LLVM
export LLVM_PREFIX=${HOME}/llvm
rm -rf ${LLVM_PREFIX}
mkdir -p ${LLVM_PREFIX}
pushd ${LLVM_PREFIX}
git init
repo=https://git.morello-project.org/morello/llvm-project-releases.git
branch=morello/linux-aarch64-release-${MORELLO_LLVM_VERSION}
git fetch -- ${repo} +refs/heads/${branch}:refs/remotes/origin/${branch}
git checkout origin/${branch} -b ${branch}
popd

# Checkout Musl sources
export MUSL_SOURCES=${HOME}/musl
rm -rf ${MUSL_SOURCES}
mkdir -p ${MUSL_SOURCES}
pushd ${MUSL_SOURCES}
git init
repo=https://git.morello-project.org/morello/musl-libc.git
gitref=${MUSL_GITREF}
git fetch -- ${repo} +refs/heads/${gitref}:refs/remotes/origin/${gitref}
git checkout origin/${gitref} -b ${gitref}
popd

# Build Musl Sysroot for Purecap
export MUSL_PREFIX_PURECAP=${HOME}/musl-sysroot-purecap
pushd ${MUSL_SOURCES}
make distclean
CC=${LLVM_PREFIX}/bin/clang ./configure --prefix=${MUSL_PREFIX_PURECAP} --target=aarch64-linux-musl_purecap
make -j8
make install
popd

# Build Musl Sysroot for Hybrid
export MUSL_PREFIX_HYBRID=${HOME}/musl-sysroot-hybrid
pushd ${MUSL_SOURCES}
make distclean
CC=${LLVM_PREFIX}/bin/clang ./configure --prefix=${MUSL_PREFIX_HYBRID} --disable-morello --target=aarch64-linux-gnu
make -j8
make install
popd


# Build and test:
touch config.make
make distclean
./configure CC=${LLVM_PREFIX}/bin/clang --sysroot=${MUSL_PREFIX_PURECAP} --sysroot-hybrid=${MUSL_PREFIX_HYBRID}
make
make test TEST_RUNNER="${MORELLOIE} --"
