<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.1"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output method="text" encoding="iso-8859-1"/>
<xsl:strip-space elements="*"/>

<xsl:variable name="base-lower" 
    select="'0123456789abcdefghijkLmnopqrstuvwxyz'"/>

<xsl:template name="convert-from-base-10">
  <xsl:param name="number"/>
  <xsl:param name="to-base"/>
   
  <xsl:choose>
    <xsl:when test="$to-base &lt; 2 or $to-base > 36">NaN</xsl:when>
    <xsl:when test="number($number) != number($number)">NaN</xsl:when>
    <xsl:when test="$to-base = 10">
      <xsl:value-of select="$number"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="convert-from-base-10-impl">
        <xsl:with-param name="number" select="$number"/>
        <xsl:with-param name="to-base" select="$to-base"/>
     </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="convert-from-base-10-impl">
  <xsl:param name="number"/>
  <xsl:param name="to-base"/>
  <xsl:param name="result"/>
  
  <xsl:variable name="to-base-digit" 
      select="substring($base-lower,$number mod $to-base + 1,1)"/>
  
  <xsl:choose>
    <xsl:when test="$number >= $to-base">
      <xsl:call-template name="convert-from-base-10-impl">
        <xsl:with-param name="number" select="floor($number div $to-base)"/>
        <xsl:with-param name="to-base" select="$to-base"/>
        <xsl:with-param name="result" select="concat($to-base-digit,$result)"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="concat($to-base-digit,$result)"/>
      </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="id"/>

<xsl:template match="/block/block[@name='PARTEI']/int[translate(@name,'PUNKTE','punkte')='punkte']">

  <xsl:text>ERESSEA </xsl:text>
  <xsl:variable name="faction" select="../id"/>
  <xsl:variable name="recruit_cost" select="../int[translate(@name,'REKUTINGSO','rekutingso')='rekrutierungskosten']/@value"/>
  <xsl:call-template name="convert-from-base-10">
    <xsl:with-param name="number" select="$faction"/>
    <xsl:with-param name="to-base" select="'36'"/>
  </xsl:call-template>
  <xsl:text> &quot;hier_passwort_einfügen&quot;&#xA;</xsl:text>
  <xsl:text>; ECHECK -l -w4 -r</xsl:text>
  <xsl:value-of select="$recruit_cost"/>
  <xsl:text> -v4.01&#xA;</xsl:text>

  <xsl:for-each select="../../block[@name='REGION']">
    <xsl:variable name="region_crname" select="string[translate(@name,'NAME','name')='name']/@value"/>
    <xsl:variable name="region_name">
      <xsl:choose>
        <xsl:when test="$region_crname">
          <xsl:value-of select="$region_crname"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="string[translate(@name,'TERAIN','terain')='terrain']/@value"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="salary" select="int[translate(@name,'LOHN','lohn')='lohn']/@value"/>

    <xsl:for-each select="block[@name='EINHEIT']/int[translate(@name,'PARTEI','partei')='partei' and $faction = @value]">
      <xsl:variable name="name" select="../string[translate(@name,'NAME','name')='name']/@value"/>
      <xsl:variable name="number" select="../int[translate(@name,'ANZHL','anzhl')='anzahl']/@value"/>
      <xsl:variable name="ship" select="../int[translate(@name,'SCHIF','schif')='schiff']/@value"/>
      <xsl:variable name="money" select="../block[@name='GEGENSTAENDE']/int[translate(@name,'SILBER','silber')='silber']/@value"/>

      <xsl:if test="position() = 1">
        <xsl:text>&#xA;</xsl:text>
        <xsl:text>REGION </xsl:text>
        <xsl:for-each select="../../id">
          <xsl:if test="position() &gt; 1">
            <xsl:text>,</xsl:text>
          </xsl:if>
          <xsl:value-of select="."/>
        </xsl:for-each>
        <xsl:text> ; </xsl:text>
        <xsl:value-of select="$region_name"/>
        <xsl:text>&#xA;; ECheck Lohn </xsl:text>
        <xsl:value-of select="$salary"/>
        <xsl:text>&#xA;&#xA;</xsl:text>
      </xsl:if>

      <xsl:text>EINHEIT </xsl:text>
      <xsl:call-template name="convert-from-base-10">
        <xsl:with-param name="number" select="../id"/>
        <xsl:with-param name="to-base" select="'36'"/>
      </xsl:call-template>
      <xsl:text>;&#x09;&#x09;</xsl:text>
      <xsl:value-of select="$name"/>
      <xsl:text> [</xsl:text>
      <xsl:value-of select="$number"/>,<xsl:value-of select="format-number(concat('0',$money), '#######')"/>
      <xsl:text>$</xsl:text>
      <xsl:if test="$ship">
        <xsl:text>,s</xsl:text>
        <xsl:call-template name="convert-from-base-10">
          <xsl:with-param name="number" select="$ship"/>
          <xsl:with-param name="to-base" select="'36'"/>
        </xsl:call-template>
      </xsl:if>
      <xsl:text>]</xsl:text>
      <xsl:text>&#xA;</xsl:text>

      <xsl:for-each select="../block[@name='COMMANDS']/entry">
        <xsl:text>  </xsl:text>
        <xsl:value-of select="translate(@value,'\','')"/>
        <xsl:text>&#xA;</xsl:text>
      </xsl:for-each>

    </xsl:for-each>

  </xsl:for-each>
  <xsl:text>NÄCHSTER&#xA;</xsl:text>

</xsl:template> 
</xsl:stylesheet>
