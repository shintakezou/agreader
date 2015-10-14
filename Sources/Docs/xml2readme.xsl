<?xml version="1.0" encoding="ISO-8859-1" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output encoding="ISO-8859-1"/>


<!-- ****************************************************** -->
<!-- *  XSL Stylesheet to convert XML source into README  * -->
<!-- *  Written by T.Pierron 10 oct 2000. Free software.  * -->
<!-- ****************************************************** -->

<xsl:template match='/manual'>
<xsl:apply-templates select="cover"/>
<xsl:apply-templates select="readme"/>
</xsl:template>

<!-- Header fields of readme file -->
<xsl:template match='cover'>
<xsl:text>
Short:    <xsl:value-of select="short"/>
Uploader: <xsl:value-of select="author/email"/>
Type:     <xsl:value-of select="type"/>
Version:  <xsl:value-of select="version"/>
Requires: <xsl:value-of select="requirement"/>

</xsl:text>
</xsl:template>

<!-- Print content of readme -->
<xsl:template match="readme">
<xsl:for-each select="p"><xsl:apply-templates/>
<xsl:text><!-- A blank line -->

</xsl:text></xsl:for-each>
</xsl:template>

</xsl:stylesheet>
