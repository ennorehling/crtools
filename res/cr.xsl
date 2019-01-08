<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">  
<xsl:output method="text"/>
<xsl:strip-space elements="*"/>

<xsl:template match="int">
<xsl:value-of select="@value"/>;<xsl:value-of select="@name"/><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="list">
<xsl:for-each select="int">
<xsl:if test="position()!='1'"><xsl:text> </xsl:text></xsl:if>
<xsl:value-of select="@value"/>
</xsl:for-each>;<xsl:value-of select="@name"/><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="string">"<xsl:value-of select="@value"/>";<xsl:value-of select="@name"/><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="id"/>

<xsl:template match="entry">"<xsl:value-of select="@value"/>"
</xsl:template>

<xsl:template match="block">
<xsl:value-of select="@name"/><xsl:for-each select="id"><xsl:text> </xsl:text><xsl:value-of select="."/></xsl:for-each><xsl:text>
</xsl:text>
<xsl:apply-templates/>
</xsl:template> 

</xsl:stylesheet>
