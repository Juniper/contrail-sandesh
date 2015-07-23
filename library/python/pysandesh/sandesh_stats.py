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

    def update_tx_stats(self, message_type, nbytes,
                        drop_reason=Sandesh.DropReason.Tx.NoDrop):
        if Sandesh.DropReason.Tx.MinDropReason < drop_reason < \
           Sandesh.DropReason.Tx.MaxDropReason:
            try:
                message_stats = self._message_type_stats[message_type]
            except KeyError:
                message_stats = SandeshMessageStats()
                self._message_type_stats[message_type] = message_stats
            finally:
                self._update_tx_stats_internal(message_stats, nbytes,
                                               drop_reason)
                self._update_tx_stats_internal(self._aggregate_stats, nbytes,
                                               drop_reason)
                return True
        return False
    # end update_tx_stats

    def update_rx_stats(self, message_type, nbytes,
                        drop_reason=Sandesh.DropReason.Rx.NoDrop):
        if Sandesh.DropReason.Rx.MinDropReason < drop_reason < \
           Sandesh.DropReason.Rx.MaxDropReason:
            try:
                message_stats = self._message_type_stats[message_type]
            except KeyError:
                message_stats = SandeshMessageStats()
                self._message_type_stats[message_type] = message_stats
            finally:
                self._update_rx_stats_internal(message_stats, nbytes,
                                               drop_reason)
                self._update_rx_stats_internal(self._aggregate_stats, nbytes,
                                               drop_reason)
                return True
        return False
    # end update_rx_stats

    def _update_tx_stats_internal(self, msg_stats, nbytes, drop_reason):
        if drop_reason is Sandesh.DropReason.Tx.NoDrop:
            try:
                msg_stats.messages_sent += 1
                msg_stats.bytes_sent += nbytes
            except TypeError:
                msg_stats.messages_sent = 1
                msg_stats.bytes_sent = nbytes
        else:
            try:
                msg_stats.messages_sent_dropped += 1
                msg_stats.bytes_sent_dropped += nbytes
            except TypeError:
                msg_stats.messages_sent_dropped = 1
                msg_stats.bytes_sent_dropped = nbytes
            if drop_reason is Sandesh.DropReason.Tx.ValidationFailed:
                try:
                    msg_stats.messages_sent_dropped_validation_failed += 1
                    msg_stats.bytes_sent_dropped_validation_failed += nbytes
                except TypeError:
                    msg_stats.messages_sent_dropped_validation_failed = 1
                    msg_stats.bytes_sent_dropped_validation_failed = nbytes
            elif drop_reason is Sandesh.DropReason.Tx.QueueLevel:
                try:
                    msg_stats.messages_sent_dropped_queue_level += 1
                    msg_stats.bytes_sent_dropped_queue_level += nbytes
                except TypeError:
                    msg_stats.messages_sent_dropped_queue_level = 1
                    msg_stats.bytes_sent_dropped_queue_level = nbytes
            elif drop_reason is Sandesh.DropReason.Tx.NoClient:
                try:
                    msg_stats.messages_sent_dropped_no_client += 1
                    msg_stats.bytes_sent_dropped_no_client += nbytes
                except TypeError:
                    msg_stats.messages_sent_dropped_no_client = 1
                    msg_stats.bytes_sent_dropped_no_client = nbytes
            elif drop_reason is Sandesh.DropReason.Tx.NoSession:
                try:
                    msg_stats.messages_sent_dropped_no_session += 1
                    msg_stats.bytes_sent_dropped_no_session += nbytes
                except TypeError:
                    msg_stats.messages_sent_dropped_no_session = 1
                    msg_stats.bytes_sent_dropped_no_session = nbytes
            elif drop_reason is Sandesh.DropReason.Tx.NoQueue:
                try:
                    msg_stats.messages_sent_dropped_no_queue += 1
                    msg_stats.bytes_sent_dropped_no_queue += nbytes
                except TypeError:
                    msg_stats.messages_sent_dropped_no_queue = 1
                    msg_stats.bytes_sent_dropped_no_queue = nbytes
            elif drop_reason is Sandesh.DropReason.Tx.ClientSendFailed:
                try:
                    msg_stats.messages_sent_dropped_client_send_failed += 1
                    msg_stats.bytes_sent_dropped_client_send_failed += nbytes
                except TypeError:
                    msg_stats.messages_sent_dropped_client_send_failed = 1
                    msg_stats.bytes_sent_dropped_client_send_failed = nbytes
            elif drop_reason is Sandesh.DropReason.Tx.HeaderWriteFailed:
                try:
                    msg_stats.messages_sent_dropped_header_write_failed += 1
                    msg_stats.bytes_sent_dropped_header_write_failed += nbytes
                except TypeError:
                    msg_stats.messages_sent_dropped_header_write_failed = 1
                    msg_stats.bytes_sent_dropped_header_write_failed = nbytes
            elif drop_reason is Sandesh.DropReason.Tx.WriteFailed:
                try:
                    msg_stats.messages_sent_dropped_write_failed += 1
                    msg_stats.bytes_sent_dropped_write_failed += nbytes
                except TypeError:
                    msg_stats.messages_sent_dropped_write_failed = 1
                    msg_stats.bytes_sent_dropped_write_failed = nbytes
            elif drop_reason is Sandesh.DropReason.Tx.SessionNotConnected:
                try:
                    msg_stats.messages_sent_dropped_session_not_connected += 1
                    msg_stats.bytes_sent_dropped_session_not_connected += nbytes
                except TypeError:
                    msg_stats.messages_sent_dropped_session_not_connected = 1
                    msg_stats.bytes_sent_dropped_session_not_connected = nbytes
            elif drop_reason is Sandesh.DropReason.Tx.WrongClientSMState:
                try:
                    msg_stats.messages_sent_dropped_wrong_client_sm_state += 1
                    msg_stats.bytes_sent_dropped_wrong_client_sm_state += nbytes
                except TypeError:
                    msg_stats.messages_sent_dropped_wrong_client_sm_state = 1
                    msg_stats.bytes_sent_dropped_wrong_client_sm_state = nbytes
            else:
                assert 0, 'Unhandled Tx drop reason <%s>' % (str(drop_reason))
    # end _update_tx_stats_internal

    def _update_rx_stats_internal(self, msg_stats, nbytes, drop_reason):
        if drop_reason is Sandesh.DropReason.Rx.NoDrop:
            try:
                msg_stats.messages_received += 1
                msg_stats.bytes_received += nbytes
            except TypeError:
                msg_stats.messages_received = 1
                msg_stats.bytes_received = nbytes
        else:
            try:
                msg_stats.messages_received_dropped += 1
                msg_stats.bytes_received_dropped += nbytes
            except TypeError:
                msg_stats.messages_received_dropped = 1
                msg_stats.bytes_received_dropped = nbytes
            if drop_reason is Sandesh.DropReason.Rx.QueueLevel:
                try:
                    msg_stats.messages_received_dropped_queue_level += 1
                    msg_stats.bytes_received_dropped_queue_level += nbytes
                except TypeError:
                    msg_stats.messages_received_dropped_queue_level = 1
                    msg_stats.bytes_received_dropped_queue_level = nbytes
            elif drop_reason is Sandesh.DropReason.Rx.NoQueue:
                try:
                    msg_stats.messages_received_dropped_no_queue += 1
                    msg_stats.bytes_received_dropped_no_queue += nbytes
                except TypeError:
                    msg_stats.messages_received_dropped_no_queue = 1
                    msg_stats.bytes_received_dropped_no_queue = nbytes
            elif drop_reason is Sandesh.DropReason.Rx.ControlMsgFailed:
                try:
                    msg_stats.messages_received_dropped_control_msg_failed += 1
                    msg_stats.bytes_received_dropped_control_msg_failed += nbytes
                except TypeError:
                    msg_stats.messages_received_dropped_control_msg_failed = 1
                    msg_stats.bytes_received_dropped_control_msg_failed = nbytes
            elif drop_reason is Sandesh.DropReason.Rx.CreateFailed:
                try:
                    msg_stats.messages_received_dropped_create_failed += 1
                    msg_stats.bytes_received_dropped_create_failed += nbytes
                except TypeError:
                    msg_stats.messages_received_dropped_create_failed = 1
                    msg_stats.bytes_received_dropped_create_failed = nbytes
            elif drop_reason is Sandesh.DropReason.Rx.DecodingFailed:
                try:
                    msg_stats.messages_received_dropped_decoding_failed += 1
                    msg_stats.bytes_received_dropped_decoding_failed += nbytes
                except TypeError:
                    msg_stats.messages_received_dropped_decoding_failed = 1
                    msg_stats.bytes_received_dropped_decoding_failed = nbytes
            else:
                assert 0, 'Unhandled Rx drop reason <%s>' % (str(drop_reason))
    # end _update_rx_stats_internal

# end class SandeshMessageStatistics
