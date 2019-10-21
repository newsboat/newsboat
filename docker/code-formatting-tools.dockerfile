FROM rust:1.38.0-alpine3.10
WORKDIR /workspace
RUN apk add --no-cache astyle==3.1-r2 git make
RUN rustup component add rustfmt
