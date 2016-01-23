#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# sandesh_stats.py
#

from pysandesh.sandesh_base import Sandesh
from pysandesh.gen_py.sandesh_uve.ttypes import SandeshMessageStats

class SandeshMessageStatistics(object):

    def __init__(self):
        self._message_type_stats = {}
        self._aggregate_stats = SandeshMessageStats()
    # end __init__

    def message_type_stats(self):
        return self._message_type_stats
    # end message_type_stats

    def aggregate_stats(self):
        return self._aggregate_stats
    # end aggregate_stats

    def update_tx_stats(self, message_type, nbytes):
        return self._update_tx_rx_stats_internal(message_type, nbytes, True,
            False)
    # end update_tx_stats

    def update_tx_drop_stats(self, message_type, nbytes):
        return self._update_tx_rx_stats_internal(message_type, nbytes, True,
            True)
    # end update_tx_drop_stats

    def update_rx_stats(self, message_type, nbytes):
        return self._update_tx_rx_stats_internal(message_type, nbytes, False,
            False)
    # end update_rx_stats

    def update_rx_drop_stats(self, message_type, nbytes):
        return self._update_tx_rx_stats_internal(message_type, nbytes, False,
            True)
    # end update_rx_drop_stats

    def _update_tx_rx_stats_internal(self, message_type, nbytes, tx, drop):
        try:
            message_stats = self._message_type_stats[message_type]
        except KeyError:
            message_stats = SandeshMessageStats()
            self._message_type_stats[message_type] = message_stats
        finally:
            if tx:
                self._update_tx_stats_internal(message_stats, nbytes, drop)
                self._update_tx_stats_internal(self._aggregate_stats, nbytes,
                    drop)
            else:
                self._update_rx_stats_internal(message_stats, nbytes, drop)
                self._update_rx_stats_internal(self._aggregate_stats, nbytes,
                    drop)
        return True
    # end update_tx_rx_stats_internal

    def _update_tx_stats_internal(self, msg_stats, nbytes, drop):
        if not drop:
            try:
                msg_stats.messages_sent += 1
                msg_stats.bytes_sent += nbytes
            except TypeError:
                msg_stats.messages_sent = 1
                msg_stats.bytes_sent = nbytes
        else:
            if msg_stats.messages_sent_dropped:
                msg_stats.messages_sent_dropped += 1
                msg_stats.bytes_sent_dropped += nbytes
            else:
                msg_stats.messages_sent_dropped = 1
                msg_stats.bytes_sent_dropped = nbytes
    # end _update_tx_stats_internal

    def _update_rx_stats_internal(self, msg_stats, nbytes, drop):
        if not drop:
            if msg_stats.messages_received:
                msg_stats.messages_received += 1
                msg_stats.bytes_received += nbytes
            else:
                msg_stats.messages_received = 1
                msg_stats.bytes_received = nbytes
        else:
            if msg_stats.messages_received_dropped:
                msg_stats.messages_received_dropped += 1
                msg_stats.bytes_received_dropped += nbytes
            else:
                msg_stats.messages_received_dropped = 1
                msg_stats.bytes_received_dropped = nbytes
    # end _update_rx_stats_internal

# end class SandeshMessageStatistics
