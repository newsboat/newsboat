# Generic Image Pager
This contrib includes a scala script and a bash script which runs it.
Depending on the first argument it can be configured to use either
kitty's icat or feh for viewing the image (true = kitty, false = feh).

## How to use?
1. Assuming you move this folder (image-pager) into `~/.config/image-pager`, you can use this:
``` sh
# IMPORTANT: You usually do not want to use a relative path here!
macro i set pager "$XDG_CONFIG_HOME/image-pager/bootstrap_image_pager.sh false %f"; open; set pager internal
```
If you moved it to some other directory, make sure to reflect that in the macro.

2. Change the false to true if you are using the kitty terminal, otherwise you'd need `feh` for
the images.

3. Make sure your `image-pager/bootstrap_image_pager.sh` is executable,
cd into the `image-pager` directory and use `chmod +x bootstrap_image_pager.sh` for that.

4. Make sure you have both `scala` and `scalac` installed, then `cd` into the `image-pager` directory,
and compile into bytecode using `scalac imagePager.scala`, the bash script will take care of running
the resulting bytecode using `scala`.

5. Now you should be able to use `Comma + i` when hovering over a feed entry to display *all* images,
even thumbnails and whatnot, and images that are not obviously images from the URL (e.g. https://thumbnails.lbry.com/tszI9GrH1u0 )

## Dependencies
1. feh or kitty
2. scala and scalac
3. Newsboat

### Notes
- This script internally uses some code from another image pager written for only kitty
(available in contrib/kitty-img-pager.sh and contributed by heussd), the main issues with it
are that it only supports kitty and that it can only display *some* images.
