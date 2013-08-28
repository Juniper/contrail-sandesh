#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# sandesh_stats.py
#

class SandeshStats(object):
    class SandeshStatsElem(object):
        def __init__(self):
            self.tx_count = 0
            self.tx_bytes = 0
            self.rx_count = 0
            self.rx_bytes = 0
        #end __init__
    #end SandeshStatsElem

    def __init__(self):
        self._sandesh_sent = 0
        self._bytes_sent = 0
        self._sandesh_received = 0
        self._bytes_received = 0
        self._stats_map = {}
    #end __init__

    def stats_map(self):
        return self._stats_map
    #end stats_map

    def update_stats(self, sandesh_name, bytes, is_tx):
        try:
            stats_elem = self._stats_map[sandesh_name]
        except KeyError:
            stats_elem = SandeshStats.SandeshStatsElem()
        finally:
            if is_tx:
                stats_elem.tx_count += 1
                stats_elem.tx_bytes += bytes
                self._sandesh_sent += 1
                self._bytes_sent += bytes
            else:
                stats_elem.rx_count += 1
                stats_elem.rx_bytes += bytes
                self._sandesh_received += 1
                self._bytes_received += bytes
            self._stats_map[sandesh_name] = stats_elem
    #end update_stats

#end class SandeshStats
