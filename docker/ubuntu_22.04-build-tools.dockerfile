# All the programs and libraries necessary to build Newsboat with newer
# compilers. Contains GCC 12 and Rust 1.90.0 by default.
#
# Configurable via build-args:
#
# - cxx_package -- additional Ubuntu packages to install. Default: g++-12
# - rust_version -- Rust version to install. Default: 1.90.0
# - cc -- C compiler to use. This gets copied into CC environment variable.
#       Default: gcc-12
# - cxx -- C++ compiler to use. This gets copied into CXX environment variable.
#       Default: g++-12
#
# Build with defaults:
#
#   docker build \
#       --build-arg UID=$(id -u) \
#       --build-arg GID=$(id -g) \
#       --tag=newsboat-build-tools \
#       --file=docker/ubuntu_22.04-build-tools.dockerfile \
#       docker
#
# Build with non-default compiler and Rust version:
#
#   docker build \
#       --build-arg UID=$(id -u) \
#       --build-arg GID=$(id -g) \
#       --tag=newsboat-build-tools \
#       --file=docker/ubuntu_22.04-build-tools.dockerfile \
#       --build-arg cxx_package=clang-13 \
#       --build-arg cc=clang-13 \
#       --build-arg cxx=clang++-13 \
#       --build-arg rust_version=1.40.0 \
#       docker
#
# Before building in a container, run this to remove any binaries that you
# might've compiled on your host system (or in another container):
#
#   make distclean
#
# Run on your local files:
#
#   docker run \
#       --rm \
#       --mount type=bind,source=$(pwd),target=/home/builder/src \
#       newsboat-build-tools \
#       make
#
# To save on bandwidth, and speed up the build slightly, share the host's Cargo
# cache with the container:
#
#   mkdir -p ~/.cargo/registry
#   docker run \
#       --mount type=bind,source=$HOME/.cargo/registry,target=/home/builder/.cargo/registry \
#       ... # the rest of the options
#
# If you want to build on the host again, run this to remove binary files
# compiled in the container:
#
#   make distclean

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND noninteractive
ENV PATH /home/builder/.cargo/bin:$PATH

RUN apt-get update \
    && apt-get upgrade --assume-yes \
    && apt install --assume-yes --no-install-recommends ca-certificates wget gnupg2

ARG cxx_package=g++-12

RUN apt-get update \
    && apt-get install --assume-yes --no-install-recommends \
        build-essential $cxx_package libsqlite3-dev libcurl4-openssl-dev libssl-dev \
        libxml2-dev libstfl-dev libjson-c-dev libncursesw5-dev gettext git \
        pkg-config zlib1g-dev asciidoctor \
    && apt-get autoremove \
    && apt-get clean

ARG UID=1000
ARG GID=1000

RUN addgroup --gid $GID builder \
    && adduser --home /home/builder --uid $UID --ingroup builder \
        --disabled-password --shell /bin/bash builder \
    && mkdir -p /home/builder/src \
    && chown -R builder:builder /home/builder

RUN apt-get install locales \
    && echo 'en_US.UTF-8 UTF-8' >> /etc/locale.gen \
    && echo 'ru_RU.KOI8-R KOI8-R' >> /etc/locale.gen \
    && echo 'ru_RU.CP1251 CP1251' >> /etc/locale.gen \
    && locale-gen
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

USER builder
WORKDIR /home/builder/src

ARG rust_version=1.90.0

RUN wget -O $HOME/rustup.sh --secure-protocol=TLSv1_2 https://sh.rustup.rs \
    && chmod +x $HOME/rustup.sh \
    && $HOME/rustup.sh -y \
        --default-host x86_64-unknown-linux-gnu \
        --default-toolchain $rust_version \
    && chmod a+w $HOME/.cargo

ENV HOME /home/builder

ARG cc=gcc-12
ARG cxx=g++-12

ENV CC=$cc
ENV CXX=$cxx
