#!/bin/bash

USER_ID=${HOST_UID:-1000}
GROUP_ID=${HOST_GID:-1000}

echo "Starting with UID: $USER_ID, GID: $GROUP_ID"
usermod -u $USER_ID builder
groupmod -g $GROUP_ID builder

exec /usr/sbin/gosu builder "$@"
