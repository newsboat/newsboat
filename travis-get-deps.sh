#!/bin/sh

case $TRAVIS_OS_NAME in
    "linux")
        sudo apt-add-repository -y ppa:ondrej/php5
        sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
        sudo apt-get update
        sudo apt-get install -qq libsqlite3-dev libcurl4-openssl-dev libxml2-dev libstfl-dev libjson-c-dev libncursesw5-dev bc asciidoc
        pip install --user cpp-coveralls
        ;;

    "osx")
        brew update
        brew install "gettext" && brew link --force "gettext"
        brew outdated "sqlite" || brew upgrade "sqlite"
        brew outdated "curl" || brew upgrade "curl"
        brew install "libstfl"
        brew outdated "json-c" || brew upgrade "json-c"
        brew install "asciidoc"

        brew install "pyenv"
        eval "$(pyenv init -)"
        pyenv install 2.7.6
        pyenv global 2.7.6
        pyenv rehash
        pip install cpp-coveralls
        pyenv rehash
        ;;
esac
