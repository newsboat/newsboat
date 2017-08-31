#!/bin/sh

case $TRAVIS_OS_NAME in
    "osx")
        brew update
        brew outdated "pkg-config" || brew upgrade "pkg-config"
        brew install "gettext" && brew link --force "gettext"
        brew outdated "sqlite" || brew upgrade "sqlite"
        brew outdated "curl" || brew upgrade "curl"
        brew install "libstfl"
        brew outdated "json-c" || brew upgrade "json-c"
        brew install "asciidoc"

        brew install "python"
        brew install "pyenv"
        eval "$(pyenv init -)"
        pip install cpp-coveralls
        pyenv rehash
        ;;
esac
