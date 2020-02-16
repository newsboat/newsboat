FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive
ENV PATH /home/builder/.cargo/bin:$PATH
ENV CXXFLAGS -m32
ENV CARGO_BUILD_TARGET i686-unknown-linux-gnu
ENV PKG_CONFIG_ALLOW_CROSS 1

RUN dpkg --add-architecture i386 \
    && apt-get update \
    && apt-get upgrade --assume-yes

RUN apt-get install --assume-yes \
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

RUN apt-get install locales \
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
        --default-toolchain stable \
        # Install only rustc, rust-std, and cargo
        --profile minimal \
    && chmod a+w $HOME/.cargo

ENV HOME /home/builder
