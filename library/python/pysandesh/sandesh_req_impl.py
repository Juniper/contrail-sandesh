#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# Sandesh Request Implementation
#

from pysandesh.sandesh_uve import SandeshUVETypeMaps
from pysandesh.sandesh_trace import SandeshTraceRequestRunner
from pysandesh.gen_py.sandesh_uve.ttypes import SandeshUVECacheReq, \
    SandeshUVECacheResp, SandeshUVETypesReq, SandeshUVETypesResp, \
    SandeshUVETypeInfo, CollectorInfoRequest, CollectorInfoResponse, \
    SandeshLoggingParamsSet, SandeshLoggingParamsStatus, SandeshLoggingParams
from pysandesh.gen_py.sandesh_uve.ttypes import SandeshMessageStats, \
    SandeshMessageTypeStats, SandeshGeneratorStats, SandeshMessageStatsReq, \
    SandeshMessageStatsResp, SandeshSendQueueSet, SandeshSendQueueStatus, \
    SandeshSendQueueResponse
from pysandesh.gen_py.sandesh.ttypes import SandeshLevel
from pysandesh.gen_py.sandesh_trace.ttypes import SandeshTraceBufInfo, \
    SandeshTraceRequest, SandeshTraceBufferListRequest, \
    SandeshTraceBufferListResponse, SandeshTraceTextResponse, \
    SandeshTraceEnableDisableReq, SandeshTraceEnableDisableRes, \
    SandeshTraceBufStatusReq, SandeshTraceBufStatusRes, \
    SandeshTraceBufferEnableDisableReq, SandeshTraceBufferEnableDisableRes, \
    SandeshTraceBufStatusInfo
from pysandesh.gen_py.sandesh_uve.ttypes import SandeshQueueStats

class SandeshReqImpl(object):

    def __init__(self, sandesh):
        self._sandesh = sandesh
        SandeshUVECacheReq.handle_request = \
            self.sandesh_uve_cache_req_handle_request
        SandeshUVETypesReq.handle_request = \
            self.sandesh_uve_types_req_handle_request
        CollectorInfoRequest.handle_request = \
            self.collector_info_request_handle_request
        SandeshLoggingParamsSet.handle_request = \
            self.sandesh_logging_params_set_handle_request
        SandeshLoggingParamsStatus.handle_request = \
            self.sandesh_logging_params_status_handle_request
        SandeshMessageStatsReq.handle_request = \
            self.sandesh_stats_handle_request
        SandeshTraceBufferListRequest.handle_request = \
            self.sandesh_trace_buffer_list_request_handle_request
        SandeshTraceRequest.handle_request = \
            self.sandesh_trace_request_handle_request
        SandeshTraceEnableDisableReq.handle_request = \
            self.sandesh_trace_enable_disable_handle_request
        SandeshTraceBufStatusReq.handle_request = \
            self.sandesh_trace_buf_status_handle_request
        SandeshTraceBufferEnableDisableReq.handle_request = \
            self.sandesh_trace_buffer_enable_disable_handle_request
        SandeshSendQueueSet.handle_request = \
            self.sandesh_send_queue_set_handle_request
        SandeshSendQueueStatus.handle_request = \
            self.sandesh_send_queue_status_handle_request
    # end __init__

    # Public functions
    def collector_info_request_handle_request(self, sandesh_req):
        resp = CollectorInfoResponse()
        client = self._sandesh.client()
        if client is not None:
            if client.connection() is not None:
                collector = client.connection().server()
                if collector is not None:
                    collector = collector.split(':')
                    resp.ip = collector[0]
                    resp.port = int(collector[1])
                resp.status = client.connection().state()
        resp.response(sandesh_req.context())
    # end collector_info_handle_request

    def sandesh_logging_params_set_handle_request(self, sandesh_req):
        # Set the logging params
        if sandesh_req.enable is not None:
            if sandesh_req.enable:
                benable = True
            else:
                benable = False
            self._sandesh.set_local_logging(benable)
        if sandesh_req.category is not None:
            self._sandesh.set_logging_category(sandesh_req.category)
        if sandesh_req.log_level is not None:
            self._sandesh.set_logging_level(sandesh_req.log_level)
        # Return the logging params
        sandesh_logging_resp = SandeshLoggingParams(
            enable=self._sandesh.is_local_logging_enabled(),
            category=self._sandesh.logging_category(
            ),
            log_level=SandeshLevel._VALUES_TO_NAMES[self._sandesh.logging_level()])
        sandesh_logging_resp.response(sandesh_req.context())
    # end sandesh_logging_params_set_handle_request

    def sandesh_logging_params_status_handle_request(self, sandesh_req):
        # Return the logging params
        sandesh_logging_resp = SandeshLoggingParams(
            enable=self._sandesh.is_local_logging_enabled(),
            category=self._sandesh.logging_category(
            ),
            log_level=SandeshLevel._VALUES_TO_NAMES[self._sandesh.logging_level()])
        sandesh_logging_resp.response(sandesh_req.context())
    # end sandesh_logging_params_status_handle_request

    def sandesh_uve_cache_req_handle_request(self, sandesh_req):
        count = 0
        uve_type_name = self._sandesh._uve_type_maps.get_uve_type_name(
            sandesh_req.tname)
        uve_type_map = self._sandesh._uve_type_maps.get_uve_type_map(
            uve_type_name)
        if uve_type_map:
            count = uve_type_map.sync_uve(
                0, sandesh_req.context(), True, self._sandesh)
        uve_cache_res = SandeshUVECacheResp(count)
        uve_cache_res.response(sandesh_req.context())
    # end sandesh_uve_cache_req_handle_request

    def sandesh_uve_types_req_handle_request(self, sandesh_req):
        uve_global_map = self._sandesh._uve_type_maps.get_uve_global_map()
        uve_type_info_list = []
        for uve_type_key, uve_type_map in uve_global_map.iteritems():
            uve_type_info = SandeshUVETypeInfo(
                uve_type_map.uve_data_type(), uve_type_map.uve_type_seqnum())
            uve_type_info_list.append(uve_type_info)
        uve_types_res = SandeshUVETypesResp(uve_type_info_list)
        uve_types_res.response(sandesh_req.context())
    # end sandesh_uve_types_req_handle_request

    def sandesh_stats_handle_request(self, sandesh_req):
        sandesh_stats = self._sandesh.stats()
        stats_map = sandesh_stats.stats_map()
        stats_list = []
        for sandesh_name, stats in stats_map.iteritems():
            msg_stat = SandeshMessageStats(stats.tx_count,
                                           stats.tx_bytes,
                                           stats.rx_count,
                                           stats.rx_bytes)
            mtype_stat = SandeshMessageTypeStats(sandesh_name,
                                                 msg_stat)
            stats_list.append(mtype_stat)

        gen_stats = SandeshGeneratorStats()
        gen_stats.type_stats = stats_list
        gen_stats.aggregate_stats = SandeshMessageStats()
        gen_stats.aggregate_stats.messages_sent = sandesh_stats._sandesh_sent
        gen_stats.aggregate_stats.bytes_sent = sandesh_stats._bytes_sent
        gen_stats.aggregate_stats.messages_received = \
            sandesh_stats._sandesh_received
        gen_stats.aggregate_stats.bytes_received = \
            sandesh_stats._bytes_received
        connection = self._sandesh.client().connection()
        if connection and connection.session():
            session = connection.session()
            squeue = session.send_queue()
            send_queue_stats = SandeshQueueStats()
            send_queue_stats.enqueues = \
                squeue.num_enqueues()
            send_queue_stats.count = \
                squeue.num_enqueues() - squeue.num_dequeues()
            gen_stats.send_queue_stats = send_queue_stats
        stats_resp = SandeshMessageStatsResp(gen_stats)
        stats_resp.response(sandesh_req.context())
    # end sandesh_stats_handle_request

    def sandesh_trace_buffer_list_request_handle_request(self, sandesh_req):
        tbuf_info_list = [SandeshTraceBufInfo(trace_buf_name=tbuf)
                          for tbuf in self._sandesh.trace_buffer_list_get()]
        tbuf_info_resp = SandeshTraceBufferListResponse(tbuf_info_list)
        tbuf_info_resp.response(sandesh_req.context())
    # end sandesh_trace_buffer_list_request_handle_request

    def sandesh_trace_enable_disable_handle_request(self, sandesh_req):
        if sandesh_req.enable is not None:
            if sandesh_req.enable:
                self._sandesh.trace_enable()
            else:
                self._sandesh.trace_disable()
        else:
            if self._sandesh.is_trace_enabled():
                self._sandesh.trace_disable()
            else:
                self._sandesh.trace_enable()
        if self._sandesh.is_trace_enabled():
            status = 'Sandesh Trace Enabled'
        else:
            status = 'Sandesh Trace Disabled'
        trace_en_dis_resp = SandeshTraceEnableDisableRes(status)
        trace_en_dis_resp.response(sandesh_req.context())
    # end sandesh_trace_enable_disable_handle_request

    def sandesh_trace_buf_status_handle_request(self, sandesh_req):
        trace_buf_status_list = []
        for tbuf in self._sandesh.trace_buffer_list_get():
            trace_buf_status = SandeshTraceBufStatusInfo()
            trace_buf_status.trace_buf_name = tbuf
            if self._sandesh.is_trace_buffer_enabled(tbuf):
                trace_buf_status.enable_disable = 'Enabled'
            else:
                trace_buf_status.enable_disable = 'Disabled'
            trace_buf_status_list.append(trace_buf_status)
        trace_buf_status_resp = SandeshTraceBufStatusRes(trace_buf_status_list)
        trace_buf_status_resp.response(sandesh_req.context())
    # end sandesh_trace_buf_status_handle_request

    def sandesh_trace_buffer_enable_disable_handle_request(self, sandesh_req):
        if sandesh_req.trace_buf_name in self._sandesh.trace_buffer_list_get():
            if sandesh_req.enable is not None:
                if sandesh_req.enable:
                    self._sandesh.trace_buffer_enable(
                        sandesh_req.trace_buf_name)
                else:
                    self._sandesh.trace_buffer_disable(
                        sandesh_req.trace_buf_name)
            else:
                if self._sandesh.is_trace_buffer_enabled(
                        sandesh_req.trace_buf_name):
                    self._sandesh.trace_buffer_disable(
                        sandesh_req.trace_buf_name)
                else:
                    self._sandesh.trace_buffer_enable(
                        sandesh_req.trace_buf_name)
            if self._sandesh.is_trace_buffer_enabled(
                    sandesh_req.trace_buf_name):
                status = 'Trace buffer Enabled'
            else:
                status = 'Trace buffer Disabled'
        else:
            status = 'Invalid Trace buffer'
        trace_buf_en_dis_resp = SandeshTraceBufferEnableDisableRes(status)
        trace_buf_en_dis_resp.response(sandesh_req.context())
    # end sandesh_trace_buffer_enable_disable_handle_request

    def sandesh_trace_request_handle_request(self, sandesh_req):
        if "http://" in sandesh_req.context():
            read_context = "Http"
        else:
            read_context = "Collector"
        trace_req_runner = SandeshTraceRequestRunner(sandesh=self._sandesh,
                                                     request_buffer_name=
                                                     sandesh_req.buf_name,
                                                     request_context=
                                                     sandesh_req.context(),
                                                     read_context=read_context,
                                                     request_count=
                                                     sandesh_req.count)
        trace_req_runner.Run()
    # end sandesh_trace_request_handle_request

    def sandesh_send_queue_set_handle_request(self, sandesh_req):
        # Set the send queue processing state
        if sandesh_req.enable is not None:
            if sandesh_req.enable:
                benable = True
            else:
                benable = False
            self._sandesh.set_send_queue(benable)
        # Return the send queue processing state
        send_queue_resp = SandeshSendQueueResponse(
            enable=self._sandesh.is_send_queue_enabled())
        send_queue_resp.response(sandesh_req.context())
    # end sandesh_send_queue_set_handle_request

    def sandesh_send_queue_status_handle_request(self, sandesh_req):
        # Return the send queue processing state
        send_queue_resp = SandeshSendQueueResponse(
            enable=self._sandesh.is_send_queue_enabled())
        send_queue_resp.response(sandesh_req.context())
    # end sandesh_send_queue_status_handle_request

# end class SandeshReqImpl
