# Contributing to Newsboat

Thank you for your interest in contributing to Newsboat! Here are a few ways
you can help:

1. [Security Vulnerabilities](#security-vulnerabilities)
2. [Feature Requests and Bug Reports](#feature-requests-and-bug-reports)
3. [Patches/Pull Requests](#patchespull-requests)
4. [Documentation](#documentation)
5. [Translations](#translations)


## Security Vulnerabilities

If you happen to stumble upon a security vulnerability, **do not open a GitHub issue**.
Instead, send an email to security@Newsboat.org and encrypt your emails using
[OpenPGP key 4ED6CD61932B9EBE](https://Newsboat.org/Newsboat.pgp).


## Feature Requests and Bug Reports

Bug reports and feature requests should be reported in the
[issue tracker](https://github.com/Newsboat/Newsboat/issues). Look through the
issue tracker to see if your bug report/feature request was not already
reported. Don't forget to look through the closed issues, as your request might
have been already fulfilled. But if in doubt, please file an issue anyway!


## Patches/Pull Requests

If you are looking for a place to start contributing, take a look at the issues labeled
[good first issue](https://github.com/Newsboat/Newsboat/labels/good%20first%20issue)
and [docs](https://github.com/Newsboat/Newsboat/labels/docs) on the issue tracker.
Contributions that modernize the C++ and clean up the codebase are also great places
to start.

Remember to write clear and concise commit messages describing your changes and make
sure to reference any issues and/or other commits. An example of a well written
commit message can be found [here](https://chris.beams.io/posts/git-commit/).

Please follow our [style guide](doc/internal/code-style.markdown) when
contributing code. See also [the Newsboat hacker's
guide](doc/internal/hackers-guide.asciidoc).

Patches can be submitted by sending a
[GitHub Pull Request](https://github.com/Newsboat/Newsboat/pull/new/master) or
emailing a patch to the Newsboat
[mailing list](https://groups.google.com/group/Newsboat) (Newsboat@googlegroups.com).
To learn more about patches, see
[here](https://www.kernel.org/doc/html/latest/process/submitting-patches.html) and
[here](https://github.com/git/git/blob/master/Documentation/SubmittingPatches). To
learn more about pull requests,
see the [GitHub docs](https://docs.github.com/en/free-pro-team@latest/github/collaborating-with-issues-and-pull-requests/creating-a-pull-request).

### Building

Newsboat can be built by running:

	$ make -jN

Where `N` is again the number of threads you want to use.

Newsboat can also be [built in Docker](doc/docker.md).

### Testing

When contributing code, it's wise to test your code to ensure it it hasn't
broken existing functionality. This can by done by running:

	$ TMPDIR=/dev/shm make -j5 PROFILE=1 check

The `5` here is the number of CPU cores in your machine *plus one*. This
parallelises the build. Rust tests already utilize as many cores as they can,
but if you want to limit them, use the `RUST_TEST_THREADS` environment variable.
`/dev/shm` is a "ramdisk", i.e. a virtual disk stored in the RAM. The tests
create a lot of temporary files, and benefit from fast storage; a ramdisk is
even better than an SSD.


## Documentation

Fixing and/or adding documentation is an easy way to get involved. The Newsboat
documentation can be found in `doc/` and related issues can be found under the
`docs` label in the
[issue tracker](https://github.com/Newsboat/Newsboat/labels/docs). New features
that are added should also be documented.


## Translations

Fixing and/or adding translations helps localize Newsboat to different regions
around the globe. Translations for the Newsboat interface can be found in `po/`
and related issues can be found under the `i18n` tag in the
[issue tracker](https://github.com/Newsboat/Newsboat/labels/i18n).
