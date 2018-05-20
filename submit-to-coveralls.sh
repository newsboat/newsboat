#!/bin/sh

if [ "$TRAVIS_OS_NAME" = "osx" ]
then
    eval "$(pyenv init -)"
fi

coveralls \
    --exclude 'test' \
    --exclude 'usr' \
    --exclude '3rd-party' \
    --gcov "$GCOV" --gcov-options '\-lp'
