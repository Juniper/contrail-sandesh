#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

import socket

def get_free_port():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(("", 0))
    port = sock.getsockname()[1]
    sock.close() 
    return port

