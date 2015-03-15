from contextlib import contextmanager

from .event import RecvIdEvent
from ._internal.simconnect import SIMCONNECT_RECV_ID
from ._internal.connection import Connection

import logging

@contextmanager
def connect( name,
             auto_close=True,  
             window_handle=None,
             user_event_win32=0,
             event_handle=None,
             config_index=0):

    connection = Connection()
    connection.open(name, window_handle, user_event_win32, event_handle, config_index)
    close_event = None
    if auto_close:
        close_event = RecvIdEvent(recv_id=SIMCONNECT_RECV_ID.SIMCONNECT_RECV_ID_QUIT,
                                  callback=connection.close)
        connection.subscribe(close_event)

    logging.debug('Yielding connection %s', connection)
    yield connection
    connection.close()
