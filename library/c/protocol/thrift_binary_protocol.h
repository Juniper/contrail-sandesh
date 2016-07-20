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

#ifndef _THRIFT_BINARY_PROTOCOL_H
#define _THRIFT_BINARY_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

/* version numbers */
#define THRIFT_BINARY_PROTOCOL_VERSION_1 0x80010000
#define THRIFT_BINARY_PROTOCOL_VERSION_MASK 0xffff0000

typedef ThriftProtocol ThriftBinaryProtocol;

void thrift_binary_protocol_init (ThriftBinaryProtocol *protocol);
int32_t thrift_binary_protocol_skip_from_buffer (uint8_t *buf, const uint32_t buflen,
                                              ThriftType type, int *error);

/* Write to ThriftTransport */
static inline int32_t
thrift_binary_protocol_write_sandesh_end (ThriftProtocol *protocol, int *error)
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

static inline int32_t
thrift_binary_protocol_write_struct_begin (ThriftProtocol *protocol,
                                           const char *name,
                                           int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (name);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

static inline int32_t
thrift_binary_protocol_write_struct_end (ThriftProtocol *protocol ,
                                         int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

static inline int32_t
thrift_binary_protocol_write_field_end (ThriftProtocol *protocol ,
                                        int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

static inline int32_t
thrift_binary_protocol_write_list_end (ThriftProtocol *protocol ,
                                       int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

/* Read from ThriftTransport */
static inline int32_t
thrift_binary_protocol_read_sandesh_end (ThriftProtocol *protocol ,
                                         int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

static inline int32_t
thrift_binary_protocol_read_struct_end (ThriftProtocol *protocol ,
                                        int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _THRIFT_BINARY_PROTOCOL_H */
