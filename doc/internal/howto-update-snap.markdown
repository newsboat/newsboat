How to re-build snap against up-to-date packages
------------------------------------------------

The snap's owner (currently Alexander Batischev) might receive an email from
the Snap Store saying:

> Subject: Newsboat contains outdated Ubuntu packages
>
> A scan of this snap shows that it was built with packages from the Ubuntu
> archive that have since received security updates. The following lists new
> USNs for affected binary packages in each snap revision:
>
> [snip]
>
> Simply rebuilding the snap will pull in the new security updates and resolve
> this. If your snap also contains vendored code, now might be a good time to
> review it for any needed updates.

This document outlines how to rebuild the snap and push it out.

You'll need:
- a machine with Snap installed (I'm using a virtual machine running Ubuntu 20.04);
- login and password for the snap's owner account.

1. Navigate to your local clone of Newsboat.
2. Check out the latest release tag: `git checkout r2.20.1`.
3. Submit a remote build for all architectures mentioned in the email:

    $ snapcraft remote-build --build-for s390x,ppc64el,arm64,armhf,amd64

4. Wait for the build to finish. It will create .snap files in the current
   directory.

5. If it doesn't exist yet, create _~/snapcraft.cred_ file:

    $ snapcraft export-login ~/snapcraft.cred

6. Load the credentials into an environment variable:

    $ export SNAPCRAFT_STORE_CREDENTIALS=$(cat ~/snapcraft.cred)

5. Upload built snaps to the Store, releasing them into all channels:

    $ for i in *.snap; do snapcraft upload --release=beta,candidate,stable $i ; done

6. Remove built snaps: `rm -f *.snap Newsboat*txt`.
