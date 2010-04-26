#!/usr/bin/env python
# -*- Mode: Python; tab-width: 4 -*-
#
# LLVM Source Files sync from Makefile.am
#
# Copyright (C) 2010 Gianluigi Tiesi <sherpya@netfarm.it>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
# ======================================================================

# script deps: python-lxml

import lxml.etree
from lxml import objectify

llvm_base = '../../libclamav/c++/'
vcprefix = '../../' + llvm_base
vcproj = 'proj/vc8/libclamav_llvm.vcproj'
mingwmake = '../mingw/llvm.mak'
projects = [ 'libclamavcxx', 'libllvmsystem', 'libllvmcodegen', 'libllvmx86codegen', 'libllvmjit' ]

def skip_line(line):
    line = line.strip()
    if line.find('=') == -1: return False
    if line.startswith('#'): return False
    return True

def skip_lib(lib):
    if lib.find('_la_SOURCES') == -1: return True
    if lib[:-11] not in projects: return True
    return False

def parse_makefile_am(path):
    mf = open(path).read()
    mf = mf.replace('\\\n', '')
    lines = mf.split('\n')
    sources = set()
    lines = filter(skip_line, lines)
    for line in lines:
        line = line.strip().replace('+=', '=')
        key, value = line.split('=', 1)
        key, value = key.strip(), value.strip()
        if skip_lib(key): continue
        values = value.split()
        for source in values:
            if source.endswith('.h'): continue
            sources.add(source)
    sources.remove('llvm/config.status')
    return sorted(sources)

def relpath(path):
    rel = vcprefix + path
    return rel.replace('/', '\\')

def gen_vcproj(path, mksources):
    proj = objectify.parse(open(path))
    root = proj.getroot()
    source_files = root.xpath('Files/Filter[@Name="Source Files"]')[0]
    files = source_files.xpath('File')
    sources = []
    for f in files:
        s = f.attrib['RelativePath'].replace('\\', '/').replace(vcprefix, '')
        sources.append(s)
    sources.sort()
    print 'Files in vcproj: %d - files in Makefile.am: %d' % (len(sources), len(mksources))
    if sources == mksources:
        print 'VC Project unchanged'
    else:
        print 'Updating VC Project'
        source_files.clear()
        source_files.attrib['Name'] = "Source Files"
        for newfile in mksources:
            newfile = vcprefix + newfile
            newfile = newfile.replace('/', '\\')
            f = lxml.etree.fromstring('<File RelativePath="%s"></File>' % newfile)
            source_files.append(f)
        out = open(path, 'w')
        out.write('<?xml version="1.0" encoding="Windows-1252"?>\n')
        proj.write(out, pretty_print=True)

def gen_mingwmake(path, sources):
    print 'Writing mingw makefile'
    f = open(path, 'wb')
    f.write('libclamav_llvm_SOURCES=$(addprefix $(top)/libclamav/c++/,' + ' \\\n\t'.join(sources) + ')')
    f.write('\n')
    f.close()

if __name__ == '__main__':
    sources = parse_makefile_am(llvm_base + 'Makefile.am')
    gen_mingwmake(mingwmake, sources)
    gen_vcproj(vcproj, sources)
