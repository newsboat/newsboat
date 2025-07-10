## Filter annoying substack subscription prompts.

## About

Filter annoying Substack subscription prompts using a [Newsboat filter](https://Newsboat.org/releases/2.30.1/docs/Newsboat.html#_scripts_and_filters_snownews_extensions).

## Requirements

- [xmllint](https://linux.die.net/man/1/xmllint) (e.g., in Debian `sudo apt install libxml2-utils`). Necessary because sed does greedy matching
- sed
- sh / bash

## Installation and usage

### Download scripts

Download the `fltr-substack.sh`, and optionally the `make-fltr-substack.sh` scripts.

e.g.,

```
mkdir -p "~/.config/Newsboat"
cd "~/.config/Newsboat"
wget https://raw.githubusercontent.com/Newsboat/Newsboat/master/contrib/fltr-substack/fltr-substack.sh
wget https://raw.githubusercontent.com/Newsboat/Newsboat/master/contrib/fltr-substack/make-fltr-substack.sh
```

### Add filters

#### Add filters automatically (may break stuff!, make backups)

Run `make-fltr-substack.sh` in the same directory which contains your `urls` file.

It will add a filter to all `.substack.com` domains. Though note that there are also substacks which are hosted on their own domain; these would have to be adjusted manually.

#### Add filters manually

Substitute a substack feed in your `urls` file:

```
https://example.com/feed
https://forecasting.substack.com/feed
https://blahblah.com/feed
```

with

```
https://example.com/feed
filter:~/.config/Newsboat/fltr-substack.sh:https://forecasting.substack.com/feed
https://blahblah.com/feed
```

(where `~/.config/Newsboat/fltr-substack.sh` is the location of the script you downloaded )

## Roadmap, guarantees

These scripts are pretty untested. I expect they will break when substack updates the way it displays its subscription prompt. Contributions are welcome.

