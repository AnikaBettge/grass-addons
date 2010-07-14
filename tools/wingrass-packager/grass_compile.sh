#!/bin/sh
# Compile GRASS 6.4, 6.5 and 7.0 (update source code from SVN repository)

SRC=/osgeo4w/usr/src

function compile {
    cd $SRC/$1
    rm -f mswindows/osgeo4w/configure-stamp
    svn up
    ./mswindows/osgeo4w/package.sh
}

compile grass64_release
compile grass6_devel
compile grass_trunk

exit 0
