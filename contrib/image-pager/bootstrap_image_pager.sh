#!/bin/bash

# Run the Scala program and capture its output
scala -classpath "$(dirname "$0")" imagePager "$@" | read command

echo "Command: $command"

# Run the command returned by the Scala program
eval "$command"
