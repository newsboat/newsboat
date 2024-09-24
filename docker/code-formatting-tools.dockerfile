# A lightweight container for re-formatting our source code. It contains:
#
# - astyle and rustfmt (code formatters)
# - GNU make (to run the aforementioned formatters)
# - git (so the CI can check if formatting introduced any changes)
#
# Build with:
#
#   docker build \
#       --tag=newsboat-code-formatting-tools \
#       --file=docker/code-formatting-tools.dockerfile \
#       docker
#
# Run on your local files with:
#
#   docker run \
#       --rm \
#       --mount type=bind,source=$(pwd),target=/workspace \
#       --user $(id -u):$(id -g) \
#       newsboat-code-formatting-tools \
#       make fmt

# Use Alpine 3.18 because 3.19 ships astyle 3.4, which is buggy for us:
# https://gitlab.com/saalen/astyle/-/issues/45
FROM rust:1.79.0-alpine3.18
WORKDIR /workspace
RUN apk add --no-cache astyle==3.1-r4 git make
RUN rustup component add rustfmt
