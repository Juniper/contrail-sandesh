#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# Sandesh
#

import os
import gevent
import pkgutil
import importlib
import trace
from work_queue import WorkQueue
from sandesh_logger import SandeshLogger
from sandesh_client import SandeshClient
from sandesh_http import SandeshHttp
from sandesh_uve import SandeshUVETypeMaps, SandeshUVEPerTypeMap
from sandesh_stats import SandeshStats
from sandesh_trace import SandeshTraceRequestRunner
from util import *
from gen_py.sandesh.ttypes import SandeshType, SandeshLevel
from gen_py.sandesh.constants import *


class Sandesh(object):
    _DEFAULT_LOG_FILE = SandeshLogger._DEFAULT_LOG_FILE
    _DEFAULT_SYSLOG_FACILITY = SandeshLogger._DEFAULT_SYSLOG_FACILITY

    class SandeshRole:
        INVALID = 0
        GENERATOR = 1
        COLLECTOR = 2
    # end class SandeshRole

    def __init__(self):
        self._context = ''
        self._scope = ''
        self._module = ''
        self._source = ''
        self._node_type = ''
        self._instance_id = ''
        self._timestamp = 0
        self._versionsig = 0
        self._type = 0
        self._hints = 0
        self._client_context = ''
        self._client = None
        self._role = self.SandeshRole.INVALID
        self._logger = None
        self._level = SandeshLevel.INVALID
        self._category = ''
        self._send_queue_enabled = True
        self._http_server = None
        self._connect_to_collector = True
    # end __init__

    # Public functions

    def init_generator(self, module, source, node_type, instance_id,
                       collectors, client_context, 
                       http_port, sandesh_req_uve_pkg_list=None,
                       discovery_client=None, connect_to_collector=True):
        self._role = self.SandeshRole.GENERATOR
        self._module = module
        self._source = source
        self._node_type = node_type
        self._instance_id = instance_id
        self._client_context = client_context
        self._collectors = collectors
        self._connect_to_collector = connect_to_collector
        self._rcv_queue = WorkQueue(self._process_rx_sandesh)
        self._init_logger(source + ':' + module + ':' + node_type + ':' \
            + instance_id)
        self._logger.info('SANDESH: CONNECT TO COLLECTOR: %s',
            connect_to_collector)
        self._stats = SandeshStats()
        self._trace = trace.Trace()
        self._sandesh_request_dict = {}
        self._uve_type_maps = SandeshUVETypeMaps()
        if sandesh_req_uve_pkg_list is None:
            sandesh_req_uve_pkg_list = []
        # Initialize the request handling
        # Import here to break the cyclic import dependency
        import sandesh_req_impl
        sandesh_req_impl = sandesh_req_impl.SandeshReqImpl(self)
        sandesh_req_uve_pkg_list.append('pysandesh.gen_py')
        for pkg_name in sandesh_req_uve_pkg_list:
            self._create_sandesh_request_and_uve_lists(pkg_name)
        if http_port != -1:
            self._http_server = SandeshHttp(
                self, module, http_port, sandesh_req_uve_pkg_list)
            gevent.spawn(self._http_server.start_http_server)
        primary_collector = None
        secondary_collector = None
        if self._collectors is not None:
            if len(self._collectors) > 0:
                primary_collector = self._collectors[0]
            if len(self._collectors) > 1:
                secondary_collector = self._collectors[1]
        if self._connect_to_collector:
            self._client = SandeshClient(
                self, primary_collector, secondary_collector,
                discovery_client)
            self._client.initiate()
    # end init_generator

    def record_port(self, name, port):
        pipe_name = '/tmp/%s.%d.%s_port' % (self._module, os.getppid(), name)
        try:
            pipeout = os.open(pipe_name, os.O_WRONLY)
        except:
            self._logger.error('Cannot write %s_port %d to %s' % (name, port, pipe_name))
        else:
            self._logger.error('Writing %s_port %d to %s' % (name, port, pipe_name))
            os.write(pipeout, '%d\n' % port)
            os.close(pipeout)
        
    def logger(self):
        return self._logger
    # end logger

    def sandesh_logger(self):
        return self._sandesh_logger
    # end sandesh_logger

    def set_logging_params(self, enable_local_log=False, category='',
                           level=SandeshLevel.SYS_INFO,
                           file=SandeshLogger._DEFAULT_LOG_FILE,
                           enable_syslog=False,
                           syslog_facility=_DEFAULT_SYSLOG_FACILITY):
        self._sandesh_logger.set_logging_params(
            enable_local_log, category, level, file,
            enable_syslog, syslog_facility)
    # end set_logging_params

    def set_local_logging(self, enable_local_log):
        self._sandesh_logger.set_local_logging(enable_local_log)
    # end set_local_logging

    def set_logging_level(self, level):
        self._sandesh_logger.set_logging_level(level)
    # end set_logging_level

    def set_logging_category(self, category):
        self._sandesh_logger.set_logging_category(category)
    # end set_logging_category

    def set_logging_file(self, file):
        self._sandesh_logger.set_logging_file(file)
    # end set_logging_file

    def is_send_queue_enabled(self):
        return self._send_queue_enabled
    # end is_send_queue_enabled

    def is_connect_to_collector_enabled(self):
        return self._connect_to_collector
    # end is_connect_to_collector_enabled

    def set_send_queue(self, enable):
        if self._send_queue_enabled != enable:
            self._logger.info("SANDESH: CLIENT: SEND QUEUE: %s -> %s",
                              self._send_queue_enabled, enable)
            self._send_queue_enabled = enable
            if enable:
                connection = self._client.connection()
                if connection and connection.session():
                    connection.session().send_queue().may_be_start_runner()
    # end set_send_queue

    def init_collector(self):
        pass
    # end init_collector

    def stats(self):
        return self._stats
    # end stats

    @classmethod
    def next_seqnum(cls):
        if not hasattr(cls, '_lseqnum'):
            cls._lseqnum = 1
        else:
            cls._lseqnum += 1
        return cls._lseqnum
    # end next_seqnum

    @classmethod
    def lseqnum(cls):
        if not hasattr(cls, '_lseqnum'):
            cls._lseqnum = 0
        return cls._lseqnum
    # end lseqnum

    def module(self):
        return self._module
    # end module

    def source_id(self):
        return self._source
    # end source_id

    def node_type(self):
        return self._node_type
    #end node_type

    def instance_id(self):
        return self._instance_id
    #end instance_id

    def scope(self):
        return self._scope
    # end scope

    def context(self):
        return self._context
    # end context

    def seqnum(self):
        return self._seqnum
    # end seqnum

    def timestamp(self):
        return self._timestamp
    # end timestamp

    def versionsig(self):
        return self._versionsig
    # end versionsig

    def type(self):
        return self._type
    # end type

    def hints(self):
        return self._hints
    # end hints

    def client(self):
        return self._client
    # end client

    def level(self):
        return self._level
    # end level

    def category(self):
        return self._category
    # end category

    def validate(self):
        return
    # end validate

    def is_local_logging_enabled(self):
        return self._sandesh_logger.is_local_logging_enabled()
    # end is_local_logging_enabled

    def logging_level(self):
        return self._sandesh_logger.logging_level()
    # end logging_level

    def logging_category(self):
        return self._sandesh_logger.logging_category()
    # end logging_category

    def is_syslog_logging_enabled(self):
        return self._sandesh_logger.is_syslog_logging_enabled()
    #end is_syslog_logging_enabled

    def logging_syslog_facility(self):
        return self._sandesh_logger.logging_syslog_facility()
    #end logging_syslog_facility

    def is_unit_test(self):
        return self._role == self.SandeshRole.INVALID
    # end is_unit_test

    def handle_test(self, sandesh_init):
        if sandesh_init.is_unit_test() or self._is_level_ut():
            if self._is_logging_allowed(sandesh_init):
                sandesh_init._logger.debug(self.log())
                return True
        return False

    def is_logging_allowed(self, sandesh_init):
        if not sandesh_init.is_local_logging_enabled():
            return False

        logging_level = sandesh_init.logging_level()
        level_allowed = logging_level >= self._level

        logging_category = sandesh_init.logging_category()
        if logging_category is None or len(logging_category) == 0:
            category_allowed = True
        else:
            category_allowed = logging_category == self._category

        return level_allowed and category_allowed
    # end is_logging_allowed

    def enqueue_sandesh_request(self, sandesh):
        self._rcv_queue.enqueue(sandesh)
    # end enqueue_sandesh_request

    def send_sandesh(self, tx_sandesh):
        if self._client:
            ret = self._client.send_sandesh(tx_sandesh)
        else:
            if self._connect_to_collector:
                self._logger.error('SANDESH: No Client: %s', tx_sandesh.log())
            else:
                self._logger.log(
                    SandeshLogger.get_py_logger_level(tx_sandesh.level()),
                    tx_sandesh.log())
    # end send_sandesh

    def send_generator_info(self):
        from gen_py.sandesh_uve.ttypes import SandeshClientInfo, \
            ModuleClientState, SandeshModuleClientTrace
        client_info = SandeshClientInfo()
        try:
            client_start_time = self._start_time
        except:
            self._start_time = UTCTimestampUsec()
        finally:
            client_info.start_time = self._start_time
            client_info.pid = os.getpid()
            if self._http_server is not None:
                client_info.http_port = self._http_server.get_port()
            client_info.collector_name = self._client.connection().collector()
            client_info.status = self._client.connection().state()
            client_info.successful_connections = \
                self._client.connection().statemachine().connect_count()
            client_info.primary = self._client.connection().primary_collector()
            if client_info.primary is None:
                client_info.primary = ''
            client_info.secondary = \
                self._client.connection().secondary_collector()
            if client_info.secondary is None:
                client_info.secondary = ''
            module_state = ModuleClientState(name=self._source + ':' +
                                             self._node_type + ':' + 
                                             self._module + ':' +
                                             self._instance_id, 
                                             client_info=client_info)
            generator_info = SandeshModuleClientTrace(
                data=module_state, sandesh=self)
            generator_info.send(sandesh=self)
    # end send_generator_info

    def get_sandesh_request_object(self, request):
        try:
            req_module = self._sandesh_request_dict[request]
        except KeyError:
            self._logger.error('Invalid Sandesh Request "%s"' % (request))
            return None
        else:
            if req_module:
                try:
                    imp_module = importlib.import_module(req_module)
                except ImportError:
                    self._logger.error(
                        'Failed to import Module "%s"' % (req_module))
                else:
                    try:
                        sandesh_request = getattr(imp_module, request)()
                        return sandesh_request
                    except AttributeError:
                        self._logger.error(
                            'Failed to create Sandesh Request "%s"' %
                            (request))
                        return None
            else:
                self._logger.error(
                    'Sandesh Request "%s" not implemented' % (request))
                return None
    # end get_sandesh_request_object

    def trace_enable(self):
        self._trace.TraceOn()
    # end trace_enable

    def trace_disable(self):
        self._trace.TraceOff()
    # end trace_disable

    def is_trace_enabled(self):
        return self._trace.IsTraceOn()
    # end is_trace_enabled

    def trace_buffer_create(self, name, size, enable=True):
        self._trace.TraceBufAdd(name, size, enable)
    # end trace_buffer_create

    def trace_buffer_delete(self, name):
        self._trace.TraceBufDelete(name)
    # end trace_buffer_delete

    def trace_buffer_enable(self, name):
        self._trace.TraceBufOn(name)
    # end trace_buffer_enable

    def trace_buffer_disable(self, name):
        self._trace.TraceBufOff(name)
    # end trace_buffer_disable

    def is_trace_buffer_enabled(self, name):
        return self._trace.IsTraceBufOn(name)
    # end is_trace_buffer_enabled

    def trace_buffer_list_get(self):
        return self._trace.TraceBufListGet()
    # end trace_buffer_list_get

    def trace_buffer_size_get(self, name):
        return self._trace.TraceBufSizeGet(name)
    # end trace_buffer_size_get

    def trace_buffer_read(self, name, read_context, count, read_cb):
        self._trace.TraceRead(name, read_context, count, read_cb)
    # end trace_buffer_read

    def trace_buffer_read_done(self, name, context):
        self._trace.TraceReadDone(name, context)
    # end trace_buffer_read_done

    # API to send the trace buffer to the Collector.
    # If trace count is not specified/or zero, then the entire trace buffer
    # is sent to the Collector.
    # [Note] No duplicate trace message sent to the Collector. i.e., If there
    # is no trace message added between two consequent calls to this API, then
    # no trace message is sent to the Collector.
    def send_sandesh_trace_buffer(self, trace_buf, count=0):
        trace_req_runner = SandeshTraceRequestRunner(sandesh=self,
                                                     request_buffer_name=
                                                     trace_buf,
                                                     request_context='',
                                                     read_context='Collector',
                                                     request_count=count)
        trace_req_runner.Run()
    # end send_sandesh_trace_buffer

    # Private functions

    def _is_level_ut(self):
        return self._level >= SandeshLevel.UT_START and \
            self._level <= SandeshLevel.UT_END
    # end _is_level_ut

    def _create_task(self):
        return gevent.spawn(self._runner.run_for_ever)
    # end _create_task

    def _process_rx_sandesh(self, rx_sandesh):
        handle_request_fn = getattr(rx_sandesh, "handle_request", None)
        if callable(handle_request_fn):
            handle_request_fn(rx_sandesh)
        else:
            self._logger.error('Sandesh Request "%s" not implemented' %
                               (rx_sandesh.__class__.__name__))
    # end _process_rx_sandesh

    def _create_sandesh_request_and_uve_lists(self, package):
        try:
            imp_pkg = __import__(package)
        except ImportError:
            self._logger.error('Failed to import package "%s"' % (package))
        else:
            try:
                pkg_path = imp_pkg.__path__
            except AttributeError:
                self._logger.error(
                    'Failed to get package [%s] path' % (package))
                return
            for importer, mod, ispkg in \
                pkgutil.walk_packages(path=pkg_path,
                                      prefix=imp_pkg.__name__ + '.'):
                if not ispkg:
                    module = mod.rsplit('.', 1)[-1]
                    if 'ttypes' == module:
                        self._logger.debug(
                            'Add Sandesh requests in module "%s"' % (mod))
                        self._add_sandesh_request(mod)
                        self._logger.debug(
                            'Add Sandesh UVEs in module "%s"' % (mod))
                        self._add_sandesh_uve(mod)
    # end _create_sandesh_request_and_uve_lists

    def _add_sandesh_request(self, mod):
        try:
            imp_module = importlib.import_module(mod)
        except ImportError:
            self._logger.error('Failed to import Module "%s"' % (mod))
        else:
            try:
                sandesh_req_list = getattr(imp_module, '_SANDESH_REQUEST_LIST')
            except AttributeError:
                self._logger.error(
                    '"%s" module does not have sandesh request list' % (mod))
            else:
                # Add sandesh requests to the dictionary.
                for req in sandesh_req_list:
                    self._sandesh_request_dict[req] = mod
    # end _add_sandesh_request

    def _get_sandesh_uve_list(self, imp_module):
        try:
            sandesh_uve_list = getattr(imp_module, '_SANDESH_UVE_LIST')
        except AttributeError:
            self._logger.error(
                '"%s" module does not have sandesh UVE list' %
                (imp_module.__name__))
            return None
        else:
            return sandesh_uve_list
    # end _get_sandesh_uve_list

    def _get_sandesh_uve_data_list(self, imp_module):
        try:
            sandesh_uve_data_list = getattr(imp_module, '_SANDESH_UVE_DATA_LIST')
        except AttributeError:
            self._logger.error(
                '"%s" module does not have sandesh UVE data list' %
                (imp_module.__name__))
            return None
        else:
            return sandesh_uve_data_list
    # end _get_sandesh_uve_data_list

    def _add_sandesh_uve(self, mod):
        try:
            imp_module = importlib.import_module(mod)
        except ImportError:
            self._logger.error('Failed to import Module "%s"' % (mod))
        else:
            sandesh_uve_list = self._get_sandesh_uve_list(imp_module)
            sandesh_uve_data_list = self._get_sandesh_uve_data_list(imp_module)
            if sandesh_uve_list is None or sandesh_uve_data_list is None:
                return
            if len(sandesh_uve_list) != len(sandesh_uve_data_list):
                self._logger.error(
                    '"%s" module sandesh UVE and UVE data list do not match' %
                     (mod))
                return
            sandesh_uve_info_list = zip(sandesh_uve_list, sandesh_uve_data_list)
            # Register sandesh UVEs
            for uve_type_name, uve_data_type_name in sandesh_uve_info_list:
                SandeshUVEPerTypeMap(self, uve_type_name, uve_data_type_name, mod)
    # end _add_sandesh_uve

    def _init_logger(self, generator):
        if not generator:
            generator = 'sandesh'
        self._sandesh_logger = SandeshLogger(generator)
        self._logger = self._sandesh_logger.logger()
    # end _init_logger

# end class Sandesh

sandesh_global = Sandesh()


class SandeshAsync(Sandesh):

    def __init__(self):
        Sandesh.__init__(self)
    # end __init__

    def send(self, sandesh=sandesh_global):
        try:
            self.validate()
        except e:
            sandesh._logger.error('sandesh "%s" validation failed [%s]' %
                                 (self.__class__.__name__, e))
            return -1
        self._seqnum = self.next_seqnum()
        if self.handle_test(sandesh):
            return 0
        sandesh.send_sandesh(self)
        return 0
    # end send

# end class SandeshAsync


class SandeshSystem(SandeshAsync):

    def __init__(self):
        SandeshAsync.__init__(self)
        self._type = SandeshType.SYSTEM
    # end __init__

# end class SandeshSystem


class SandeshObject(SandeshAsync):

    def __init__(self):
        SandeshAsync.__init__(self)
        self._type = SandeshType.OBJECT
    # end __init__

# end class SandeshObject


class SandeshFlow(SandeshAsync):

    def __init__(self):
        SandeshAsync.__init__(self)
        self._type = SandeshType.FLOW
    # end __init__

# end class SandeshFlow


class SandeshRequest(Sandesh):

    def __init__(self):
        Sandesh.__init__(self)
        self._type = SandeshType.REQUEST
    # end __init__

    def request(self, context='', sandesh=sandesh_global):
        try:
            self.validate()
        except e:
            sandesh._logger.error('sandesh "%s" validation failed [%s]' %
                                 (self.__class__.__name__, e))
            return -1
        if context == 'ctrl':
            self._hints |= SANDESH_CONTROL_HINT
        self._context = context
        self._seqnum = self.next_seqnum()
        if self.handle_test(sandesh):
            return 0
        sandesh.send_sandesh(self)
        return 0
    # end request

# end class SandeshRequest


class SandeshResponse(Sandesh):

    def __init__(self):
        Sandesh.__init__(self)
        self._type = SandeshType.RESPONSE
        self._more = False
    # end __init__

    def response(self, context='', more=False, sandesh=sandesh_global):
        try:
            self.validate()
        except e:
            sandesh._logger.error('sandesh "%s" validation failed [%s]' %
                                 (self.__class__.__name__, e))
            return -1
        self._context = context
        self._more = more
        self._seqnum = self.next_seqnum()
        if self._context.find('http://') == 0:
            SandeshHttp.create_http_response(self, sandesh)
        else:
            if self.handle_test(sandesh):
                return 0
            sandesh.send_sandesh(self)
        return 0
    # end response

# end class SandeshResponse


class SandeshUVE(Sandesh):

    def __init__(self):
        Sandesh.__init__(self)
        self._type = SandeshType.UVE
        self._more = False
    # end __init__

    def send(self, isseq=False, seqno=0, context='',
             more=False, sandesh=sandesh_global):
        try:
            self.validate()
        except e:
            sandesh._logger.error('sandesh "%s" validation failed [%s]' %
                                 (self.__class__.__name__, e))
            return -1
        if isseq is True:
            self._seqnum = seqno
        else:
            uve_type_map = sandesh._uve_type_maps.get_uve_type_map(
                self.__class__.__name__)
            if uve_type_map is None:
                return -1
            self._seqnum = self.next_seqnum()
            uve_type_map.update_uve(self)
        self._context = context
        self._more = more
        if self._context.find('http://') == 0:
            SandeshHttp.create_http_response(self, sandesh)
        else:
            if self.handle_test(sandesh):
                return 0
            if sandesh._client:
                sandesh._client.send_uve_sandesh(self)
            else:
                sandesh._logger.debug(self.log())
        return 0
    # end send

# end class SandeshUVE


class SandeshAlarm(SandeshUVE):

    def __init__(self):
        SandeshUVE.__init__(self)
        self._type = SandeshType.ALARM
    # end __init__

# end class SandeshAlarm


class SandeshTrace(Sandesh):

    def __init__(self, type):
        Sandesh.__init__(self)
        self._type = type
        self._more = False
    # end __init__

    def send_trace(self, context='', more=False,
                   sandesh=sandesh_global):
        try:
            self.validate()
        except e:
            sandesh._logger.error('sandesh "%s" validation failed [%s]' %
                                 (self.__class__.__name__, e))
            return -1
        self._context = context
        self._more = more
        if self._context.find('http://') == 0:
            SandeshHttp.create_http_response(self, sandesh)
        else:
            if self.handle_test(sandesh):
                return 0
            sandesh.send_sandesh(self)
        return 0
    # end send_trace

    def trace_msg(self, name, sandesh=sandesh_global):
        if sandesh._trace.IsTraceOn() and sandesh._trace.IsTraceBufOn(name):
            # store the trace buffer name in category
            self._category = name
            self._seqnum = sandesh._trace.TraceWrite(name, self)
    # end trace_msg

# end class SandeshTrace
