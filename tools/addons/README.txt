These scripts are used for building addons modules (on build server,
currently at the CTU in Prague) and publishing their manual pages on
publishing server (grass.osgeo.org).

WORKFLOW

On building server are run two scripts (see crontab.build):

1) grass-addons.sh to recompile GRASS locally

2) grass-addons-build.sh to build addons modules on build server, the
script provides tarballs with created addons manual pages and logs for
publishing

Result is copied to publishing server by (see crontab.publish):

3) grass-addons-publish.sh downloads provided tarballs (2) and creates
index.html page on publishing server

SCRIPTS OVERVIEW

* build-xml.py - support Python script called by compile-xml.sh
* compile.sh - support script to compile GRASS Addons modules called by compile-xml.sh
* compile-xml.sh - creates XML file for each addons module (used by g.extension)
* grass-addons-index.sh - creates ovewview index page, called by grass-addons-publish.sh
* grass-addons-build.sh - called on Building server (2)
* grass-addons-publish.sh - called on Publishing server (1)
* grass-addons.sh - compiles GRASS and Addons, called by grass-addons-build.sh
* update_manual.py - support Python script which modifies addons
  manual pages for Publishing server (called by grass-addons-build.sh)

RESULTS

* manual pages at: http://grass.osgeo.org/grass70/manuals/addons/
* XML file at: https://grass.osgeo.org/addons/

SEE ALSO

https://trac.osgeo.org/grass/wiki/AddOnsManagement
