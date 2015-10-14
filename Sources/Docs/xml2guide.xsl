<?xml version="1.0" encoding="ISO-8859-1" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output encoding="ISO-8859-1" method="text"/>


<!-- ****************************************************** -->
<!-- *  XSL Stylesheet to convert XML source into AGuide  * -->
<!-- *   Written by T.Pierron 7 oct 2000. Free software   * -->
<!-- ****************************************************** -->

<xsl:template match='/manual'>

	<!-- Cover page => AG Hidden properties -->
	<xsl:apply-templates select="cover"/>

	<!-- First node, summarize all others -->
	<xsl:call-template name="MAIN"/>

	<!-- Then follows content of all other nodes -->
	<xsl:apply-templates select="content"/>

</xsl:template>

<!-- Build header of AmigaGuide file and hidden properties -->
<xsl:template match="cover">
<xsl:text>
@DATABASE <xsl:value-of select="soft"/>
@Author <xsl:apply-templates select="author"/>
$VER:<xsl:value-of select="soft"/> v<xsl:value-of select="version"/>
@(c) <xsl:value-of select="copy"/>
@Master DocAGR.xml, xml2guide.xsl
@wordwrap
@tab 8
</xsl:text>
</xsl:template>

<!-- Gathered author information -->
<xsl:template match="author">
<xsl:value-of select="firstname"/><xsl:text> </xsl:text><xsl:value-of select="lastname"/>, <xsl:value-of select="email"/></xsl:template>

<!-- Get the name of a node -->
<xsl:template name="node-name">
<xsl:choose>
	<xsl:when test="@target">"<xsl:value-of select="@target"/>"</xsl:when>
	<xsl:otherwise>"<xsl:value-of select="@title"/>"</xsl:otherwise>
</xsl:choose>
</xsl:template>

<!-- Sablotron does not support format-number() for now :-( -->
<xsl:template name="format-num">
<xsl:param name="num"/>
<xsl:choose>
	<!-- Okay, that's really crap but who cares as long as it works -->
	<xsl:when test="$num=1"><xsl:text>a</xsl:text></xsl:when>
	<xsl:when test="$num=2"><xsl:text>b</xsl:text></xsl:when>
	<xsl:when test="$num=3"><xsl:text>c</xsl:text></xsl:when>
	<xsl:when test="$num=4"><xsl:text>d</xsl:text></xsl:when>
	<xsl:when test="$num=5"><xsl:text>e</xsl:text></xsl:when>
	<xsl:otherwise><xsl:text>f</xsl:text></xsl:otherwise>
</xsl:choose>
</xsl:template>

<!-- Format the subsection name -->
<xsl:template name="format-sub">
<xsl:for-each select="subsection">
<xsl:text>
      @{" <xsl:call-template name="format-num"><xsl:with-param name="num" select="position()"/></xsl:call-template>. <xsl:value-of select="@title"/> " link <xsl:call-template name="node-name"/>}	<xsl:value-of select="@desc"/>.</xsl:text>
</xsl:for-each>
<xsl:text>
</xsl:text>
</xsl:template>

<!-- Summarize all nodes in the XML file -->
<xsl:template name="MAIN">
<xsl:text>
@node MAIN "Summary"

		@{b}<xsl:value-of select="cover/soft"/> v<xsl:value-of select="cover/version"/> User's Manual

		Written by <xsl:apply-templates select="cover/author"/>@{ub}

<xsl:apply-templates select="cover/long"/>
@{settabs 32}
<xsl:for-each select="content/chapter">
  @{"<xsl:if test="position() &lt; 10"><xsl:text> </xsl:text></xsl:if><xsl:value-of select="position()"/>. <xsl:value-of select="@title"/> " link <xsl:call-template name="node-name"/>}<xsl:text>	<xsl:value-of select="@desc"/>.
<xsl:if test="subsection"><xsl:call-template name="format-sub"/></xsl:if></xsl:text></xsl:for-each>
@endnode
</xsl:text>
</xsl:template>


<!-- Here is generated the content of each node of the AmigaGuide file -->
<xsl:template match="content">
<xsl:for-each select="chapter">
<xsl:text>
@node <xsl:call-template name="node-name"/> "<xsl:value-of select="@title"/>"


		@{b}<xsl:value-of select="position()"/>. <xsl:value-of select="@desc"/>@{ub}

<xsl:for-each select="p">@{pari 3}<xsl:apply-templates/><xsl:text>

</xsl:text>
</xsl:for-each><xsl:if test="subsection">@{settabs 32}<xsl:call-template name="format-sub"/></xsl:if>
@endnode
</xsl:text>

	<!-- Insert subchapter after main chapter definition -->

<xsl:if test="subsection">
<xsl:for-each select="subsection">
@node <xsl:call-template name="node-name"/> "<xsl:value-of select="@title"/>"


		@{b}<xsl:call-template name="format-num"><xsl:with-param name="num" select="position()"/></xsl:call-template>. <xsl:value-of select="@desc"/>@{ub}


<xsl:for-each select="p">@{pari 3}<xsl:apply-templates/><xsl:text>

</xsl:text>
</xsl:for-each>
@endnode
</xsl:for-each>
</xsl:if>
</xsl:for-each>
</xsl:template>


<!-- Handle unordered list -->
<xsl:template match="unordered-list">
<xsl:for-each select="li">@{lindent <xsl:value-of select="../@indent"/>}<xsl:text>
@{lindent <xsl:value-of select="../@indent+2"/>}@{b}*@{ub} <xsl:apply-templates/></xsl:text></xsl:for-each>@{pard}<xsl:text>
</xsl:text>
</xsl:template>

<!-- Like previous, but add just a number before -->
<xsl:template match="ordered-list">
<xsl:for-each select="li">@{lindent <xsl:value-of select="..@indent"/>}<xsl:text>
@{lindent <xsl:value-of select="../@indent+3"/>}<xsl:value-of select="position()"/>. <xsl:apply-templates/></xsl:text></xsl:for-each>@{pard}<xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="definition-list">
<xsl:for-each select="li">
<xsl:text>@{lindent <xsl:value-of select="../@indent"/>}
@{b}*@{ub} @{fg highlighttext}@{u}<xsl:apply-templates select="term"/>@{uu}@{fg text}@{lindent <xsl:value-of select="../@indent+2"/>}
<xsl:apply-templates select="def"/>
</xsl:text></xsl:for-each>@{pard}</xsl:template>

<!-- Simple anchor tag -->
<xsl:template match="link">@{"<xsl:value-of select="@name"/>" link "<xsl:value-of select="@target"/>"}</xsl:template>

<!-- Line break -->
<xsl:template match="br">
<xsl:text>
   </xsl:text>
</xsl:template>

<xsl:template match="brterm">	<!-- Line break in definition list -->
<xsl:text>@{uu}@{fg text}@{lindent <xsl:value-of select="../../../@indent+2"/>}
@{fg highlighttext}@{u}</xsl:text>
</xsl:template>

<!-- Standard styles modifiers -->
<xsl:template match="pre"><xsl:text><xsl:apply-templates/></xsl:text></xsl:template>

<xsl:template match="b">@{b}<xsl:apply-templates/>@{ub}</xsl:template>

<xsl:template match="u">@{u}<xsl:apply-templates/>@{uu}</xsl:template>

<xsl:template match="em">@{fg highlighttext}<xsl:apply-templates/>@{fg text}</xsl:template>

<xsl:template match="unsupported">@{fg Shine}@{bg Fill}Unsupported@{bg Background}@{fg Text} </xsl:template>

<!-- Disturbing characters -->
<xsl:template match="at">\@</xsl:template>
<xsl:template match="a-s">\\</xsl:template>
<xsl:template match="copy-symb">©</xsl:template>

<!-- Show special link -->
<xsl:template match="unknown-link">Example: @{"Unsupported link" rxs "QUIT"}</xsl:template>

</xsl:stylesheet>
