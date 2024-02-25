#!/usr/bin/env php
<?php

declare(strict_types=1);

$usage = <<<EOL
usage: twtxt2rss [author]

Read a twtxt feed from stdin then print it as a RSS feed on stdout.
EOL;

if ($argc !== 2) {
    fwrite(STDERR, sprintf("%s\n", $usage));
    exit(1);
}

$author = $argv[1];

class Twt
{
    private DateTime $date;
    private string $message;

    public function __construct(DateTime $date, string $message)
    {
        $message = htmlspecialchars($message);
        $message = $this->HTMLize($message);

        $this->message = $message;
        $this->date = $date;
    }

    private function HTMLizeMentions(string $message): string
    {
        $pcre = '/(' . htmlspecialchars('@<') . '.+?' . htmlspecialchars('>') . ')/';
        $replacement = '<b>$1</b>';
        $htmlizedMessage = preg_replace($pcre, $replacement, $message);

        return $htmlizedMessage ? $htmlizedMessage : $message;
    }

    private function HTMLize(string $message): string
    {
        // TODO: Find a way to HTMLize links that doesn't conflict with mentions

        $message = $this->HTMLizeMentions($message);

        return $message;
    }

    public function getDate(): DateTime
    {
        return $this->date;
    }

    public function getMessage(): string
    {
        return $this->message;
    }
}

$twts = [];

while ($line = fgets(STDIN)) {
    $line = trim($line);
    $line = explode("\t", $line, 2);

    if (count($line) !== 2)
        continue;

    $date = DateTime::createFromFormat(DateTime::RFC3339, $line[0]);
    $message = $line[1];

    if (!$date)
        continue;

    $twts[] = new Twt($date, $message);
}

?>
<?= "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"?>

<rss version="2.0">
    <channel>
        <title><?= sprintf('%s’s twtxt feed', $author) ?></title>
        <description><?= sprintf('%s’s twtxt feed', $author) ?></description>
        <link><?= 'http://localhost' ?></link>
<?php foreach ($twts as $twt) : ?>
    <item>
        <title><?= $twt->getDate()->format('d-m-Y H:i:s') ?></title>
        <description><![CDATA[<p><?= $twt->getMessage() ?></p>]]></description>
        <pubDate><?= $twt->getDate()->format(DateTime::RSS) ?></pubDate>
    </item>
<?php endforeach ?>
    </channel>
</rss>
