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
one that we use for Continuous Integration:

    # In the root of Newsboat's repository
    $ docker build \
        --tag=Newsboat-build-tools \
        --file=docker/ubuntu_22.04-build-tools.dockerfile \
        docker

This will use the description from "docker/ubuntu_22.04-build-tools.dockerfile"
to build an image named "Newsboat-build-tools". That image contains all the
compilers and libraries that one needs to build Newsboat from source.

You can now create a container from that image, and run commands inside it. But
the coolest thing is: you can run those commands *on the files in your host
system*. This way, you can have an isolated, controlled build environment, while
using your favourite tools to edit the files. Let's build Newsboat this way:

    $ docker run \
        --rm \
        --mount type=bind,source=$(pwd),target=/home/builder/src \
        --user $(id -u):$(id -g) \
        Newsboat-build-tools \
        make -j9

`--rm` deletes the container once it finished, by default it is kept and will
just litter up your system. `--mount` links your current directory to
"/home/builder/src" inside the container. `--user` specifies the user and the
group that will own the newly created files (object files, docs, and the final
executable); `id` determines your current user and group IDs.
"Newsboat-build-tools" is the image from which we're creating the container, and
`make -j9` is the command we're running inside of it.

Newsboat depends on a number of Rust packages ("crates"), which it downloads on
each build using Cargo. To save on bandwidth, and speed up the build, you can
share your host's Cargo cache with the container:

    # Creating the directory in case it doesn't exist
    $ mkdir -p ~/.cargo/registry
    $ docker run \
        --mount type=bind,source=$HOME/.cargo/registry,target=/home/builder/.cargo/registry \
        ... # the rest of the options

Sharing files between host system and the container has a downside: the
resulting binaries are shared, too. This can lead to linking errors and other
strange behaviour. When you're switching from container to host, or vice versa,
remove all binaries with this command:

    $ make distclean

That's all the basics that you'll need to e.g. build Newsboat in Docker, or to
reproduce an issue with CI. If you want to dive deeper, take a look at files in
docker/ directory. All of them have a short description of what they're for, how
to build them, and how to run them. Our Cirrus CI config (.cirrus.yml) shows how
we use those to build and test Newsboat on every commit.
