#!/bin/sh

# Run this script on a machine with Clang 18 and Rust 1.81. These versions both
# use LLVM 18; matching LLVM versions are required for grcov to produce
# a report across the entire codebase.
#
# One can prepare such environment with Docker:
#
# docker build \
#   --build-arg rust_version=1.81 \
#   --build-arg cxx_package='clang-18 libclang-rt-18-dev' \
#   --build-arg cxx=clang++-18 \
#   --build-arg cc=clang-18 \
#   --tag=Newsboat-clang-18-rust-1.82:24.04 \
#   --file=docker/ubuntu_24.04-build-tools.dockerfile \
#   docker
#
# docker run \
#   -it --rm \
#   --mount type=bind,source=$(pwd),target=/workspace -w /workspace \
#   --user $(id -u):$(id -g) \
#   Newsboat-clang-18-rust-1.81:24.04 \
#   bash -c 'rustup component add llvm-tools-preview && cargo install grcov && test/generate_coverage_report.sh'

set -e

export CC=clang-19
export CXX=clang++-19
export CXXFLAGS='-O0 -fprofile-instr-generate -fcoverage-mapping'
export RUSTFLAGS='-Clink-dead-code -Cinstrument-coverage'
export LLVM_PROFILE_FILE='%h_%m.profraw'
export PROFILE=1

OUTDIR=html/coverage

make --jobs=9 NEWSBOAT_RUN_IGNORED_TESTS=1 ci-check

rm -rf html
grcov . --source-dir . --binary-path . \
    --ignore-not-existing \
    --ignore='/*' --ignore='3rd-party/*' --ignore='doc/*' --ignore='test/*' \
    --ignore='target/*' --ignore='Newsboat.cpp' --ignore='Podboat.cpp' \
    -t html -o ./$OUTDIR

echo "Coverage reports:"
find $OUTDIR -name 'index.html'
