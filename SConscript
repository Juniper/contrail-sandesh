#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

import sys

SandeshEnv = DefaultEnvironment().Clone()

SandeshEnv.Append(CPPDEFINES='SANDESH')
SandeshEnv.Replace(LIBS='')

# TODO(md): Change it to SCons' Configure context (autoconf equivalent) for automatic
# discovery of headers on different platforms
if sys.platform.startswith('freebsd'):
    SandeshEnv.Append(CPPDEFINES='HAVE_SYS_STAT_H')

subdirs = ['compiler', 'library']
for dir in subdirs:
    SConscript(dir + '/SConscript', exports = 'SandeshEnv',
               variant_dir = SandeshEnv['TOP'] + '/tools/sandesh/' + dir,
               duplicate = 0)

# Local Variables:
# mode: python
# End:
