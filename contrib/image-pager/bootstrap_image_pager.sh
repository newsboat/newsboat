#!/bin/bash

# Set the classpath to the directory containing the script
classpath="$(dirname "$0")"

# Run the Scala program and capture its output
result=$(scala -classpath "$classpath" imagePager "$@")

# Print the result
#echo "Received result: $result"

eval "$result"
