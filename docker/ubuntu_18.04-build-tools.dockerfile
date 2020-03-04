FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive
ENV PATH /home/builder/.cargo/bin:$PATH

RUN apt-get update \
    && apt-get install --assume-yes --no-install-recommends software-properties-common \
    && add-apt-repository "ppa:ubuntu-toolchain-r/test" \
    && apt-get update \
    && apt-get upgrade --assume-yes

ARG cxx_package

RUN apt-get update \
    && apt-get install --assume-yes --no-install-recommends \
        build-essential $cxx_package libsqlite3-dev libcurl4-openssl-dev libssl-dev \
        libxml2-dev libstfl-dev libjson-c-dev libncursesw5-dev gettext git \
        asciidoctor wget \
    && apt-get autoremove \
    && apt-get clean

RUN addgroup --gid 1000 builder \
    && adduser --home /home/builder --uid 1000 --ingroup builder \
        --disabled-password --shell /bin/bash builder \
    && mkdir -p /home/builder/src \
    && chown -R builder:builder /home/builder

RUN apt-get install locales \
    && echo 'en_US.UTF-8 UTF-8' >> /etc/locale.gen \
    && locale-gen
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

USER builder
WORKDIR /home/builder/src

ARG rust_version=1.41.1

RUN wget -O $HOME/rustup.sh --secure-protocol=TLSv1_2 https://sh.rustup.rs \
    && chmod +x $HOME/rustup.sh \
    && $HOME/rustup.sh -y \
        --default-host x86_64-unknown-linux-gnu \
        --default-toolchain $rust_version \
        # Install only rustc, rust-std, and cargo
        --profile minimal \
    && chmod a+w $HOME/.cargo

ENV HOME /home/builder

ARG cc
ARG cxx

ENV CC=$cc
ENV CXX=$cxx
