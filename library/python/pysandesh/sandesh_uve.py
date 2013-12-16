#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# Sandesh UVE
#

import importlib
from sandesh_logger import SandeshLogger

logger = SandeshLogger('SandeshUVE').logger()

class SandeshUVETypeMaps(object):

    def __init__(self):
        self._uve_global_map = {}
    #end __init__

    def get_uve_global_map(self):
        return self._uve_global_map
    #end get_uve_global_map

    def register_uve_type_map(self, uve_type_key, uve_type_map):
        try:
            uve_map = self._uve_global_map[uve_type_key]
        except KeyError:
            self._uve_global_map[uve_type_key] = uve_type_map
        else:
            logger.error('UVE type "%s" already added' % (uve_type_key))
            assert 0
    #end register_uve_type_map

    def get_uve_type_map(self, uve_type_key):
        try:
            uve_map = self._uve_global_map[uve_type_key]
        except KeyError:
            logger.error('UVE type "%s" not present in the UVE global map' % (uve_type_key))
            return None
        else:
            return uve_map
    #end get_uve_type_map

    def update_uve_type_map(self, uve_type_key, uve_type_map):
        try:
            uve_map = self._uve_global_map[uve_type_key]
        except KeyError:
            logger.error('UVE type "%s" not present in the UVE global map' % (uve_type_key))
            assert 0
        else:
            self._uve_global_map[uve_type_key] = uve_type_map
    #end update_uve_type_map

    def sync_all_uve_types(self, inmap, sandesh_instance):
        for uve_type, uve_type_map in self._uve_global_map.iteritems():
            try:
                in_seqno = inmap[uve_type]
            except KeyError:
                uve_type_map.sync_uve(0, '', False, sandesh_instance)
            else:
                uve_type_map.sync_uve(in_seqno, '', False, sandesh_instance)
    #end sync_all_uve_types

#end class SandeshUVETypeMaps

class SandeshUVEPerTypeMap(object):

    class UVEMapEntry(object):
        def __init__(self, data, seqno):
            self.data = data
            self.seqno = seqno
            self.update_count = 0
        #end __init__

    #end class UVEMapEntry

    def __init__(self, sandesh, uve_type_name, uve_module):
        self._sandesh = sandesh
        self._uve_type = uve_type_name
        self._uve_module = uve_module
        self._uve_map = {}
        sandesh._uve_type_maps.register_uve_type_map(uve_type_name, self)
    #end __init__

    def uve_type_seqnum(self):
        seqnum = 0
        try:
            imp_module = importlib.import_module(self._uve_module)
        except ImportError:
            logger.error('Failed to import Module "%s"' % (self._uve_module))
        else:
            seqnum = getattr(imp_module, self._uve_type).lseqnum()
        return seqnum
    #end uve_type_seqnum

    def update_uve(self, uve_sandesh):
        uve_name = uve_sandesh.data.name
        try:
            uve_entry = self._uve_map[uve_name]
        except KeyError:
            logger.debug('Add uve <%s, %s> in the [%s] map' \
                % (uve_name, uve_sandesh.seqnum(), self._uve_type))
            self._uve_map[uve_name] = \
                SandeshUVEPerTypeMap.UVEMapEntry(uve_sandesh.data, uve_sandesh.seqnum())
        else:
            if uve_entry.data.deleted is True:
                if uve_sandesh.data.deleted is not True:
                    # The uve entry in the cache has been marked for deletion and
                    # a new uve entry with the same key has been created. Replace the
                    # deleted uve entry in the cache with this new entry.
                    logger.debug('Re-add uve <%s, %s> in the [%s] map' \
                        % (uve_name, uve_sandesh.seqnum(), self._uve_type))
                    self._uve_map[uve_name] = \
                        SandeshUVEPerTypeMap.UVEMapEntry(uve_sandesh.data, uve_sandesh.seqnum())
                else:
                    # Duplicate uve delete. Do we need to update the seqnum here?
                    logger.error('Duplicate uve delete <%s>' % (uve_name))
            else:
                uve_entry.data = uve_sandesh.update_uve(uve_entry.data)
                uve_entry.seqno = uve_sandesh.seqnum()
                uve_entry.update_count = uve_entry.update_count + 1
                self._uve_map[uve_name] = uve_entry
        # Now, update the uve_global_map
        self._sandesh._uve_type_maps.update_uve_type_map(self._uve_type, self)
    #end update_uve

    def sync_uve(self, seqno, ctx, more, sandesh_instance):
        count = 0
        try:
            imp_module = importlib.import_module(self._uve_module)
        except ImportError:
            logger.error('Failed to import Module "%s"' % (self._uve_module))
        else:
            for uve_name, uve_entry in self._uve_map.iteritems():
                if seqno == 0 or seqno < uve_entry.seqno:
                    try:
                        sandesh_uve = getattr(imp_module, self._uve_type)()
                    except AttributeError:
                        logger.error('Failed to create sandesh UVE "%s"' % (self._uve_type))
                        break
                    else:
                        sandesh_uve.data = uve_entry.data
                        logger.debug('send sync_uve <%s: %s> in the [%s] map)' \
                            % (uve_entry.data.name, uve_entry.seqno, self._uve_type))
                        sandesh_uve.send(True, uve_entry.seqno, ctx, more, sandesh_instance)
                        count += 1
        return count
    #end sync_uve

#end class SandeshUVEPerTypeMap
