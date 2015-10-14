<?xml version="1.0" encoding="ISO-8859-1" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output encoding="ISO-8859-1" method="html"/>


<!-- **************************************************** -->
<!-- *  XSL Stylesheet to convert XML source into HTML.  * -->
<!-- *  Written by T.Pierron 25 oct 2000. Free software  * -->
<!-- **************************************************** -->

<xsl:template match='/manual'>
<html>
	<!-- Cover page -->
	<xsl:comment> Automatically generated from xml source. </xsl:comment>
	<head><xsl:apply-templates select="cover"/></head>

	<!-- First node, summarize all others -->
	<xsl:call-template name="MAIN"/>

	<!-- Then follows content of all other nodes -->
	<xsl:apply-templates select="content"/>
</html>
</xsl:template>

<!-- Some information about what's contain in -->
<xsl:template match="cover">
<title><xsl:value-of select="soft"/> User's Manual</title>
</xsl:template>


<xsl:template match="author">
<xsl:value-of select="firstname"/><xsl:text> </xsl:text><xsl:value-of select="lastname"/>, <xsl:value-of select="email"/></xsl:template>

<!-- Format a chapter header -->
<xsl:template name="node-name">
<xsl:choose>
	<xsl:when test="@target"><xsl:value-of select="@target"/></xsl:when>
	<xsl:otherwise><xsl:value-of select="@title"/></xsl:otherwise>
</xsl:choose>
</xsl:template>

<!-- Summarize all nodes in the XML file -->
<xsl:template name="MAIN">
<center>
	<h1><xsl:value-of select="cover/soft"/> v<xsl:value-of select="cover/version"/> User's Manual</h1>
	<h1>Written by <xsl:apply-templates select="cover/author"/></h1>
	<h3><xsl:apply-templates select="cover/copy"/></h3>
</center>

<p><xsl:apply-templates select="cover/long"/></p>

<center><h1>SUMMARY</h1></center>
<table width="75%">
<tr><td>
<font size="+3"><ol>
<!-- First column: anchors -->
<xsl:for-each select="content/chapter">
<li>
	<b><xsl:text disable-output-escaping="yes">
		&lt;a href="#<xsl:call-template name="node-name"/>"&gt;</xsl:text>
		<xsl:value-of select="@title"/>
		<xsl:text disable-output-escaping="yes">&lt;/a&gt;</xsl:text>
	</b>
	<font size="+1"><ol type="a">
	<xsl:for-each select="subsection">
	<li><b><a href="#{@target}"><xsl:value-of select="@title"/></a></b></li>
	</xsl:for-each>
	</ol></font>
</li>
</xsl:for-each>
</ol></font>
</td><td align="right">

<!-- Second column chapter's description -->
<xsl:for-each select="content/chapter">
	<font size="+3">&#160;</font><b><xsl:value-of select="@desc"/></b><br/>
	<xsl:for-each select="subsection">
		<font size="+1">&#160;</font><b><xsl:value-of select="@desc"/></b><br/>
	</xsl:for-each>
</xsl:for-each>
</td></tr>
</table>
</xsl:template>

	<!-- Here is generated the content of the HTML file -->

<!-- Generate anchor reference -->
<xsl:template name="gen-anchor">
<xsl:text disable-output-escaping="yes">
&lt;a name="<xsl:call-template name="node-name"/>"/&gt;</xsl:text>
</xsl:template>

<!-- Format the subsection title and content -->
<xsl:template name="format-sub">
<xsl:param name="section"/>
<xsl:for-each select="subsection">
<xsl:call-template name="gen-anchor"/>
<h3 align="center"><xsl:value-of select="$section"/>.<xsl:value-of select="position()"/>&#160;<xsl:value-of select="@title"/></h3>

<xsl:for-each select="p">		<!-- Yes that's all :-) -->
<p><xsl:apply-templates/></p>
</xsl:for-each>

</xsl:for-each>
</xsl:template>

<!-- Format main chapter -->
<xsl:template match="content">
<xsl:for-each select="chapter">
<xsl:call-template name="gen-anchor"/>
<h2 align="center"><xsl:value-of select="position()"/>. <xsl:value-of select="@desc"/></h2>

<xsl:for-each select="p">
<p><xsl:apply-templates/></p>
</xsl:for-each>

<!-- Add next, subsections of file -->
<xsl:if test="subsection">
<xsl:call-template name="format-sub">
<xsl:with-param name="section" select="position()"/>
</xsl:call-template>
</xsl:if>
</xsl:for-each>
</xsl:template>

<!-- Handle unordered list -->
<xsl:template match="unordered-list">
<ul>
<xsl:for-each select="li">
	<li><xsl:apply-templates/></li>
</xsl:for-each>
</ul>
</xsl:template>

<!-- Like previous, but add just a number before -->
<xsl:template match="ordered-list">
<ol>
<xsl:for-each select="li">
	<li><xsl:apply-templates/></li>
</xsl:for-each>
</ol>
</xsl:template>

<xsl:template match="definition-list">
<dl>
<xsl:for-each select="li">
	<dt><u><font color="#802050"><xsl:apply-templates select="term"/></font></u></dt>
   <dd><xsl:apply-templates select="def"/></dd>
</xsl:for-each>
</dl>
</xsl:template>

<!-- Simple anchor tag -->
<xsl:template match="link"><a href="#{@target}"><xsl:value-of select="@name"/></a></xsl:template>

<!-- Line breaking in definition list: let HTML manage BR -->
<xsl:template match="br"><xsl:text> </xsl:text></xsl:template>

<xsl:template match="brterm">	<!-- Line break in defined term -->
<br/>
</xsl:template>

<!-- Standard styles modifiers -->
<xsl:template match="pre"><pre><xsl:text><xsl:apply-templates/></xsl:text></pre></xsl:template>

<xsl:template match="b"><b><xsl:apply-templates/></b></xsl:template>

<xsl:template match="u"><u><xsl:apply-templates/></u></xsl:template>

<xsl:template match="em"><b><font color="#802050"><xsl:apply-templates/></font></b></xsl:template>

<xsl:template match="unsupported"><b><font color="red">Unsupported</font></b>&#160;</xsl:template>

<xsl:template match="link"><a href="#{@ref}"><xsl:apply-templates/></a></xsl:template>

<!-- Disturbing characters -->
<xsl:template match="at">@</xsl:template>
<xsl:template match="a-s">\</xsl:template>
<xsl:template match="copy-symb"><xsl:text disable-output-escaping="yes">&amp;copy;</xsl:text></xsl:template>

</xsl:stylesheet>

