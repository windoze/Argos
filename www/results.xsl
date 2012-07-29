<?xml version="1.0" encoding="UTF-8" ?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:template match="/">
<html>
<head>
<STYLE>
table.result { border-collapse: collapse; }
th.result, td.result { border: 1px #000 solid; padding: 3 }
</STYLE>
<title>Results</title>
</head>
<body>
    <xsl:for-each select="result/histograms">
        <h1>Histograms</h1>
        <hr noshade="noshade" />
        <xsl:for-each select="histogram">
            <p/>
            <table border="0">
                <tr>
                    <td align="right" style="padding-right:10px;">Key:</td>
                    <td><xsl:value-of select="@key"/></td>
                </tr>
                <tr>
                    <td align="right" style="padding-right:10px;">Entry count:</td>
                    <td><xsl:value-of select="@count"/></td>
                </tr>
            </table>
            <table class="result">
                <tr class="result" bgcolor="#e0e0ff">
                    <th class="result" bgcolor="#e0e0ff"><xsl:value-of select="@key"/></th>
                    <xsl:for-each select="item">
                        <th class="result"><xsl:value-of select="@key"/></th>
                    </xsl:for-each>
                </tr>
                <tr class="result">
                    <th class="result" bgcolor="#e0e0ff">Count</th>
                    <xsl:for-each select="item">
                        <th class="result"><xsl:value-of select="@count"/></th>
                    </xsl:for-each>
                </tr>
            </table>
        </xsl:for-each>
    </xsl:for-each>
    <h1>Results</h1>
    <hr noshade="noshade" />
    <table border="0">
        <tr>
            <td align="right" style="padding-right:10px;">Total result:</td>
            <td><xsl:value-of select="result/items/@total"/></td>
        </tr>
        <tr>
            <td align="right" style="padding-right:10px;">Returned result:</td>
            <td><xsl:value-of select="result/items/@returned"/></td>
        </tr>
    </table>
    <p/>
    <table class="result">
        <tr class="result" bgcolor="#e0e0ff">
            <xsl:for-each select="result/items/*[1]/field">
                <th class="result"><xsl:value-of select="@name"/></th>
            </xsl:for-each>
        </tr>
        <xsl:for-each select="result/items/item">
            <xsl:choose>
                <xsl:when test="position() mod 2 = 1">
                    <tr class="result" bgcolor="#f0f0f0">
                        <xsl:for-each select="field">
                          <td class="result"><xsl:value-of select="@value"/></td>
                        </xsl:for-each>
                    </tr>
                </xsl:when>
                <xsl:when test="position() mod 2 = 0">
                    <tr class="result">
                        <xsl:for-each select="field">
                          <td class="result"><xsl:value-of select="@value"/></td>
                        </xsl:for-each>
                    </tr>
                </xsl:when>
            </xsl:choose>
        </xsl:for-each>
    </table>
</body>
</html>
</xsl:template>
</xsl:stylesheet>
