<!--
Replaces newline characters in description field of Slashdot's RDF/RSS feed with html linebreaks (`<br>`)
Uses xsltproc (part of libxslt)

Can be used with a filter url, for example:
"filter:xsltproc /usr/share/doc/Newsboat/contrib/slashdot-replace-newlines.xslt -:https://rss.slashdot.org/Slashdot/slashdotMain"
-->
<xsl:stylesheet version="1.0"
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 xmlns:str="http://exslt.org/strings"
 xmlns:purl="http://purl.org/rss/1.0/">
    <xsl:template match="node()|@*">
        <xsl:copy>
            <xsl:apply-templates select="node()|@*"/>
        </xsl:copy>
    </xsl:template>

    <xsl:template match="purl:description">
        <xsl:copy>
            <!-- '&#10;' encodes a newline character (\n) -->
            <xsl:value-of select="str:replace(node(), '&#10;', '&lt;br&gt;')"/>
        </xsl:copy>
    </xsl:template>
</xsl:stylesheet>
