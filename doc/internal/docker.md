Building Newsboat with Docker
=============================

[Docker](https://www.docker.com/) is a program that builds Linux containers
according to the spec provided by the user. We use Docker to create development
environments, with all the necessary tools and libraries already installed.
These containers can be used for continuous testing, and also locally if you
feel like it.

Each Docker container is described by a "Dockerfile". We keep ours in the
"docker" directory.

To use a container, you need to build its image first. For example, let's create
one that we use for cross-compiling from amd64 to i686:

    # In the root of Newsboat's repository
    $ docker build \
        --tag=newsboat-ubuntu18.04-i686 \
        --file=docker/ubuntu_18.04-i686.dockerfile \
        docker

This will use the description from "docker/ubuntu_18.04-i686.dockerfile" to
build an image named "newsboat-ubuntu18.04-i686".

You can now create a container from that image, and run commands inside it. But
the coolest thing is: you can run those commands *on the files in your host
system*. This way, you can have an isolated, controlled build environment, while
using your favourite tools to edit the files. Let's build Newsboat this way:

    $ docker run \
        --rm \
        --mount type=bind,source=$(pwd),target=/home/builder/src \
        --user $(id -u):$(id -g) \
        newsboat-ubuntu18.04-i686 \
        make -j9

`--rm` deletes the container once it finished, by default it is kept and will
just litter up your system.
`--mount` links your current directory to "/home/builder/src" inside the container.
`--user` specifies the user and the group that will own the newly created files
(object files, docs, and the final executable); `id` determines your current
user and group IDs.
"newsboat-ubuntu18.04-i686" is the image from which we're creating the
container, and `make -j9` is the command we're running inside of it.
