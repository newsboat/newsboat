FROM rust:1.38.0-alpine3.10
WORKDIR /workspace
ENV CLANG_VERSION=8.0.0-r0
RUN apk add --no-cache clang==$CLANG_VERSION
RUN rustup component add rustfmt
