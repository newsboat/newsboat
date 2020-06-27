# Tools for cross-building Newsboat to i686.
#
# Build with defaults:
#
#   docker build \
#       --tag=newsboat-i686-build-tools \
#       --file=docker/ubuntu_18.04-i686.dockerfile \
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
#       --mount type=bind,source=$HOME/.cargo,target=/home/builder/.cargo \
#       --user $(id -u):$(id -g) \
#       newsboat-i686-build-tools \
#       make
#
# If you want to build on the host again, run this to remove binary files
# compiled in the container:
#
#   make distclean

FROM ubuntu:20.04

ENV DEBIAN_FRONTEND noninteractive
ENV PATH /home/builder/.cargo/bin:$PATH
ENV CXXFLAGS -m32
ENV CARGO_BUILD_TARGET i686-unknown-linux-gnu
ENV PKG_CONFIG_ALLOW_CROSS 1

RUN dpkg --add-architecture i386 \
    && apt-get update \
    && apt-get upgrade --assume-yes

RUN apt-get update \
    && apt-get install --assume-yes \
        build-essential gettext g++-multilib libcurl4-openssl-dev:i386 \
        libjson-c-dev:i386 libsqlite3-0:i386 libsqlite3-dev:i386 \
        libssl-dev:i386 libstfl-dev:i386 libxml2-dev:i386 pkg-config:i386 \
        # `curl` would be enough for our needs, but it pulls in amd64 versions
        # of libraries we use, interfering with the build environment. So
        # `wget` it is.
        wget \
    && apt-get install --assume-yes --no-install-recommends asciidoctor \
    && apt-get autoremove \
    && apt-get clean

RUN addgroup --gid 1000 builder \
    && adduser --home /home/builder --uid 1000 --ingroup builder \
        --disabled-password --shell /bin/bash builder \
    && mkdir -p /home/builder/src \
    && chown -R builder:builder /home/builder

RUN apt-get update \
    && apt-get install locales \
    && echo 'en_US.UTF-8 UTF-8' >> /etc/locale.gen \
    && locale-gen
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

USER builder
WORKDIR /home/builder/src

RUN wget -O $HOME/rustup.sh --secure-protocol=TLSv1_2 https://sh.rustup.rs \
    && chmod +x $HOME/rustup.sh \
    && $HOME/rustup.sh -y \
        --default-host i686-unknown-linux-gnu \
        --default-toolchain 1.44.1 \
        # Install only rustc, rust-std, and cargo
        --profile minimal \
    && chmod a+w $HOME/.cargo

ENV HOME /home/builder
