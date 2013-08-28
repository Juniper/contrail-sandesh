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

class Event(object):

    def __init__(self, event = None, session = None, msg = None, source = None):
        self.event = event
        self.session = session
        self.msg = msg
        self.source = source
    #end __init__

#end class Event

class SandeshStateMachine(object):

    # FSM states
    _IDLE = 'Idle'
    _CONNECT = 'Connect'
    _ESTABLISHED = 'Established'
    _CLIENT_INIT = 'ClientInit'

    # FSM events
    _EV_START = 'EvStart'
    _EV_STOP = 'EvStop'
    _EV_IDLE_HOLD_TIMER_EXPIRED = 'EvIdleHoldTimerExpired'
    _EV_TCP_CONNECTED = 'EvTcpConnected'
    _EV_TCP_CONNECT_FAIL = 'EvTcpConnectFail'
    _EV_TCP_CLOSE = 'EvTcpClose'
    _EV_SANDESH_MESSAGE_RECV = 'EvSandeshMessageRecv'
    _EV_SANDESH_CTRL_MESSAGE_RECV = 'EvSandeshCtrlMessageRecv'
    _EV_SANDESH_UVE_SEND = 'EvSandeshUVESend'

    _IDLE_HOLD_TIME = 0

    def __init__(self, connection):

        def _on_idle(e):
            e.sm._logger.info("Sandesh Client: State[Idle]")
            if e.sm._IDLE_HOLD_TIME != 0:
                if e.sm._idle_hold_timer is None:
                    e.sm._idle_hold_timer = gevent.spawn_later(e.sm._IDLE_HOLD_TIME,
                                                               e.sm._idle_hold_timer_expired)
            else:
                e.sm._enqueue(Event(event = e.sm._EV_IDLE_HOLD_TIMER_EXPIRED,
                                    session = e.session,
                                    msg = e.msg))
        #end _on_idle

        def _on_connect(e):
            e.sm._logger.info("Sandesh Client: State[Connect]")
            e.sm._connection.reset_collector()
            e.sm._connection.connect()
        #end _on_connect

        def _on_established(e):
            e.sm._logger.info("Sandesh Client: State[Established]")
            e.sm._connection.set_collector(e.source)
            e.sm._connection.handle_sandesh_ctrl_msg(e.msg)
            self._connection.sandesh_instance().send_generator_info()
        #end _on_established

        def _on_client_init(e):
            e.sm._logger.info("Sandesh Client: State[ClientInit]")
            e.sm._connects += 1
            e.sm._connection.handle_initialized(e.sm._connects)
            self._connection.sandesh_instance().send_generator_info()
        #end _on_client_init

        # FSM - Fysom
        self._fsm = Fysom({
                           'initial': {'state' : self._IDLE,
                                       'event' : self._EV_START,
                                       'defer' : True
                                      },
                           'events': [
                                      {'name' : self._EV_IDLE_HOLD_TIMER_EXPIRED,
                                       'src' : self._IDLE,
                                       'dst' : self._CONNECT
                                      },
                                      {'name' : self._EV_TCP_CONNECTED,
                                       'src' : self._CONNECT,
                                       'dst' : self._CLIENT_INIT
                                      },
                                      {'name' : self._EV_SANDESH_CTRL_MESSAGE_RECV,
                                       'src'  : self._CLIENT_INIT,
                                       'dst'  : self._ESTABLISHED
                                      },
                                      {'name' : self._EV_TCP_CLOSE,
                                       'src'  : self._CLIENT_INIT,
                                       'dst'  : self._IDLE
                                      },
                                      {'name' : self._EV_TCP_CLOSE,
                                       'src' : self._ESTABLISHED,
                                       'dst' : self._CONNECT
                                      },
                                     ],
                           'callbacks': {
                                         'on' + self._IDLE : _on_idle,
                                         'on' + self._CONNECT : _on_connect,
                                         'on' + self._ESTABLISHED : _on_established,
                                         'on' + self._CLIENT_INIT : _on_client_init,
                                        }
                          })

        self._connection = connection
        self._connects = 0
        sandesh_logger = SandeshLogger('SandeshStateMachine')
        self._logger = sandesh_logger.logger()
        self._event_queue = WorkQueue(self._dequeue_event,
                                      self._is_ready_to_dequeue_event)
    #end __init__

    # Public functions

    def initialize(self):
        self._enqueue(Event(event = self._EV_START))
    #end initialize

    def state(self):
        return self._fsm.current
    #end state 

    def shutdown(self):
        self._enqueue(Event(event = self._EV_STOP))
    #end shutdown

    def set_admin_state(self, down):
        if down == True:
            self._enqueue(Event(event = self._EV_STOP))
        else:
            self._enqueue(Event(event = self._EV_START))
    #end set_admin_state

    def connect_count(self):
        return self._connects
    #end connect_count

    def on_session_event(self, session, event):
        if SandeshSession.SESSION_ESTABLISHED == event:
            self._logger.info("Session Event: TCP Connected")
            self._enqueue(Event(event = self._EV_TCP_CONNECTED,
                                session = session))
        elif SandeshSession.SESSION_ERROR == event:
            self._logger.error("Session Event: TCP Connect Fail")
            self._enqueue(Event(event = self._EV_TCP_CONNECT_FAIL,
                                session = session))
        elif SandeshSession.SESSION_CLOSE == event:
            self._logger.error("Session Event: TCP Connection Closed")
            self._enqueue(Event(event = self._EV_TCP_CLOSE,
                                session = session))
        else:
            self._logger.error("Received unknown session event [%d]" % (event))
    #end on_session_event

    def on_sandesh_ctrl_msg_receive(self, session, sandesh_ctrl, collector):
        if sandesh_ctrl.success == True:
            self._enqueue(Event(event = self._EV_SANDESH_CTRL_MESSAGE_RECV, 
                                session = session,
                                msg = sandesh_ctrl,
                                source = collector))
        else:
            # Negotiation with the Collector failed, reset the 
            # connection and retry after sometime.
            self._logger.error("Negotiation with the Collector %s failed." % (collector))
            self._connection.disconnect()
    #end on_sandesh_ctrl_msg_receive

    def on_sandesh_uve_msg_send(self, sandesh_uve):
        self._enqueue(Event(event = self._EV_SANDESH_UVE_SEND,
                            msg = sandesh_uve))
    #end on_sandesh_uve_msg_send

    # Private functions

    def _idle_hold_timer_expired(self):
        self._enqueue(Event(event = self._EV_IDLE_HOLD_TIMER_EXPIRED))

    def _enqueue(self, event):
        self._event_queue.enqueue(event)
    #end _enqueue

    def _is_ready_to_dequeue_event(self):
        return True
    #end _is_ready_to_dequeue_event

    def _dequeue_event(self, event):
        self._logger.info("Processing event[%s] in state[%s]" % (event.event, self._fsm.current))
        if event.event == self._EV_SANDESH_UVE_SEND:
            if self._fsm.current == self._ESTABLISHED:
                self._connection.handle_sandesh_uve_msg(event.msg)
            else:
                self._logger.info("Discarding event[%s] in state[%s]" % (event.event, self._fsm.current))
        elif self._fsm.cannot(event.event) is True:
            self._logger.info("Unconsumed event[%s] in state[%s]" % (event.event, self._fsm.current))
        else:
            result = getattr(self._fsm, event.event)(session = event.session,
                                                     msg = event.msg,
                                                     sm = self,
                                                     source = event.source)
    #end _dequeue_event

#end class SandeshStateMachine
