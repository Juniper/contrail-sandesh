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

#ifndef _THRIFT_TRANSPORT_H
#define _THRIFT_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enumerated definitions of supported transports
 */
typedef enum {
  T_TRANSPORT_MEMORY_BUFFER = 1,
  T_TRANSPORT_FAKE = 2,
} ThriftTransportType;

/*!
 * Thrift Transport class
 */
struct _ThriftTransport
{
  ThriftTransportType ttype;
  /* vtable */
  int32_t (*read) (struct _ThriftTransport *transport, void *buf,
                   u_int32_t len, int *error);
  u_int8_t (*write) (struct _ThriftTransport *transport, const void *buf,
                     const u_int32_t len, int *error);
};
typedef struct _ThriftTransport ThriftTransport;

/* used by THRIFT_TYPE_TRANSPORT */
ThriftTransportType thrift_transport_get_type (void);

/* virtual public methods */

/*!
 * Read some data into the buffer buf.
 * \public \memberof ThriftTransportInterface
 */
static inline int32_t
thrift_transport_read (ThriftTransport *transport, void *buf,
                       u_int32_t len, int *error)
{
  return transport->read (transport, buf, len, error);
}

/*!
 * Writes data from a buffer to the transport.
 * \public \memberof ThriftTransportInterface
 */
static inline u_int8_t
thrift_transport_write (ThriftTransport *transport, const void *buf,
                        const u_int32_t len, int *error)
{
  return transport->write (transport, buf, len, error);
}

/* define error/exception types */
typedef enum
{
  THRIFT_TRANSPORT_ERROR_UNKNOWN,
  THRIFT_TRANSPORT_ERROR_HOST,
  THRIFT_TRANSPORT_ERROR_SOCKET,
  THRIFT_TRANSPORT_ERROR_CONNECT,
  THRIFT_TRANSPORT_ERROR_SEND,
  THRIFT_TRANSPORT_ERROR_RECEIVE,
  THRIFT_TRANSPORT_ERROR_CLOSE
} ThriftTransportError;

#ifdef __cplusplus
}
#endif

#endif /* _THRIFT_TRANSPORT_H */
