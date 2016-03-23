#!/bin/sh
#####################################################################
# convert the html files to plain text
#
# You may pass the original file as an argument, or pass it by pipe.
#
# Copyright 2014 Yunhui Fu
# License: GPL v3.0 or later
#####################################################################

EXEC_HTMLESC=$(which htmlescape)
if [ "${EXEC_HTMLESC}" = "" ]; then
    EXEC_HTMLESC="../src/htmlescape"
fi

if [ "$1" = "" ]; then
	# special codes: http://www.web2generators.com/html/entities
	sed \
		-e ':a;N;$!ba;s/\r/\\r/gI' \
		-e ':a;N;$!ba;s/\n/\\n/gI' \
		-e ':a;N;$!ba;s/\t/\t/gI' \
		-e 's|[ ]+| |gI' \
		-e 's|<[ ]*head[^>]*>|<head>|gI' \
		-e 's|<[ ]*/[ ]*head[^>]*>|</head>|gI' \
		-e 's|<head>.*</head>||gI' \
		-e 's|<[ ]*script[^>]*>|<script>|gI' \
		-e 's|<[ ]*/[ ]*script[^>]*>|</script>|gI' \
		-e 's|<script>.*</script>||gI' \
		-e 's|<[ ]*style[^>]*>|<style>|gI' \
		-e 's|<[ ]*/[ ]*style[^>]*>|</style>|gI' \
		-e 's|<[ ]*td[^>]*>|\t|gI' \
		-e 's|<[ ]*br[^>]*>|\n|gI' \
		-e 's|<[ ]*li[^>]*>|\n|gI' \
		-e 's|<[ ]*div[^>]*>|\n\n|gI' \
		-e 's|<[ ]*tr[^>]*>|\n\n|gI' \
		-e 's|<[ ]*p[^>]*>|\n\n|gI' \
		-e 's|<[^>]*>||gI' \
		-e 's|&bull;| * |gI' \
		-e 's|&lsaquo;|<|gI' \
		-e 's|&rsaquo;|>|gI' \
		-e 's|&trade;|(tm)|gI' \
		-e 's|&frasl;|/|gI' \
		-e 's|&lt;|<|gI' \
		-e 's|&gt;|>|gI' \
		-e 's|&copy;|©|gI' \
		-e 's|&reg;|®|gI' \
		-e 's|&quot;|\"|gI' \
		-e "s|&apos;|'|gI" \
		-e 's|&amp;|&|gI' \
		-e 's|&nbsp;| |gI' \
		-e 's|&iexcl;|¡|gI' \
		-e 's|&cent;|¢|gI' \
		-e 's|&pound;|£|gI' \
		-e 's|&curren;|¤|gI' \
		-e 's|&yen;|¥|gI' \
		-e 's|&brvbar;|¦|gI' \
		-e 's|&sect;|§|gI' \
		-e 's|&uml;|¨|gI' \
		-e 's|&ordf;|ª|gI' \
		-e 's|&laquo;|«|gI' \
		-e 's|&not;|¬|gI' \
		-e 's|&shy;|§|gI' \
		-e 's|&macr;|¯|gI' \
		-e 's|&deg;|°|gI' \
		-e 's|&plusmn;|±|gI' \
		-e 's|&sup2;|²|gI' \
		-e 's|&sup3;|³|gI' \
		-e 's|&acute;|´|gI' \
		-e 's|&micro;|µ|gI' \
		-e 's|&para;|¶|gI' \
		-e 's|&middot;|·|gI' \
		-e 's|&cedil;|¸|gI' \
		-e 's|&sup1;|¹|gI' \
		-e 's|&ordm;|º|gI' \
		-e 's|&raquo;|»|gI' \
		-e 's|&frac14;|¼|gI' \
		-e 's|&frac12;|½|gI' \
		-e 's|&frac34;|¾|gI' \
		-e 's|&iquest;|¿|gI' \
		-e 's|&times;|×|gI' \
		-e 's|&divide;|÷|gI' \
	| ${EXEC_HTMLESC}

	#| sed -r "s@([^0-9]*)&#([0-9]*);([.]*)@echo -n \1 \;echo \2 | awk '{printf(\"%c\",\$1)}' \; echo -n \3@ge" \
	#| sed -e 's|echo -n ||g' \
	#| sed -e 's/^[ \t]*//' | sed -e 's/[ \t]*$//' | awk -v RS='\n\n' 1

else
	if [ "$1" = "-t" ]; then
		# test
		#echo "<html>< head a='same' >\r\n\r\n\t\r\n\t\n\r    \r\t\r< / head >\t<body>           <script lang=''>a;dlkfs;fkjf</script>吳&#37390;，<br/>乃<BR>古劍<br />也<style a='b'>d adf</style><br/><del>Deleted</del><br/><ins>Inserted</ins><br/><table><tr><td>a</td><td>b</td></tr><tr><td>a2</td><td>b2</td></tr>&apos; 5&times;5</body></html>" | $0
		#echo "11X24；1579&#39029;；&#22270;200幅<br />" | $0
		#echo "11X24；1579&#39029;" | $0
		echo "11X24；1579&#39029;" | sed -r "s@([.]*)&#([0-9]*);([.]*)@echo -n \1 \;echo \2 | awk '{printf(\"%c\",\$1)}' \; echo -n \3 \; echo @ge" | sed -e 's|echo -n ||g'

		echo
	else
		cat "$1" | $0
	fi
fi

#echo "abc&#37390;def&#37390;ghi&#37390;jkl" | sed -r "s@([^0-9]*)&#([0-9]*);([.]*)@echo -n \1 \;echo \2 | awk '{printf(\"%c\",\$1)}' \; echo -n \3@ge" | sed -e 's|echo -n ||g'
