#!/usr/bin/python

import os
import sys
import re

def get_addons(path):
    return os.walk(path).next()[1]

def main(htmlfile, prefix):
    try:
        f = open(htmlfile)
        shtml = f.read()
    except IOError as e:
        sys.exit("Unable to read manual page: %s" % e)
    else:
        f.close()

    pos = []

    # find URIs
    pattern = r'''<a href="([^"]+)">([^>]+)</a>'''
    addons = get_addons(os.sep.join(htmlfile.split(os.sep)[:4]))
    for match in re.finditer(pattern, shtml):
        if match.group(1)[:7] == 'http://':
            continue
        if match.group(1).replace('.html', '') in addons:
            continue
        pos.append(match.start(1))

    if not pos:
        return # no match

    # replace file URIs
    ohtml = shtml[:pos[0]]
    for i in range(1, len(pos)):
        ohtml += prefix + '/' + shtml[pos[i-1]:pos[i]]
    ohtml += prefix + '/' + shtml[pos[-1]:]

    # write updated html file
    try:
        f = open(htmlfile, 'w')
        f.write(ohtml)
    except IOError as e:
        sys.exit("Unable for write manual page: %s" % e)
    else:
        f.close()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("provide file and url")
    main(sys.argv[1], sys.argv[2])
