#!/bin/sh

clang-format --style=file -i *.cpp doc/*.cpp include/*.h rss/*.h rss/*.cpp src/*.cpp test/*.h test/*.cpp
