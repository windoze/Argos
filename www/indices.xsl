<?xml version="1.0" encoding="UTF-8" ?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:template match="/indices">
<html>
<head>
<STYLE>
table.field { border-collapse: collapse; }
th.field, td.field { border: 1px #000 solid; padding: 3 }
</STYLE>
<title>Indices</title>
</head>
<body>
    <h1>Indices</h1>
    <hr/>
    <table border="0">
        <tr>
            <td align="right" style="padding-right:10px;">Count:</td>
            <td><xsl:value-of select="@count"/></td>
        </tr>
    </table>
    <table class="field">
        <tr class="field" bgcolor="#e0e0ff">
            <th class="field">Name</th>
            <th class="field">Path</th>
            <th class="field">Documents</th>
            <th class="field">Erased</th>
            <th class="field">Config</th>
        </tr>
        <xsl:for-each select="index">
            <xsl:variable name="idxname" select="@name" />
            <xsl:choose>
                <xsl:when test="position() mod 2 = 0">
                    <tr class="field" bgcolor="#f0f0f0">
                        <td class="field"><xsl:value-of select="@name"/></td>
                        <td class="field"><xsl:value-of select="@path"/></td>
                        <td class="field"><xsl:value-of select="@active"/></td>
                        <td class="field"><xsl:value-of select="@erased"/></td>
                        <td class="field"><a href="/{$idxname}/config.xml">config.xml</a></td>
                    </tr>
                </xsl:when>
                <xsl:when test="position() mod 2 = 1">
                    <tr class="field">
                        <td class="field"><xsl:value-of select="@name"/></td>
                        <td class="field"><xsl:value-of select="@path"/></td>
                        <td class="field"><xsl:value-of select="@active"/></td>
                        <td class="field"><xsl:value-of select="@erased"/></td>
                        <td class="field"><a href="/{$idxname}/config.xml">config.xml</a></td>
                    </tr>
                </xsl:when>
            </xsl:choose>
        </xsl:for-each>
    </table>
</body>
</html>
</xsl:template>
</xsl:stylesheet>
