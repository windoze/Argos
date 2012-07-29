<?xml version="1.0" encoding="UTF-8" ?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:template match="/config">
<html>
<head>
<STYLE>
table.field { border-collapse: collapse; }
th.field, td.field { border: 1px #000 solid; padding: 3 }
</STYLE>
<title>Config</title>
</head>
<body>
    <h1>Fields</h1>
    <p/>
    <table class="field">
        <tr class="field" bgcolor="#e0e0ff">
            <th class="field">Name</th>
            <th class="field">Type</th>
            <th class="field">Multi-Value</th>
            <th class="field">Store</th>
            <th class="field">Index</th>
            <th class="field">Namespace</th>
            <th class="field">Analyzer</th>
        </tr>
        <xsl:for-each select="fields/field">
            <xsl:choose>
                <xsl:when test="position() mod 2 = 0">
                    <tr class="field" bgcolor="#f0f0f0">
                      <td class="field"><xsl:value-of select="@name"/></td>
                      <td class="field"><xsl:value-of select="@type"/></td>
                      <td class="field"><xsl:value-of select="@multi"/></td>
                      <td class="field"><xsl:value-of select="@store"/></td>
                      <td class="field"><xsl:value-of select="@index"/></td>
                      <td class="field"><xsl:value-of select="@namespace"/></td>
                      <td class="field"><xsl:value-of select="@analyzer"/></td>
                    </tr>
                </xsl:when>
                <xsl:when test="position() mod 2 = 1">
                    <tr class="field">
                      <td class="field"><xsl:value-of select="@name"/></td>
                      <td class="field"><xsl:value-of select="@type"/></td>
                      <td class="field"><xsl:value-of select="@multi"/></td>
                      <td class="field"><xsl:value-of select="@store"/></td>
                      <td class="field"><xsl:value-of select="@index"/></td>
                      <td class="field"><xsl:value-of select="@namespace"/></td>
                      <td class="field"><xsl:value-of select="@analyzer"/></td>
                    </tr>
                </xsl:when>
            </xsl:choose>
        </xsl:for-each>
    </table>
</body>
</html>
</xsl:template>
</xsl:stylesheet>
