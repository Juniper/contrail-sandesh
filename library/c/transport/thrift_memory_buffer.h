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

#ifndef _THRIFT_MEMORY_BUFFER_H
#define _THRIFT_MEMORY_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

/*! \file thrift_memory_buffer.h
 *  \brief Implementation of a Thrift memory buffer transport.
 */

/*!
 * ThriftMemoryBuffer instance.
 */
struct _ThriftMemoryBuffer
{
  /* Needs to be the first element */
  ThriftTransport tt;

  /* private */
  u_int8_t *buf;
  u_int32_t buf_roffset;
  u_int32_t buf_woffset;
  u_int32_t buf_size;
  u_int8_t owner;
};
typedef struct _ThriftMemoryBuffer ThriftMemoryBuffer;

void thrift_memory_buffer_init (ThriftMemoryBuffer *transport, void *buf, u_int32_t size);
int thrift_memory_buffer_wrote_bytes (ThriftMemoryBuffer *t, u_int32_t len);

#ifdef __cplusplus
}
#endif

#endif
