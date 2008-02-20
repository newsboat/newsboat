<?xml version="1.0" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="xml" indent="yes"/> 

<!--

	If you want to use the Wesnoth RSS feed (http://feed43.com/wesnoth.xml),
	then copy this file e.g. to ~/bin, install xmlstarlet, and add the following
	line to your ~/.newsbeuter/urls file:

	"filter:xmlstarlet tr ~/bin/fixwesnoth.xsl:http://feed43.com/wesnoth.xml"

-->

<xsl:template match="/rss/channel">
	<rss version="2.0">
		<channel>
			<title><xsl:value-of select="title" /></title>
			<link><xsl:value-of select="link" /></link>
			<description><xsl:value-of select="description" /></description>
			<lastBuildDate><xsl:value-of select="lastBuildDate" /></lastBuildDate>
			<xsl:for-each select="item">
			<item>
				<title><xsl:value-of select="title" /></title>
				<link><xsl:value-of select="link" /></link>
				<description><xsl:value-of select="description" /></description>
				<guid isPermaLink="false"><xsl:value-of select="title" /></guid>
			</item>
			</xsl:for-each>
		</channel>
	</rss>
</xsl:template>

</xsl:stylesheet>
