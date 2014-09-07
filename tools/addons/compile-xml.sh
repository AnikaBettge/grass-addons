#!/bin/sh

if test -z "$1" ; then
    echo "DEST not defined"
    exit 1
fi
DEST="$1"

compile() {
    ./compile.sh $1 $2 $3 1
}

build_xml() {
    ./build-xml.py $1
    cp $1/modules.xml $DEST/grass$2/
    cp -r $1/logs $DEST/grass$2/
}

# update GRASS Addons SVN
(cd ..; svn up || (svn cleanup && svn up))

### export GISRC=$HOME/grassdata/demolocation/.grassrc70

# compile AddOns for GRASS 7 and GRASS 6.5
compile ../../grass7 ~/src/grass70_release/dist.x86_64-unknown-linux-gnu ~/.grass7/addons
compile ../../grass6 ~/src/grass64_release/dist.x86_64-unknown-linux-gnu ~/.grass6/addons

# create XML file for AddOns
build_xml ~/.grass7/addons 7
build_xml ~/.grass6/addons 6

exit 0
