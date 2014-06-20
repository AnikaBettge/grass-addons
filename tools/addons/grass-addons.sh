#!/bin/sh

DIR=$HOME/src
# XMLDIR=/osgeo/grass/grass-cms/addons/
# MANDIR=/osgeo/grass/grass-cms/
XMLDIR=/var/www/grass/addons/
MANDIR=/var/www/grass

if [ ! -d "$XMLDIR" ]; then
    mkdir -p $XMLDIR
fi

build_addons() {
cd $DIR/grass-addons/ 

##revl=`svn info | grep 'Revision' | cut -d':' -f2 | tr -d ' '`
##revr=`svn info -rHEAD | grep 'Revision' | cut -d':' -f2 | tr -d ' '`
nup=`(svn up || (svn cleanup && svn up)) | wc -l`

###if [ "$revl" != "$revr" ] || [ "$1" = "f" ] ; then
if [ "$nup" -gt 1 ] || [ "$1" = "f" ] ; then
    ###svn up || (svn cleanup && svn up)

    cd tools/addons/ 
    ./compile-xml.sh $XMLDIR
    for version in 6 7 ; do
    	cd $HOME/.grass${version}/addons/
    	cp modules.xml $XMLDIR/grass${version}
    	rsync -ag --delete logs $XMLDIR/grass${version}
    	cd $XMLDIR/grass${version}/logs
    	ln -sf ALL.html index.html
    done

    update_manual 7 0
    update_manual 6 4
fi
}

recompile_grass() {
    cd $DIR

    for gdir in "grass70_release" "grass64_release" ; do
	cd $gdir
        echo "Recompiling $gdir..." 1>&2
	svn up
	make distclean >/dev/null 2>&1
	if [ $gdir = "grass64_release" ] ; then 
	    num=6
	else
	    num=7
	fi
	$DIR/configures.sh grass$num >/dev/null 2>&1
	make >/dev/null 2>&1
        cat error.log 1>&2
	cd ..
    done
}

update_manual() {
    major=$1
    minor=$2
    echo "Updating manuals for GRASS ${major}.${minor}..."
    dst="$MANDIR/grass${major}${minor}/manuals/addons/"
    if [ ! -d $dst ] ; then
	mkdir -p $dst
        cd $dst
	wget http://grass.osgeo.org/grass${major}${minor}/manuals/grass_logo.png 
	wget http://grass.osgeo.org/grass${major}${minor}/manuals/grassdocs.css
    fi
    cd $HOME/.grass${major}/addons/
    for m in $(ls -d */) ; do 
        if [ `ls ${m}docs/html/ -w1 2>/dev/null | wc -l` -gt 0 ] ; then
	    cp ${m}docs/html/* $dst
        fi
    done

    cd $DIR/grass-addons/tools/addons    
    ./build-index.sh $MANDIR ${major} ${minor}
}

export GRASS_SKIP_MAPSET_OWNER_CHECK="1"

if [ "$1" = "c" ] || [ "$2" = "c" ] ; then
    recompile_grass
fi

build_addons $1

exit 0
