# A container with tools for re-formatting our source code. It contains:
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

FROM rust:1.85.0-bookworm
WORKDIR /workspace
# Ensure that Astyle 3.1 is used, because later versions are buggy for us:
# https://gitlab.com/saalen/astyle/-/issues/45
RUN apt-get update && apt-get install -y astyle=3.1-* && apt-get clean && rm -rf /var/lib/apt/lists/*
RUN rustup component add rustfmt
