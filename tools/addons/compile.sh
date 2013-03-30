#!/bin/sh

if [ -z "$3" ]; then
    echo "usage: $0 svn_path topdir addons_path [separate]"
    echo "eg. $0 ~/src/grass-addons/grass7/ ~/src/grass_trunk/dist.x86_64-unknown-linux-gnu ~/.grass7/addons"
    exit 1
fi

SVN_PATH="$1"
TOPDIR="$2"
ADDON_PATH="$3"
GRASS_VERSION=`echo $ADDON_PATH | cut -d'/' -f6 | sed 's/grass//g'`
INDEX_FILE="ALL.html"

if [ ! -d "$3" ] ; then
    mkdir -p "$3"
fi

if [ -n "$4" ] ; then
    SEP=1 # useful for collecting files (see build-xml.py)
else
    SEP=0
fi

rm -rf "$ADDON_PATH"
mkdir  "$ADDON_PATH"

cd "$SVN_PATH"

date=`date -I`
uname=`uname`
mkdir "$ADDON_PATH/logs"
touch "$ADDON_PATH/logs/ALL.log"
echo "<!--<?xml-stylesheet href=\"style.css\" type=\"text/css\"?>-->
<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\"
	  \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">

<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" >

<head>
<meta http-equiv=\"Content-Type\" content=\"application/xhtml+xml; charset=utf-8\" />
<title>GRASS $ADDON_PATH AddOns Logs</title>
<style type=\"text/css\">
h1 { font-size: 125%; font-weight: bold; }
table
{
border-collapse:collapse;
}
table,th, td
{
border: 1px solid black;
}
</style>
</head>
<body>
<h1>GRASS $GRASS_VERSION AddOns / $uname (logs gererated $date)</h1>
<hr /> 
<table cellpadding=\"5\">
<tr><th style=\"background-color: grey\">AddOns</th>
<th style=\"background-color: grey\">Status</th>
<th style=\"background-color: grey\">Log file</th></tr>" > "$ADDON_PATH/logs/$INDEX_FILE"

echo "-----------------------------------------------------"
echo "AddOns '$ADDON_PATH'..."
echo "-----------------------------------------------------"
for c in "display" "general" "imagery" "raster" "raster3d" "vector"; do
    if [ ! -d $c ]; then
	continue
    fi
    cd $c
    for m in `ls -d */Makefile 2>/dev/null` ; do
	m="${m%%/Makefile}"
	echo -n "Compiling $m..."
	cd "$m"
	if [ $SEP -eq 1 ] ; then
	    path="$ADDON_PATH/$m"
	else
	    path="$ADDON_PATH"
	fi

	echo "<tr><td><tt>$c/$m</tt></td>" >> "$ADDON_PATH/logs/$INDEX_FILE"	
	make MODULE_TOPDIR="$TOPDIR" clean > /dev/null 2>&1
	make MODULE_TOPDIR="$TOPDIR" \
	    BIN="$path/bin" \
	    HTMLDIR="$path/docs/html" \
	    MANDIR="$path/man/man1" \
	    SCRIPTDIR="$path/scripts" \
	    ETC="$path/etc" > "$ADDON_PATH/logs/$m.log" 2>&1
	if [ `echo $?` -eq 0 ] ; then
	    printf "%-30s%s\n" "$c/$m" "SUCCESS" >> "$ADDON_PATH/logs/ALL.log"
	    echo " SUCCESS"
	    echo "<td style=\"background-color: green\">SUCCESS</td>" >> "$ADDON_PATH/logs/$INDEX_FILE"
	else
	    printf "%-30s%s\n" "$c/$m" "FAILED" >> "$ADDON_PATH/logs/ALL.log"
	    echo " FAILED"
	    echo "<td style=\"background-color: red\">FAILED</td>" >> "$ADDON_PATH/logs/$INDEX_FILE"
	fi
	echo "<td><a href=\"$m.log\">log</a></td></tr>" >> "$ADDON_PATH/logs/$INDEX_FILE"
	cd ..
    done
    cd ..
done

echo "</table><hr />
<div style=\"text-align: right\">Valid: <a href=\"http://validator.w3.org/check/referer\">XHTML</a></div>
</body></html>" >> "$ADDON_PATH/logs/$INDEX_FILE"

exit 0
