#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# Sandesh Session
#

import socket
from transport import TTransport
from protocol import TXMLProtocol
from work_queue import WorkQueue
from tcp_session import TcpSession
from sandesh_logger import SandeshLogger

_XML_SANDESH_OPEN = '<sandesh length="0000000000">'
_XML_SANDESH_OPEN_ATTR_LEN = '<sandesh length="'
_XML_SANDESH_OPEN_END = '">'
_XML_SANDESH_CLOSE = '</sandesh>'


class SandeshReader(object):

    _READ_OK = 0
    _READ_ERR = -1

    def __init__(self, session, sandesh_msg_handler):
        self._session = session
        self._sandesh_instance = session.sandesh_instance()
        self._sandesh_msg_handler = sandesh_msg_handler
        self._read_buf = ''
        self._sandesh_len = 0
        self._logger = session._logger
    # end __init__

    # Public functions

    def read_msg(self, rcv_buf):
        self._read_buf += rcv_buf
        while True:
            (ret, sandesh) = self._extract_sandesh()
            if ret < 0:
                self._logger.error('Failed to extract sandesh')
                return self._READ_ERR
            if not sandesh:
                # read more data
                self._logger.debug(
                    'Not enough data to extract sandesh. Read more data.')
                break
            # Call sandesh message handler
            self._sandesh_msg_handler(self._session, sandesh)
            self._read_buf = self._read_buf[self._sandesh_len:]
            self._sandesh_len = 0
            if not len(self._read_buf):
                break

        return self._READ_OK
    # end read_msg

    @staticmethod
    def extract_sandesh_header(sandesh_xml):
        transport = TTransport.TMemoryBuffer(sandesh_xml)
        protocol_factory = TXMLProtocol.TXMLProtocolFactory()
        protocol = protocol_factory.getProtocol(transport)

        from gen_py.sandesh.ttypes import SandeshHeader
        hdr = SandeshHeader()
        hdr_len = hdr.read(protocol)
        if hdr_len == -1:
            return (None, 0, None)
        # Extract the sandesh name
        (length, sandesh_name) = protocol.readSandeshBegin()
        if length == -1:
            return (hdr, hdr_len, None)
        return (hdr, hdr_len, sandesh_name)
    # end extract_sandesh_header

    # Private functions

    def _extract_sandesh(self):
        if not self._sandesh_len:
            (ret, length) = self._extract_sandesh_len()
            if ret < 0:
                return (self._READ_ERR, None)
            elif not length:
                return (self._READ_OK, None)
            self._sandesh_len = length
        if len(self._read_buf) < self._sandesh_len:
            return (self._READ_OK, None)
        # Sanity check
        sandesh_close_tag = self._read_buf[
            self._sandesh_len - len(_XML_SANDESH_CLOSE):self._sandesh_len]
        if sandesh_close_tag != _XML_SANDESH_CLOSE:
            return (self._READ_ERR, None)

        # Extract sandesh
        sandesh_begin = len(_XML_SANDESH_OPEN)
        sandesh_end = self._sandesh_len - len(_XML_SANDESH_CLOSE)
        sandesh = self._read_buf[sandesh_begin:sandesh_end]
        return (self._READ_OK, sandesh)
    # end _extract_sandesh

    def _extract_sandesh_len(self):
        # Do we have enough data to extract the sandesh length?
        if len(self._read_buf) < len(_XML_SANDESH_OPEN):
            self._logger.debug('Not enough data to extract sandesh length')
            return (self._READ_OK, 0)
        # Sanity checks
        if self._read_buf[:len(_XML_SANDESH_OPEN_ATTR_LEN)] != \
                _XML_SANDESH_OPEN_ATTR_LEN:
            return (self._READ_ERR, 0)
        if self._read_buf[len(_XML_SANDESH_OPEN) - len(_XML_SANDESH_OPEN_END):
                          len(_XML_SANDESH_OPEN)] != _XML_SANDESH_OPEN_END:
            return (self._READ_ERR, 0)
        len_str = self._read_buf[len(_XML_SANDESH_OPEN_ATTR_LEN):
                                 len(_XML_SANDESH_OPEN) -
                                 len(_XML_SANDESH_OPEN_END)]
        try:
            length = int(len_str)
        except ValueError:
            self._logger.error(
                'Invalid sandesh length [%s] in the received message' %
                (len_str))
            return (self._READ_ERR, 0)

        self._logger.debug('Extracted sandesh length: %s' % (len_str))
        return (self._READ_OK, length)
    # end _extract_sandesh_len

# end class SandeshReader


class SandeshWriter(object):

    _MAX_SEND_BUF_SIZE = 4096

    def __init__(self, session):
        self._session = session
        self._sandesh_instance = session.sandesh_instance()
        self._send_buf_cache = ''
        self._logger = session._logger
    # end __init__

    # Public functions

    @staticmethod
    def encode_sandesh(sandesh, sandesh_instance=None):
        transport = TTransport.TMemoryBuffer()
        protocol_factory = TXMLProtocol.TXMLProtocolFactory()
        protocol = protocol_factory.getProtocol(transport)

        from gen_py.sandesh.ttypes import SandeshHeader

        sandesh_hdr = SandeshHeader(sandesh.scope(),
                                    sandesh.timestamp(),
                                    sandesh.module(),
                                    sandesh.source_id(),
                                    sandesh.context(),
                                    sandesh.seqnum(),
                                    sandesh.versionsig(),
                                    sandesh.type(),
                                    sandesh.hints(),
                                    sandesh.level(),
                                    sandesh.category(),
                                    sandesh.node_type(),
                                    sandesh.instance_id())
        # write the sandesh header
        if sandesh_hdr.write(protocol) < 0:
            if sandesh_instance is not None:
                sandesh_instance.msg_stats().update_tx_drop_stats(
                    sandesh.__class__.__name__, 0)
            return None
        # write the sandesh
        if sandesh.write(protocol) < 0:
            if sandesh_instance is not None:
                sandesh_instance.msg_stats().update_tx_drop_stats(
                    sandesh.__class__.__name__, 0)
            return None
        # get the message
        msg = transport.getvalue()
        # calculate the message length
        msg_len = len(_XML_SANDESH_OPEN) + len(msg) + len(_XML_SANDESH_CLOSE)
        len_width = len(_XML_SANDESH_OPEN) - \
            (len(_XML_SANDESH_OPEN_ATTR_LEN) + len(_XML_SANDESH_OPEN_END))
        # pad the length with leading 0s
        len_str = (str(msg_len)).zfill(len_width)
        encoded_buf = _XML_SANDESH_OPEN_ATTR_LEN + len_str + \
            _XML_SANDESH_OPEN_END + msg + _XML_SANDESH_CLOSE
        return encoded_buf
    # end encode_sandesh

    def send_msg(self, sandesh, more):
        send_buf = self.encode_sandesh(sandesh)
        if send_buf is None:
            self._logger.error('Failed to send sandesh')
            return -1
        # update sandesh tx stats
        self._sandesh_instance.msg_stats().update_tx_stats(
            sandesh.__class__.__name__, len(send_buf))
        if more:
            self._send_msg_more(send_buf)
        else:
            self._send_msg_all(send_buf)
        return 0
    # end send_msg

    # Private functions

    def _send_msg_more(self, send_buf):
        self._send_buf_cache += send_buf
        if len(self._send_buf_cache) >= self._MAX_SEND_BUF_SIZE:
            # send the message
            self._send(self._send_buf_cache)
            # reset the cache
            self._send_buf_cache = ''
    # end _send_msg_more

    def _send_msg_all(self, send_buf):
        # send the message
        self._send(self._send_buf_cache + send_buf)
        # reset the cache
        self._send_buf_cache = ''
    # end _send_msg_all

    def _send(self, send_buf):
        if self._session.write(send_buf) < 0:
            self._logger.error('Error sending message')
    # end _send

# end class SandeshWriter


class SandeshSession(TcpSession):
    _KEEPALIVE_IDLE_TIME = 15  # in secs
    _KEEPALIVE_INTERVAL = 3  # in secs
    _KEEPALIVE_PROBES = 5
    _TCP_USER_TIMEOUT_OPT = 18
    _TCP_USER_TIMEOUT_VAL = 30000 # ms

    def __init__(self, sandesh_instance, server, event_handler,
                 sandesh_msg_handler):
        self._sandesh_instance = sandesh_instance
        self._logger = sandesh_instance._logger
        self._event_handler = event_handler
        self._reader = SandeshReader(self, sandesh_msg_handler)
        self._writer = SandeshWriter(self)
        self._send_queue = WorkQueue(self._send_sandesh,
                                     self._is_ready_to_send_sandesh)
        TcpSession.__init__(self, server)
    # end __init__

    # Public functions

    def sandesh_instance(self):
        return self._sandesh_instance
    # end sandesh_instance

    def is_send_queue_empty(self):
        return self._send_queue.is_queue_empty()
    # end is_send_queue_empty

    def is_connected(self):
        return self._connected
    # end is_connected

    def enqueue_sandesh(self, sandesh):
        self._send_queue.enqueue(sandesh)
    # end enqueue_sandesh

    def send_queue(self):
        return self._send_queue
    # end send_queue

    # Overloaded functions from TcpSession

    def connect(self):
        TcpSession.connect(self, timeout=5)
    # end connect

    def _on_read(self, buf):
        if self._reader.read_msg(buf) < 0:
            self._logger.error('SandeshReader Error. Close Collector session')
            self.close()
    # end _on_read

    def _handle_event(self, event):
        self._event_handler(self, event)
    # end _handle_event

    def _set_socket_options(self):
        self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
        if hasattr(socket, 'TCP_KEEPIDLE'):
            self._socket.setsockopt(
                socket.IPPROTO_TCP, socket.TCP_KEEPIDLE,
                self._KEEPALIVE_IDLE_TIME)
        if hasattr(socket, 'TCP_KEEPALIVE'):
            self._socket.setsockopt(
                socket.IPPROTO_TCP, socket.TCP_KEEPALIVE,
                self._KEEPALIVE_IDLE_TIME)
        if hasattr(socket, 'TCP_KEEPINTVL'):
            self._socket.setsockopt(
                socket.IPPROTO_TCP, socket.TCP_KEEPINTVL,
                self._KEEPALIVE_INTERVAL)
        if hasattr(socket, 'TCP_KEEPCNT'):
            self._socket.setsockopt(
                socket.IPPROTO_TCP, socket.TCP_KEEPCNT, self._KEEPALIVE_PROBES)
        try:
            self._socket.setsockopt(socket.IPPROTO_TCP,
                self._TCP_USER_TIMEOUT_OPT, self._TCP_USER_TIMEOUT_VAL)
        except:
            self._logger.error('setsockopt failed: option %d, value %d' %
                (self._TCP_USER_TIMEOUT_OPT, self._TCP_USER_TIMEOUT_VAL))
    # end _set_socket_options

    # Private functions

    def _send_sandesh(self, sandesh):
        if self._send_queue.is_queue_empty():
            more = False
        else:
            more = True
        if not self._connected:
            if self._sandesh_instance.is_logging_dropped_allowed(sandesh):
                self._logger.error(
                    "SANDESH: %s: %s" % ("Not connected", sandesh.log()))
            self._sandesh_instance.msg_stats().update_tx_drop_stats(
                sandesh.__class__.__name__, 0)
            return
        if sandesh.is_logging_allowed(self._sandesh_instance):
            self._logger.log(
                SandeshLogger.get_py_logger_level(sandesh.level()),
                sandesh.log())
        self._writer.send_msg(sandesh, more)
    # end _send_sandesh

    def _is_ready_to_send_sandesh(self):
        return self._sandesh_instance.is_send_queue_enabled()
    # end _is_ready_to_send_sandesh

# end class SandeshSession
