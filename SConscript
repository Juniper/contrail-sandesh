#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

# -*- mode: python; -*-

Import('BuildEnv');

SandeshEnv = BuildEnv.Clone()

SandeshEnv.Append(CPPDEFINES='SANDESH')
SandeshEnv.Replace(LIBS='')

if BuildEnv['OPT'] != 'coverage':
    SandeshEnv.SConscript('compiler/SConscript', exports='SandeshEnv', duplicate = 0)
SandeshEnv.SConscript('library/SConscript', exports='SandeshEnv', duplicate = 0)
SandeshEnv.SConscript('common/SConscript', exports='SandeshEnv', duplicate = 0)
