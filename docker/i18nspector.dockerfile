# This image contains i18nspector, a tool we use to check our
# internationalization files (*.po).
#
# Build:
#
#   docker build \
#       --tag=Newsboat-i18nspector \
#       --file=docker/i18nspector.dockerfile \
#       docker
#
# Run on your local files:
#
#   docker run \
#       --rm \
#       --mount type=bind,source=$(pwd),target=/workdir \
#       Newsboat-i18nspector \
#       make run-i18nspector
#
# On continuous integration, we fail a build if i18nspector prints out any
# errors or warnings (lines that start with "E:" and "W:").

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update \
    && apt-get upgrade --assume-yes \
    && apt-get install --assume-yes --no-install-recommends i18nspector make

WORKDIR /workdir
