# All the programs and libraries necessary to build Newsboat with newer
# compilers. Contains GCC 13 and Rust 1.76.0 by default.
#
# Configurable via build-args:
#
# - cxx_package -- additional Ubuntu packages to install. Default: g++-13
# - rust_version -- Rust version to install. Default: 1.76.0
# - cc -- C compiler to use. This gets copied into CC environment variable.
#       Default: gcc-13
# - cxx -- C++ compiler to use. This gets copied into CXX environment variable.
#       Default: g++-13
#
# For now, this Dockerfile can only be used in our CI.

FROM ubuntu:23.10

ENV DEBIAN_FRONTEND noninteractive
ENV PATH /home/builder/.cargo/bin:$PATH

RUN apt-get update \
    && apt-get upgrade --assume-yes \
    && apt install --assume-yes --no-install-recommends ca-certificates wget gnupg2

ARG cxx_package=g++-13

RUN apt-get update \
    && apt-get install --assume-yes --no-install-recommends \
        build-essential $cxx_package libsqlite3-dev libcurl4-openssl-dev libssl-dev \
        libxml2-dev libstfl-dev libjson-c-dev libncursesw5-dev gettext git \
        pkg-config zlib1g-dev asciidoctor \
    && apt-get autoremove \
    && apt-get clean

RUN addgroup builder \
    && adduser --home /home/builder --ingroup builder \
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

ARG rust_version=1.76.0

RUN wget -O $HOME/rustup.sh --secure-protocol=TLSv1_2 https://sh.rustup.rs \
    && chmod +x $HOME/rustup.sh \
    && $HOME/rustup.sh -y \
        --default-host x86_64-unknown-linux-gnu \
        --default-toolchain $rust_version \
    && chmod a+w $HOME/.cargo

ENV HOME /home/builder

ARG cc=gcc-13
ARG cxx=g++-13

ENV CC=$cc
ENV CXX=$cxx
