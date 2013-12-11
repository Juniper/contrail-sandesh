#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

import sys

SandeshEnv = DefaultEnvironment().Clone()

SandeshEnv.Append(CPPDEFINES='SANDESH')
SandeshEnv.Replace(LIBS='')

if sys.platform == 'darwin':
    SandeshEnv.Append(CPPPATH = '#/build/include/boost')

while SandeshEnv['CCFLAGS'].count('--coverage') > 0:
    SandeshEnv['CCFLAGS'].remove('--coverage')

subdirs = ['compiler', 'library']
for dir in subdirs:
    SConscript(dir + '/SConscript', exports = 'SandeshEnv',
               variant_dir = SandeshEnv['TOP'] + '/tools/sandesh/' + dir,
               duplicate = 0)

# Local Variables:
# mode: python
# End:
