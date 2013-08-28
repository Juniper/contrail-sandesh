#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# util
#

import datetime

def UTCTimestampUsec():
    epoch = datetime.datetime.utcfromtimestamp(0)
    now = datetime.datetime.utcnow()
    delta = now-epoch
    return (delta.microseconds + (delta.seconds + delta.days * 24 * 3600) * 10**6)
#end UTCTimestampUsec

def UTCTimestampUsecToString(utc_usec):
    return datetime.datetime.fromtimestamp(utc_usec/1000000).strftime('%Y-%m-%d %H:%M:%S')
#end UTCTimestampUsecToString
