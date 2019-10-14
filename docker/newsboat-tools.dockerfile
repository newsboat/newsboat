FROM rust:1.38.0-alpine3.10
WORKDIR /workspace
ENV CLANG_VERSION=8.0.0-r0
RUN apk add --no-cache git clang==$CLANG_VERSION make
RUN rustup component add rustfmt
