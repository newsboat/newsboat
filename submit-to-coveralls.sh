#!/bin/sh

if [ "$TRAVIS_OS_NAME" = "osx" ]
then
    eval "$(pyenv init -)"
fi

coveralls --exclude 'test' --exclude 'usr' --gcov-options '\-lp'
