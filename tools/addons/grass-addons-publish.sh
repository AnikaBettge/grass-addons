#!/bin/sh

# updated for new CMS path MN 8/2015

URL=http://geo102.fsv.cvut.cz/grass/addons/
ADDONS=${HOME}/src/grass-addons

process () {
    major=$1
    minor=$2

    echo "Updating manuals for GRASS ${major}.${minor}..."
    wget -q $URL/grass${major}/modules.xml -O /var/www/grass/grass-cms/addons/grass${major}/modules.xml
    wget -q $URL/grass${major}/logs.tar.gz -O logs.tar.gz
    wget -q $URL/grass${major}/html.tar.gz -O html.tar.gz

    tar xzf logs.tar.gz
    rm -rf /var/www/grass/grass-cms/addons/grass${major}/logs
    mv logs /var/www/grass/grass-cms/addons/grass${major}/

    tar xzf html.tar.gz
    rm -rf /var/www/grass/grass-cms/grass${major}${minor}/manuals/addons
    mv addons /var/www/grass/grass-cms/grass${major}${minor}/manuals/
    rm logs.tar.gz
    rm html.tar.gz
}

cd $ADDONS
nup=`(svn up || (svn cleanup && svn up)) | wc -l`
if [ "$nup" -gt 1 ] || [ "$1" = "f" ] ; then
    process 7 0
    process 6 4 

    ${ADDONS}/tools/addons/grass-addons-index.sh
fi

exit 0
