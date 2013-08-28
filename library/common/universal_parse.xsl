<!-- 
 Copyright (c) 2013 Juniper Networks, Inc. All rights reserved. 
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html" indent="yes" doctype-public="-//W3C//DTD HTML 4.0 Transitional//EN" />
<xsl:variable name="snhreq" select="/child::node()/@type"/>

<xsl:template match="*">
</xsl:template>

<xsl:template match="text()">
</xsl:template>
<xsl:template match="@*">
</xsl:template>

<xsl:template match="/"><html><head>
<link href="style.css" rel="stylesheet" type="text/css"/>
<script src="jquery-1.8.1.js"></script>
<script src="jquery.collapse.js"></script>
<script src="jquery.collapse_storage.js"></script>
<title>HTTP Introspect</title>
</head><body>

<xsl:choose><xsl:when test="$snhreq = 'rlist'">
<xsl:for-each select="*">
<h1>Module: <xsl:value-of select="name()"/></h1>
</xsl:for-each>
<table border="1"><tr><th>Requests</th></tr>
<xsl:for-each select="*">
<xsl:for-each select="*">
<xsl:variable name="reqname" select="name()"/>
<tr><td>
<a href="#Snh_{$reqname}"><xsl:value-of select="$reqname"/> </a></td></tr>
</xsl:for-each></xsl:for-each>
</table><br/>
</xsl:when></xsl:choose>

<table border="1">
<xsl:for-each select="*">
<xsl:choose>
<xsl:when test="attribute::type[.='sandesh']">
<tr>
<th><xsl:value-of select="name()"/></th></tr><tr>
<td><xsl:apply-templates select="attribute::type[.='sandesh']"/></td></tr>
</xsl:when>
<xsl:when test="attribute::type[.='rlist']">
<tr><td><xsl:apply-templates select="attribute::type[.='rlist']"/></td></tr>
</xsl:when>
<xsl:otherwise>
<tr><td><xsl:apply-templates select="attribute::type[.='slist']"/></td></tr>
</xsl:otherwise>
</xsl:choose>
</xsl:for-each>
</table>
</body></html></xsl:template>


<xsl:template match="@type[.='rlist']">
<table border="1">
<xsl:for-each select="../*">
<xsl:choose>
<xsl:when test="@type = 'sandesh'">
<xsl:variable name="reqx" select="name(.)"/>
<tr><th><h3 id="Snh_{$reqx}"><xsl:value-of select="name()"/></h3></th></tr><tr><td>
<div class="definition">
<form action="Snh_{$reqx}" method="get">
<xsl:apply-templates select="@type[.='sandesh']"/>
<button type="submit">Send</button>
</form></div>
</td></tr>
</xsl:when>
</xsl:choose>
</xsl:for-each>
</table>
</xsl:template>


<xsl:template match="@type[.='slist']">
<table border="1">
<xsl:for-each select="../*">
<xsl:choose>
<xsl:when test="@type = 'sandesh'">
<tr><th><h3><xsl:value-of select="name()"/></h3></th></tr><tr><td>
<xsl:apply-templates select="@type[.='sandesh']"/>
</td></tr>
</xsl:when>
</xsl:choose>
</xsl:for-each>
</table>
</xsl:template>


<xsl:template match="element">
<xsl:choose>
<xsl:when test="$snhreq = 'rlist'">
<xsl:variable name="vary" select="name(..)"/>
<tr><td style="color:blue"><input type="text" name="{$vary}"/></td></tr>
</xsl:when>
<xsl:otherwise>
<tr><td style="color:blue"><pre><xsl:value-of select="."/></pre></td></tr>
</xsl:otherwise>
</xsl:choose>
</xsl:template>

<xsl:template match="@type[.='list']">
<xsl:for-each select="../*">
<xsl:choose>
<xsl:when test="@type = 'struct'">
<xsl:apply-templates select="@type[.='struct']"/>
</xsl:when>
<xsl:otherwise>
<xsl:apply-templates select="element"/>
</xsl:otherwise>   
</xsl:choose>
</xsl:for-each>
</xsl:template>



<xsl:template match="@type[.='struct']">

<xsl:choose>
<xsl:when test="name(..) = 'list'">
<xsl:for-each select="../*[position() =1]">
<tr><xsl:for-each select="*">
<th><xsl:value-of select="name()"/>
<xsl:if test="$snhreq = 'rlist'"> (<xsl:value-of select="@type"/>)</xsl:if></th>
</xsl:for-each></tr>
</xsl:for-each>
<xsl:for-each select="../*">
<tr><xsl:for-each select="*">

<xsl:choose>
<xsl:when test="@type[.='struct'] | @type[.='list']">

<xsl:choose>
<xsl:when test="$snhreq = 'rlist'">
<td><table border="1">
<xsl:apply-templates select="@type"/>
</table></td>
</xsl:when>
<xsl:otherwise>
<xsl:variable name="tbname" select="name()"/>
<xsl:variable name="count" select="position()"/>
<xsl:variable name="num"><xsl:number/></xsl:variable>
<td><div data-collapse="accordion persist"><h5><xsl:value-of select="name()"/></h5>
<table id="{generate-id()}-{$tbname}-{$count}" border="1">
<!--KKK--><xsl:apply-templates select="@type"/>
</table></div></td>
</xsl:otherwise>
</xsl:choose>

</xsl:when>
<xsl:otherwise>
<xsl:apply-templates select="@type"/>
</xsl:otherwise>
</xsl:choose>

</xsl:for-each></tr>
</xsl:for-each>
</xsl:when>
<xsl:otherwise>
<xsl:for-each select="../*">
<xsl:for-each select="*">
<tr><td>
<xsl:value-of select="name()"/>
<xsl:if test="$snhreq = 'rlist'"> (<xsl:value-of select="@type"/>)</xsl:if></td>
<xsl:choose>
<xsl:when test="@type[.='struct'] | @type[.='list']">

<xsl:choose>
<xsl:when test="$snhreq = 'rlist'">
<td><table border="1">
<xsl:apply-templates select="@type"/>
</table></td>
</xsl:when>
<xsl:otherwise>
<xsl:variable name="tbname" select="name()"/>
<xsl:variable name="count" select="position()"/>
<xsl:variable name="num"><xsl:number/></xsl:variable>
<td><div data-collapse="accordion persist"><h5><xsl:value-of select="name()"/></h5>
<table id="{generate-id()}-{$tbname}-{$count}" border="1">
<xsl:apply-templates select="@type"/>
</table></div></td>
</xsl:otherwise>
</xsl:choose>

</xsl:when>
<xsl:otherwise>
<xsl:apply-templates select="@type"/>
</xsl:otherwise>
</xsl:choose>
</tr>
</xsl:for-each>
</xsl:for-each>
</xsl:otherwise>
</xsl:choose>
</xsl:template>


<xsl:template match="@type[.='sandesh']">
<table border="1">
<xsl:choose>
<!--xsl:when test="(../../@type = 'slist') or (../../@type = 'rlist')"-->
<xsl:when test="(../../@type = 'slist')">
<tr><xsl:for-each select="../*">
<th><xsl:value-of select="name()"/></th>
</xsl:for-each></tr>
<tr><xsl:for-each select="../*">

<xsl:choose>
<xsl:when test="@type[.='struct'] | @type[.='list']">
<td><table border="1">
<!--NNN--><xsl:apply-templates select="@type[.='struct'] | @type[.='list']"/>
</table></td></xsl:when>
<xsl:otherwise>
<xsl:apply-templates select="@type"/>
</xsl:otherwise>
</xsl:choose>

</xsl:for-each></tr>
</xsl:when>
<xsl:otherwise>
<xsl:for-each select="../*">
<xsl:choose>
<xsl:when test="@type[.='struct'] | @type[.='list']">

<tr><td><xsl:value-of select="name()"/>
<xsl:if test="$snhreq = 'rlist'"> (<xsl:value-of select="@type"/>)</xsl:if></td><td>
<!--div data-collapse="accordion persist"-->
<div>
<!--h4><xsl:value-of select="name()"/></h4--><!--OOO--><table border="1">
<xsl:apply-templates select="@type[.='struct']"/>
<xsl:apply-templates select="@type[.='list']"/>
</table></div>
</td></tr>

</xsl:when>
<xsl:otherwise>
<tr><td><xsl:value-of select="name()"/>
<xsl:if test="$snhreq = 'rlist'"> (<xsl:value-of select="@type"/>)</xsl:if></td>
<xsl:apply-templates select="@type"/>
</tr>
</xsl:otherwise>
</xsl:choose>
</xsl:for-each>
</xsl:otherwise>
</xsl:choose>
</table>
</xsl:template>


<xsl:template match="@type">
<xsl:choose>
<xsl:when test="../@link">
<xsl:variable name="linkx" select="../@link"/>
<xsl:variable name="valuex" select=".."/>
<td style="color:blue"><pre><a href="Snh_{$linkx}?_x={$valuex}"><xsl:value-of select=".."/></a></pre></td>
</xsl:when>
<xsl:otherwise>

<xsl:choose>
<xsl:when test="$snhreq = 'rlist'">
<xsl:variable name="varx" select="name(..)"/>
<td style="color:blue"><input type="text" name="{$varx}"/></td>
</xsl:when>
<xsl:otherwise>
<td style="color:blue"><pre><xsl:value-of select=".."/></pre></td>
</xsl:otherwise>
</xsl:choose>
</xsl:otherwise>
</xsl:choose>
</xsl:template>

</xsl:stylesheet>
