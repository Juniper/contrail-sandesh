/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "sandesh.h"

u_int8_t
thrift_transport_is_open (ThriftTransport *transport)
{
  return transport->is_open (transport);
}

u_int8_t
thrift_transport_open (ThriftTransport *transport, int *error)
{
  return transport->open (transport, error);
}

u_int8_t
thrift_transport_close (ThriftTransport *transport, int *error)
{
  return transport->close (transport, error);
}

int32_t
thrift_transport_read (ThriftTransport *transport, void *buf,
                       u_int32_t len, int *error)
{
  return transport->read (transport, buf, len, error);
}

u_int8_t
thrift_transport_read_end (ThriftTransport *transport, int *error)
{
  return transport->read_end (transport, error);
}

u_int8_t
thrift_transport_write (ThriftTransport *transport, const void *buf,
                        const u_int32_t len, int *error)
{
  return transport->write (transport, buf, len, error);
}

u_int8_t
thrift_transport_write_end (ThriftTransport *transport, int *error)
{
  return transport->write_end (transport, error);
}

u_int8_t
thrift_transport_flush (ThriftTransport *transport, int *error)
{
  return transport->flush (transport, error);
}
