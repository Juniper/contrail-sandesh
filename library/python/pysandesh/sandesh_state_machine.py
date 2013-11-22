#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# Sandesh State Machine
#

import gevent
from fysom import Fysom
from work_queue import WorkQueue
from sandesh_logger import SandeshLogger
from sandesh_session import SandeshSession

class State(object):
    # FSM states
    _IDLE = 'Idle'
    _DISCONNECT = 'Disconnect'
    _CONNECT = 'Connect'
    _CONNECT_TO_BACKUP = 'ConnectToBackup'
    _CLIENT_INIT = 'ClientInit'
    _ESTABLISHED = 'Established'

#end class State

class Event(object):
    # FSM events
    _EV_START = 'EvStart'
    _EV_STOP = 'EvStop'
    _EV_IDLE_HOLD_TIMER_EXPIRED = 'EvIdleHoldTimerExpired'
    _EV_CONNECT_TIMER_EXPIRED = 'EvConnectTimerExpired'
    _EV_COLLECTOR_UNKNOWN = 'EvCollectorUnknown'
    _EV_BACKUP_COLLECTOR_UNKNOWN = 'EvBackupCollectorUnknown'
    _EV_TCP_CONNECTED = 'EvTcpConnected'
    _EV_TCP_CONNECT_FAIL = 'EvTcpConnectFail'
    _EV_TCP_CLOSE = 'EvTcpClose'
    _EV_COLLECTOR_CHANGE = 'EvCollectorChange'
    _EV_SANDESH_CTRL_MESSAGE_RECV = 'EvSandeshCtrlMessageRecv'
    _EV_SANDESH_UVE_SEND = 'EvSandeshUVESend'

    def __init__(self, event, session=None, msg=None, source=None,
                 primary_collector=None, secondary_collector=None): 
        self.event = event
        self.session = session
        self.msg = msg
        self.source = source
        self.primary_collector = primary_collector
        self.secondary_collector = secondary_collector
    #end __init__

#end class Event

class SandeshStateMachine(object):

    _IDLE_HOLD_TIME = 5 # in seconds
    _CONNECT_TIME = 30 # in seconds

    def __init__(self, connection, logger, primary_collector, 
                 secondary_collector):

        def _on_idle(e):
            if e.sm._connect_timer is not None:
                e.sm._cancel_connect_timer()
            # Reset active and backup collector
            self._active_collector = self._connection.primary_collector()
            self._backup_collector = self._connection.secondary_collector()
            # clean up existing connection
            e.sm._delete_session()
            e.sm._start_idle_hold_timer()
        #end _on_idle

        def _on_disconnect(e):
            pass
        #end _on_disconnect

        def _on_connect(e):
            if e.sm._idle_hold_timer is not None:
                e.sm._cancel_idle_hold_timer()
            e.sm._connection.reset_collector()
            # clean up existing connection
            e.sm._delete_session()
            if e.sm._active_collector is not None:
                e.sm._create_session()
                e.sm._start_connect_timer()
                e.sm._session.connect()
            else:
                e.sm.enqueue_event(Event(event = Event._EV_COLLECTOR_UNKNOWN))
        #end _on_connect

        def _on_connect_to_backup(e):
            if e.sm._connect_timer is not None:
                e.sm._cancel_connect_timer()
            # clean up existing connection
            e.sm._delete_session()
            # try to connect to the backup collector, if known
            if e.sm._backup_collector is not None:
                e.sm._active_collector, e.sm._backup_collector = \
                    e.sm._backup_collector, e.sm._active_collector
                e.sm._create_session()
                e.sm._start_connect_timer()
                e.sm._session.connect()
            else:
                e.sm.enqueue_event(Event(event = Event._EV_BACKUP_COLLECTOR_UNKNOWN))
        #end _on_connect_to_backup

        def _on_client_init(e):
            e.sm._connects += 1
            gevent.spawn(e.sm._session.read)
            e.sm._connection.handle_initialized(e.sm._connects)
            e.sm._connection.sandesh_instance().send_generator_info()
        #end _on_client_init
        
        def _on_established(e):
            e.sm._cancel_connect_timer()
            e.sm._connection.set_collector(e.sm_event.source)
            e.sm._connection.handle_sandesh_ctrl_msg(e.sm_event.msg)
            self._connection.sandesh_instance().send_generator_info()
        #end _on_established

        # FSM - Fysom
        self._fsm = Fysom({
                           'initial': {'state' : State._IDLE,
                                       'event' : Event._EV_START,
                                       'defer' : True
                                      },
                           'events': [
                                      # _IDLE
                                      {'name' : Event._EV_IDLE_HOLD_TIMER_EXPIRED,
                                       'src'  : State._IDLE,
                                       'dst'  : State._CONNECT
                                      },
                                      {'name' : Event._EV_COLLECTOR_CHANGE,
                                       'src'  : State._IDLE,
                                       'dst'  : State._CONNECT
                                      },

                                      # _DISCONNECT 
                                      {'name' : Event._EV_COLLECTOR_CHANGE,
                                       'src'  : State._DISCONNECT,
                                       'dst'  : State._CONNECT
                                      },

                                      # _CONNECT
                                      {'name' : Event._EV_COLLECTOR_UNKNOWN,
                                       'src'  : State._CONNECT,
                                       'dst'  : State._DISCONNECT
                                      },
                                      {'name' : Event._EV_TCP_CONNECT_FAIL,
                                       'src'  : State._CONNECT,
                                       'dst'  : State._CONNECT_TO_BACKUP
                                      },
                                      {'name' : Event._EV_CONNECT_TIMER_EXPIRED,
                                       'src'  : State._CONNECT,
                                       'dst'  : State._CONNECT_TO_BACKUP
                                      },
                                      {'name' : Event._EV_COLLECTOR_CHANGE,
                                       'src'  : State._CONNECT,
                                       'dst'  : State._IDLE
                                      },
                                      {'name' : Event._EV_TCP_CONNECTED,
                                       'src'  : State._CONNECT,
                                       'dst'  : State._CLIENT_INIT
                                      },

                                      # _CONNECT_TO_BACKUP
                                      {'name' : Event._EV_BACKUP_COLLECTOR_UNKNOWN,
                                       'src'  : State._CONNECT_TO_BACKUP,
                                       'dst'  : State._IDLE
                                      },
                                      {'name' : Event._EV_TCP_CONNECT_FAIL,
                                       'src'  : State._CONNECT_TO_BACKUP,
                                       'dst'  : State._IDLE
                                      },
                                      {'name' : Event._EV_CONNECT_TIMER_EXPIRED,
                                       'src'  : State._CONNECT_TO_BACKUP,
                                       'dst'  : State._IDLE
                                      },
                                      {'name' : Event._EV_COLLECTOR_CHANGE,
                                       'src'  : State._CONNECT_TO_BACKUP,
                                       'dst'  : State._IDLE
                                      },
                                      {'name' : Event._EV_TCP_CONNECTED,
                                       'src'  : State._CONNECT_TO_BACKUP,
                                       'dst'  : State._CLIENT_INIT
                                      },

                                      # _CLIENT_INIT
                                      {'name' : Event._EV_CONNECT_TIMER_EXPIRED,
                                       'src'  : State._CLIENT_INIT,
                                       'dst'  : State._IDLE
                                      },
                                      {'name' : Event._EV_TCP_CLOSE,
                                       'src'  : State._CLIENT_INIT,
                                       'dst'  : State._IDLE
                                      },
                                      {'name' : Event._EV_COLLECTOR_CHANGE,
                                       'src'  : State._CLIENT_INIT,
                                       'dst'  : State._IDLE
                                      },
                                      {'name' : Event._EV_SANDESH_CTRL_MESSAGE_RECV,
                                       'src'  : State._CLIENT_INIT,
                                       'dst'  : State._ESTABLISHED
                                      },

                                      # _ESTABLISHED
                                      {'name' : Event._EV_TCP_CLOSE,
                                       'src'  : State._ESTABLISHED,
                                       'dst'  : State._CONNECT_TO_BACKUP
                                      },
                                      {'name' : Event._EV_COLLECTOR_CHANGE,
                                       'src'  : State._ESTABLISHED,
                                       'dst'  : State._CONNECT
                                      }
                                     ],
                           'callbacks': {
                                         'on' + State._IDLE : _on_idle,
                                         'on' + State._CONNECT : _on_connect,
                                         'on' + State._CONNECT_TO_BACKUP : _on_connect_to_backup,
                                         'on' + State._CLIENT_INIT : _on_client_init,
                                         'on' + State._ESTABLISHED : _on_established,
                                        }
                          })

        self._connection = connection
        self._session = None
        self._connects = 0
        self._idle_hold_timer = None
        self._connect_timer = None
        self._active_collector = primary_collector
        self._backup_collector = secondary_collector
        self._logger = logger
        self._event_queue = WorkQueue(self._dequeue_event,
                                      self._is_ready_to_dequeue_event)
    #end __init__

    # Public functions

    def initialize(self):
        self.enqueue_event(Event(event = Event._EV_START))
    #end initialize

    def session(self):
        return self._session 
    #end session 

    def state(self):
        return self._fsm.current
    #end state 

    def shutdown(self):
        self.enqueue_event(Event(event = Event._EV_STOP))
    #end shutdown

    def set_admin_state(self, down):
        if down == True:
            self.enqueue_event(Event(event = Event._EV_STOP))
        else:
            self.enqueue_event(Event(event = Event._EV_START))
    #end set_admin_state

    def connect_count(self):
        return self._connects
    #end connect_count

    def active_collector(self):
        return self._active_collector
    #end active_collector

    def backup_collector(self):
        return self._backup_collector
    #end backup_collector

    def enqueue_event(self, event):
        self._event_queue.enqueue(event)
    #end enqueue_event

    def on_session_event(self, session, event):
        if session is not self._session:
            self._logger.error("Ignore session event [%d] received for old session" % (event))
            return 
        if SandeshSession.SESSION_ESTABLISHED == event:
            self._logger.info("Session Event: TCP Connected")
            self.enqueue_event(Event(event = Event._EV_TCP_CONNECTED,
                                     session = session))
        elif SandeshSession.SESSION_ERROR == event:
            self._logger.error("Session Event: TCP Connect Fail")
            self.enqueue_event(Event(event = Event._EV_TCP_CONNECT_FAIL,
                                     session = session))
        elif SandeshSession.SESSION_CLOSE == event:
            self._logger.error("Session Event: TCP Connection Closed")
            self.enqueue_event(Event(event = Event._EV_TCP_CLOSE,
                                     session = session))
        else:
            self._logger.error("Received unknown session event [%d]" % (event))
    #end on_session_event

    def on_sandesh_ctrl_msg_receive(self, session, sandesh_ctrl, collector):
        if sandesh_ctrl.success == True:
            self.enqueue_event(Event(event = Event._EV_SANDESH_CTRL_MESSAGE_RECV, 
                                     session = session,
                                     msg = sandesh_ctrl,
                                     source = collector))
        else:
            # Negotiation with the Collector failed, reset the 
            # connection and retry after sometime.
            self._logger.error("Negotiation with the Collector %s failed." % (collector))
            self._session.close()
    #end on_sandesh_ctrl_msg_receive

    def on_sandesh_uve_msg_send(self, sandesh_uve):
        self.enqueue_event(Event(event = Event._EV_SANDESH_UVE_SEND,
                                 msg = sandesh_uve))
    #end on_sandesh_uve_msg_send

    # Private functions

    def _create_session(self):
        assert self._session is None
        col_info = self._active_collector.split(':')
        collector = (col_info[0], int(col_info[1]))
        self._session = SandeshSession(self._connection.sandesh_instance(),
                                       collector,
                                       self.on_session_event,
                                       self._connection._receive_sandesh_msg) 
    #end _create_session

    def _delete_session(self):
        if self._session:
            self._session.close()
            self._session = None
            self._connection.reset_collector()
    #end _delete_session 

    def _start_idle_hold_timer(self):
        if self._idle_hold_timer is None:
            if self._IDLE_HOLD_TIME:
                self._idle_hold_timer = gevent.spawn_later(self._IDLE_HOLD_TIME,
                                            self._idle_hold_timer_expiry_handler)
            else:
                self.enqueue_event(Event(event = Event._EV_IDLE_HOLD_TIMER_EXPIRED))
    #end _start_idle_hold_timer

    def _cancel_idle_hold_timer(self):
        if self._idle_hold_timer is not None:
            gevent.kill(self._idle_hold_timer)
            self._idle_hold_timer = None
    #end _cancel_idle_hold_timer

    def _idle_hold_timer_expiry_handler(self):
        self._idle_hold_timer = None
        self.enqueue_event(Event(event = Event._EV_IDLE_HOLD_TIMER_EXPIRED))
    #end _idle_hold_timer_expiry_handler
    
    def _start_connect_timer(self):
        if self._connect_timer is None:
            self._connect_timer = gevent.spawn_later(self._CONNECT_TIME,
                                        self._connect_timer_expiry_handler, 
                                        self._session)
    #end _start_connect_timer

    def _cancel_connect_timer(self):
        if self._connect_timer is not None:
            gevent.kill(self._connect_timer)
            self._connect_timer = None
    #end _cancel_connect_timer

    def _connect_timer_expiry_handler(self, session):
        self._connect_timer = None
        self.enqueue_event(Event(event = Event._EV_CONNECT_TIMER_EXPIRED,
                                 session = session))
    #end _connect_timer_expiry_handler

    def _is_ready_to_dequeue_event(self):
        return True
    #end _is_ready_to_dequeue_event

    def _log_event(self, event):
        if self._fsm.current == State._ESTABLISHED and \
           event.event == Event._EV_SANDESH_UVE_SEND:
           return False
        return True
    #end _log_event

    def _dequeue_event(self, event):
        if self._log_event(event):
            self._logger.info("Processing event[%s] in state[%s]" \
                              % (event.event, self._fsm.current))
        if event.session is not None and event.session is not self._session:
            self._logger.info("Ignore event [%s] received for old session" \
                              % (event.event))
            return
        if event.event == Event._EV_COLLECTOR_CHANGE:
            old_active_collector = self._active_collector
            self._active_collector = event.primary_collector
            self._backup_collector = event.secondary_collector
            if old_active_collector == self._active_collector:
                self._logger.info("No change in active collector. Ignore event [%s]" \
                                  % (event.event))
                return
        if event.event == Event._EV_SANDESH_UVE_SEND:
            if self._fsm.current == State._ESTABLISHED or self._fsm.current == State._CLIENT_INIT:
                self._connection.handle_sandesh_uve_msg(event.msg)
            else:
                self._logger.info("Discarding event[%s] in state[%s]" \
                                  % (event.event, self._fsm.current))
        elif event.event == Event._EV_SANDESH_CTRL_MESSAGE_RECV and \
                self._fsm.current == State._ESTABLISHED:
            self._connection.handle_sandesh_ctrl_msg(event.msg)
        elif self._fsm.cannot(event.event) is True:
            self._logger.info("Unconsumed event[%s] in state[%s]" \
                              % (event.event, self._fsm.current))
        else:
            prev_state = self.state()
            getattr(self._fsm, event.event)(sm = self, sm_event = event)
            # Log state transition
            self._logger.info("Sandesh Client: Event[%s] => State[%s] -> State[%s]" \
                              % (event.event, prev_state, self.state()))
    #end _dequeue_event

#end class SandeshStateMachine
