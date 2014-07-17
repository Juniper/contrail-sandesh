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

#ifndef __KERNEL__
static u_int64_t
thrift_bitwise_cast_u_int64 (double v)
{
  union {
    double from;
    u_int64_t to;
  } u;
  u.from = v;
  return u.to;
}

static double
thrift_bitwise_cast_double (u_int64_t v)
{
  union {
    u_int64_t from;
    double to;
  } u;
  u.from = v;
  return u.to;
}
#endif

int32_t
thrift_binary_protocol_write_message_begin (ThriftProtocol *protocol,
    const char *name, const ThriftMessageType message_type,
    const int32_t seqid, int *error)
{
  int32_t version = (THRIFT_BINARY_PROTOCOL_VERSION_1)
                   | ((int32_t) message_type);
  int32_t ret;
  int32_t xfer = 0;

  if ((ret = thrift_protocol_write_i32 (protocol, version, error)) < 0)
  {
    return -1;
  }
  xfer += ret;
  if ((ret = thrift_protocol_write_string (protocol, name, error)) < 0)
  {
    return -1;
  }
  xfer += ret;
  if ((ret = thrift_protocol_write_i32 (protocol, seqid, error)) < 0)
  {
    return -1;
  }
  xfer += ret;
  return xfer;
}

int32_t
thrift_binary_protocol_write_message_end (ThriftProtocol *protocol ,
                                          int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

int32_t
thrift_binary_protocol_write_sandesh_begin (ThriftProtocol *protocol,
    const char *name, int *error)
{
  int32_t ret;
  int32_t xfer = 0;

  if ((ret = thrift_protocol_write_string (protocol, name, error)) < 0)
  {
    return -1;
  }
  xfer += ret;
  return xfer;
}

int32_t
thrift_binary_protocol_write_sandesh_end (ThriftProtocol *protocol ,
                                          int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

int32_t
thrift_binary_protocol_write_struct_begin (ThriftProtocol *protocol ,
                                           const char *name ,
                                           int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (name);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

int32_t
thrift_binary_protocol_write_struct_end (ThriftProtocol *protocol ,
                                         int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

int32_t
thrift_binary_protocol_write_field_begin (ThriftProtocol *protocol,
                                          const char *name,
                                          const ThriftType field_type,
                                          const int16_t field_id,
                                          int *error)
{
  int32_t ret;
  int32_t xfer = 0;

  if ((ret = thrift_protocol_write_byte (protocol, (int8_t) field_type,
                                         error)) < 0)
  {
    return -1;
  }
  xfer += ret;
  if ((ret = thrift_protocol_write_i16 (protocol, field_id, error)) < 0)
  {
    return -1;
  }
  xfer += ret;
  return xfer;
}

int32_t
thrift_binary_protocol_write_field_end (ThriftProtocol *protocol ,
                                        int *error )
{
  return 0;
}

int32_t
thrift_binary_protocol_write_field_stop (ThriftProtocol *protocol,
                                         int *error)
{

  return thrift_protocol_write_byte (protocol, (int8_t) T_STOP, error);
}

int32_t
thrift_binary_protocol_write_map_begin (ThriftProtocol *protocol,
                                        const ThriftType key_type,
                                        const ThriftType value_type,
                                        const u_int32_t size,
                                        int *error)
{
  int32_t ret;
  int32_t xfer = 0;

  if ((ret = thrift_protocol_write_byte (protocol, (int8_t) key_type,
                                         error)) < 0)
  {
    return -1;
  }
  xfer += ret;
  if ((ret = thrift_protocol_write_byte (protocol, (int8_t) value_type,
                                         error)) < 0)
  {
    return -1;
  }
  xfer += ret;
  if ((ret = thrift_protocol_write_i32 (protocol, (int32_t) size, error)) < 0)
  {
    return -1;
  }
  xfer += ret;
  return xfer;
}

int32_t
thrift_binary_protocol_write_map_end (ThriftProtocol *protocol ,
                                      int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

int32_t
thrift_binary_protocol_write_list_begin (ThriftProtocol *protocol,
                                         const ThriftType element_type,
                                         const u_int32_t size,
                                         int *error)
{
  int32_t ret;
  int32_t xfer = 0;

  if ((ret = thrift_protocol_write_byte (protocol, (int8_t) element_type,
                                         error)) < 0)
  {
    return -1;
  }
  xfer += ret;

  if ((ret = thrift_protocol_write_i32 (protocol, (int32_t) size, error)) < 0)
  {
    return -1;
  }
  xfer += ret;

  return xfer;
}

int32_t
thrift_binary_protocol_write_list_end (ThriftProtocol *protocol ,
                                       int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

int32_t
thrift_binary_protocol_write_set_begin (ThriftProtocol *protocol,
                                        const ThriftType element_type,
                                        const u_int32_t size,
                                        int *error)
{
  return thrift_protocol_write_list_begin (protocol, element_type,
                                           size, error);
}

int32_t
thrift_binary_protocol_write_set_end (ThriftProtocol *protocol ,
                                      int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

int32_t
thrift_binary_protocol_write_bool (ThriftProtocol *protocol,
                                   const u_int8_t value, int *error)
{
  u_int8_t tmp = value ? 1 : 0;
  return thrift_protocol_write_byte (protocol, tmp, error);
}

int32_t
thrift_binary_protocol_write_byte (ThriftProtocol *protocol, const int8_t value,
                                   int *error)
{
  if (thrift_transport_write (protocol->transport,
                              (const void *) &value, 1, error))
  {
    return 1;
  } else {
    return -1;
  }
}

int32_t
thrift_binary_protocol_write_i16 (ThriftProtocol *protocol, const int16_t value,
                                  int *error)
{
  int16_t net = htons (value);
  if (thrift_transport_write (protocol->transport,
                              (const void *) &net, 2, error))
  {
    return 2;
  } else {
    return -1;
  }
}

int32_t
thrift_binary_protocol_write_i32 (ThriftProtocol *protocol, const int32_t value,
                                  int *error)
{
  int32_t net = htonl (value);
  if (thrift_transport_write (protocol->transport,
                              (const void *) &net, 4, error))
  {
    return 4;
  } else {
    return -1;
  }
}

int32_t
thrift_binary_protocol_write_i64 (ThriftProtocol *protocol, const int64_t value,
                                  int *error)
{
  int64_t net;
  os_put_value64((uint8_t *)&net, value);
  if (thrift_transport_write (protocol->transport,
                              (const void *) &net, 8, error))
  {
    return 8;
  } else {
    return -1;
  }
}

int32_t
thrift_binary_protocol_write_u16 (ThriftProtocol *protocol, const u_int16_t value,
                                  int *error)
{
  u_int16_t net = htons (value);
  if (thrift_transport_write (protocol->transport,
                              (const void *) &net, 2, error))
  {
    return 2;
  } else {
    return -1;
  }
}

int32_t
thrift_binary_protocol_write_u32 (ThriftProtocol *protocol, const u_int32_t value,
                                  int *error)
{
  u_int32_t net = htonl (value);
  if (thrift_transport_write (protocol->transport,
                              (const void *) &net, 4, error))
  {
    return 4;
  } else {
    return -1;
  }
}

int32_t
thrift_binary_protocol_write_u64 (ThriftProtocol *protocol, const uint64_t value,
                                  int *error)
{
  int64_t net;
  os_put_value64((uint8_t *)&net, value);
  if (thrift_transport_write (protocol->transport,
                              (const void *) &net, 8, error))
  {
    return 8;
  } else {
    return -1;
  }
}

int32_t
thrift_binary_protocol_write_ipv4 (ThriftProtocol *protocol, const u_int32_t value,
                                  int *error)
{
  return thrift_binary_protocol_write_u32 (protocol, value, error);
}

int32_t
thrift_binary_protocol_write_uuid_t (ThriftProtocol *protocol, const uuid_t value,
                                  int *error)
{
  if (thrift_transport_write (protocol->transport,
                              (const void *) value, 16, error))
  {
    return 16;
  } else {
    return -1;
  }
}

int32_t
thrift_binary_protocol_write_double (ThriftProtocol *protocol,
                                     const double value, int *error)
{
#ifndef __KERNEL__
  // TODO
  u_int64_t bits = thrift_bitwise_cast_u_int64 (value);
  if (thrift_transport_write (protocol->transport,
                              (const void *) &bits, 8, error))
  {
    return 8;
  } else {
    return -1;
  }
#else
  return -1;
#endif
}

int32_t
thrift_binary_protocol_write_string (ThriftProtocol *protocol,
                                     const char *str, int *error)
{
  u_int32_t len = str != NULL ? strlen (str) : 0;
  /* write the string length + 1 which includes the null terminator */
  return thrift_protocol_write_binary (protocol, (const void *) str,
                                       len, error);
}

int32_t
thrift_binary_protocol_write_binary (ThriftProtocol *protocol,
                                     const void * buf,
                                     const u_int32_t len, int *error)
{
  int32_t ret;
  int32_t xfer = 0;

  if ((ret = thrift_protocol_write_i32 (protocol, len, error)) < 0)
  {
    return -1;
  }
  xfer += ret;

  if (len > 0)
  {
    if (thrift_transport_write (protocol->transport,
                                (const void *) buf, len, error) == 0)
    {
      return -1;
    }
    xfer += len;
  }

  return xfer;
}

int32_t
thrift_binary_protocol_read_message_begin (ThriftProtocol *protocol,
                                           char **name,
                                           ThriftMessageType *message_type,
                                           int32_t *seqid, int *error)
{
  int32_t ret;
  int32_t xfer = 0;
  int32_t sz;

  if ((ret = thrift_protocol_read_i32 (protocol, &sz, error)) < 0)
  {
    return -1;
  }
  xfer += ret;

  if (sz < 0)
  {
    /* check for version */
    u_int32_t version = sz & THRIFT_BINARY_PROTOCOL_VERSION_MASK;
    if (version != THRIFT_BINARY_PROTOCOL_VERSION_1)
    {
      *error = THRIFT_PROTOCOL_ERROR_BAD_VERSION;
      os_log(OS_LOG_ERR, "Expected version %d, got %d",
             THRIFT_BINARY_PROTOCOL_VERSION_1, version);
      return -1;
    }

    *message_type = (ThriftMessageType) (sz & 0x000000ff);

    if ((ret = thrift_protocol_read_string (protocol, name, error)) < 0)
    {
      return -1;
    }
    xfer += ret;

    if ((ret = thrift_protocol_read_i32 (protocol, seqid, error)) < 0)
    {
      return -1;
    }
    xfer += ret;
  }
  return xfer;
}

int32_t
thrift_binary_protocol_read_message_end (ThriftProtocol *protocol ,
                                         int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

int32_t
thrift_binary_protocol_read_sandesh_begin (ThriftProtocol *protocol,
                                           char **name,
                                           int *error)
{
  int32_t ret;
  int32_t xfer = 0;

  if ((ret = thrift_protocol_read_string (protocol, name, error)) < 0)
  {
    return -1;
  }
  xfer += ret;
  return xfer;
}

int32_t
thrift_binary_protocol_read_sandesh_end (ThriftProtocol *protocol ,
                                         int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

int32_t
thrift_binary_protocol_read_struct_begin (ThriftProtocol *protocol ,
                                          char **name,
                                          int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  *name = NULL;
  return 0;
}

int32_t
thrift_binary_protocol_read_struct_end (ThriftProtocol *protocol ,
                                        int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

int32_t
thrift_binary_protocol_read_field_begin (ThriftProtocol *protocol,
                                         char **name,
                                         ThriftType *field_type,
                                         int16_t *field_id,
                                         int *error)
{
  int32_t ret;
  int32_t xfer = 0;
  int8_t type;

  if ((ret = thrift_protocol_read_byte (protocol, &type, error)) < 0)
  {
    return -1;
  }
  xfer += ret;
  *field_type = (ThriftType) type;
  if (*field_type == T_STOP)
  {
    *field_id = 0;
    return xfer;
  }
  if ((ret = thrift_protocol_read_i16 (protocol, field_id, error)) < 0)
  {
    return -1;
  }
  xfer += ret;
  return xfer;
}

int32_t
thrift_binary_protocol_read_field_end (ThriftProtocol *protocol ,
                                       int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

int32_t
thrift_binary_protocol_read_map_begin (ThriftProtocol *protocol,
                                       ThriftType *key_type,
                                       ThriftType *value_type,
                                       u_int32_t *size,
                                       int *error)
{
  int32_t ret;
  int32_t xfer = 0;
  int8_t k, v;
  int32_t sizei;

  if ((ret = thrift_protocol_read_byte (protocol, &k, error)) < 0)
  {
    return -1;
  }
  xfer += ret;
  *key_type = (ThriftType) k;

  if ((ret = thrift_protocol_read_byte (protocol, &v, error)) < 0)
  {
    return -1;
  }
  xfer += ret;
  *value_type = (ThriftType) v;

  if ((ret = thrift_protocol_read_i32 (protocol, &sizei, error)) <0)
  {
    return -1;
  }
  xfer += ret;

  if (sizei < 0)
  {
    *error = THRIFT_PROTOCOL_ERROR_NEGATIVE_SIZE;
    os_log(OS_LOG_ERR, "Got negative size of %d", sizei);
    return -1;
  }

  *size = (u_int32_t) sizei;
  return xfer;
}

int32_t
thrift_binary_protocol_read_map_end (ThriftProtocol *protocol ,
                                     int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

int32_t
thrift_binary_protocol_read_list_begin (ThriftProtocol *protocol,
                                        ThriftType *element_type,
                                        u_int32_t *size, int *error)
{
  int32_t ret;
  int32_t xfer = 0;
  int8_t e;
  int32_t sizei;

  if ((ret = thrift_protocol_read_byte (protocol, &e, error)) < 0)
  {
    return -1;
  }
  xfer += ret;
  *element_type = (ThriftType) e;

  if ((ret = thrift_protocol_read_i32 (protocol, &sizei, error)) < 0)
  {
    return -1;
  }
  xfer += ret;

  if (sizei < 0)
  {
    *error = THRIFT_PROTOCOL_ERROR_NEGATIVE_SIZE;
    os_log(OS_LOG_ERR, "Got negative size of %d", sizei);
    return -1;
  }

  *size = (u_int32_t) sizei;
  return xfer;
}

int32_t
thrift_binary_protocol_read_list_end (ThriftProtocol *protocol ,
                                      int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

int32_t
thrift_binary_protocol_read_set_begin (ThriftProtocol *protocol,
                                       ThriftType *element_type,
                                       u_int32_t *size, int *error)
{


  return thrift_protocol_read_list_begin (protocol, element_type, size, error);
}

int32_t
thrift_binary_protocol_read_set_end (ThriftProtocol *protocol ,
                                     int *error )
{
  THRIFT_UNUSED_VAR (protocol);
  THRIFT_UNUSED_VAR (error);
  return 0;
}

int32_t
thrift_binary_protocol_read_bool (ThriftProtocol *protocol, u_int8_t *value,
                                  int *error)
{
  int32_t ret;
  void * b[1];

  if ((ret = 
       thrift_transport_read (protocol->transport,
                              b, 1, error)) < 0)
  {
    return -1;
  }
  *value = *(int8_t *) b != 0;
  return ret;
}

int32_t
thrift_binary_protocol_read_byte (ThriftProtocol *protocol, int8_t *value,
                                  int *error)
{
  int32_t ret;
  void * b[1];

  if ((ret =
       thrift_transport_read (protocol->transport,
                              b, 1, error)) < 0)
  {
    return -1;
  }
  *value = *(int8_t *) b;
  return ret;
}

int32_t
thrift_binary_protocol_read_i16 (ThriftProtocol *protocol, int16_t *value,
                                 int *error)
{
  int32_t ret;
  union bytes {
    void * b[2];
    int16_t all;
  } bytes;

  if ((ret =
       thrift_transport_read (protocol->transport,
                              bytes.b, 2, error)) < 0)
  {
    return -1;
  }
  *value = ntohs (bytes.all);
  return ret;
}

int32_t
thrift_binary_protocol_read_i32 (ThriftProtocol *protocol, int32_t *value,
                                 int *error)
{
  int32_t ret;
  union {
    void * b[4];
    int32_t all;
  } bytes;

  if ((ret =
       thrift_transport_read (protocol->transport,
                              bytes.b, 4, error)) < 0)
  {
    return -1;
  }
  *value = ntohl (bytes.all);
  return ret;
}

int32_t
thrift_binary_protocol_read_i64 (ThriftProtocol *protocol, int64_t *value,
                                 int *error)
{
  int32_t ret;
  void * b[8];

  if ((ret =
       thrift_transport_read (protocol->transport,
                              b, 8, error)) < 0)
  {
    return -1;
  }
  *value = os_get_value64((uint8_t *)b);
  return ret;
}

int32_t
thrift_binary_protocol_read_u16 (ThriftProtocol *protocol, u_int16_t *value,
                                 int *error)
{
  int32_t ret;
  union {
    void * b[2];
    u_int16_t all;
  } bytes;

  if ((ret =
       thrift_transport_read (protocol->transport,
                              bytes.b, 2, error)) < 0)
  {
    return -1;
  }
  *value = ntohs (bytes.all);
  return ret;
}

int32_t
thrift_binary_protocol_read_u32 (ThriftProtocol *protocol, u_int32_t *value,
                                 int *error)
{
  int32_t ret;
  union {
      void * b[4];
      u_int32_t all;
  } bytes;

  if ((ret =
       thrift_transport_read (protocol->transport,
                              bytes.b, 4, error)) < 0)
  {
    return -1;
  }
  *value = ntohl (bytes.all);
  return ret;
}

int32_t
thrift_binary_protocol_read_u64 (ThriftProtocol *protocol, uint64_t *value,
                                 int *error)
{
  int32_t ret;
  void * b[8];

  if ((ret =
       thrift_transport_read (protocol->transport,
                              b, 8, error)) < 0)
  {
    return -1;
  }
  *value = os_get_value64((uint8_t *)b);
  return ret;
}

int32_t
thrift_binary_protocol_read_ipv4 (ThriftProtocol *protocol, u_int32_t *value,
                                 int *error)
{
  return thrift_binary_protocol_read_u32 (protocol, value, error);
}

int32_t
thrift_binary_protocol_read_uuid_t (ThriftProtocol *protocol, uuid_t *value,
                                 int *error)
{
  int32_t ret;

  if ((ret =
       thrift_transport_read (protocol->transport,
                              value, 16, error)) < 0)
  {
    return -1;
  }
  return ret;
}

int32_t
thrift_binary_protocol_read_double (ThriftProtocol *protocol,
                                    double *value, int *error)
{
#ifndef __KERNEL__
  int32_t ret;
  union {
    void * b[8];
    u_int64_t all;
  } bytes;

  if ((ret =
       thrift_transport_read (protocol->transport,
                              bytes.b, 8, error)) < 0)
  {
    return -1;
  }
  *value = thrift_bitwise_cast_double (bytes.all);
  return ret;
#else
  return -1;
#endif
}

int32_t
thrift_binary_protocol_read_string (ThriftProtocol *protocol,
                                    char **str, int *error)
{
  u_int32_t len;
  int32_t ret;
  int32_t xfer = 0;
  int32_t read_len = 0;

  /* read the length into read_len */
  if ((ret =
       thrift_protocol_read_i32 (protocol, &read_len, error)) < 0)
  {
    return -1;
  }
  xfer += ret;

  if (read_len > 0)
  {
    /* allocate the memory for the string */
    len = (u_int32_t) read_len + 1; // space for null terminator
    *str = os_malloc (len);
    memset (*str, 0, len);
    if ((ret =
         thrift_transport_read (protocol->transport,
                                *str, read_len, error)) < 0)
    {
      os_free (*str);
      *str = NULL;
      len = 0;
      return -1;
    }
    xfer += ret;
  } else {
    *str = NULL;
  }

  return xfer;
}

int32_t
thrift_binary_protocol_read_binary (ThriftProtocol *protocol,
                                    void **buf, u_int32_t *len,
                                    int *error)
{
  int32_t ret;
  int32_t xfer = 0;
  int32_t read_len = 0;
 
  /* read the length into read_len */
  if ((ret =
       thrift_protocol_read_i32 (protocol, &read_len, error)) < 0)
  {
    return -1;
  }
  xfer += ret;

  if (read_len > 0)
  {
    /* allocate the memory as an array of unsigned char for binary data */
    *len = (u_int32_t) read_len;
    *buf = os_malloc (*len);
    memset (*buf, 0, *len);
    if ((ret =
         thrift_transport_read (protocol->transport,
                                *buf, *len, error)) < 0)
    {
      os_free (*buf);
      *buf = NULL;
      *len = 0;
      return -1;
    }
    xfer += ret;
  } else {
    *buf = NULL;
  }

  return xfer;
}

void
thrift_binary_protocol_init (ThriftBinaryProtocol *protocol)
{
  protocol->write_message_begin = thrift_binary_protocol_write_message_begin;
  protocol->write_message_end = thrift_binary_protocol_write_message_end;
  protocol->write_sandesh_begin = thrift_binary_protocol_write_sandesh_begin;
  protocol->write_sandesh_end = thrift_binary_protocol_write_sandesh_end;
  protocol->write_struct_begin = thrift_binary_protocol_write_struct_begin;
  protocol->write_struct_end = thrift_binary_protocol_write_struct_end;
  protocol->write_field_begin = thrift_binary_protocol_write_field_begin;
  protocol->write_field_end = thrift_binary_protocol_write_field_end;
  protocol->write_field_stop = thrift_binary_protocol_write_field_stop;
  protocol->write_map_begin = thrift_binary_protocol_write_map_begin;
  protocol->write_map_end = thrift_binary_protocol_write_map_end;
  protocol->write_list_begin = thrift_binary_protocol_write_list_begin;
  protocol->write_list_end = thrift_binary_protocol_write_list_end;
  protocol->write_set_begin = thrift_binary_protocol_write_set_begin;
  protocol->write_set_end = thrift_binary_protocol_write_set_end;
  protocol->write_bool = thrift_binary_protocol_write_bool;
  protocol->write_byte = thrift_binary_protocol_write_byte;
  protocol->write_i16 = thrift_binary_protocol_write_i16;
  protocol->write_i32 = thrift_binary_protocol_write_i32;
  protocol->write_i64 = thrift_binary_protocol_write_i64;
  protocol->write_u16 = thrift_binary_protocol_write_u16;
  protocol->write_u32 = thrift_binary_protocol_write_u32;
  protocol->write_u64 = thrift_binary_protocol_write_u64;
  protocol->write_ipv4 = thrift_binary_protocol_write_ipv4;
  protocol->write_double = thrift_binary_protocol_write_double;
  protocol->write_string = thrift_binary_protocol_write_string;
  protocol->write_binary = thrift_binary_protocol_write_binary;
  protocol->write_xml = thrift_binary_protocol_write_string;
  protocol->write_uuid_t = thrift_binary_protocol_write_uuid_t;
  protocol->read_message_begin = thrift_binary_protocol_read_message_begin;
  protocol->read_message_end = thrift_binary_protocol_read_message_end;
  protocol->read_sandesh_begin = thrift_binary_protocol_read_sandesh_begin;
  protocol->read_sandesh_end = thrift_binary_protocol_read_sandesh_end;
  protocol->read_struct_begin = thrift_binary_protocol_read_struct_begin;
  protocol->read_struct_end = thrift_binary_protocol_read_struct_end;
  protocol->read_field_begin = thrift_binary_protocol_read_field_begin;
  protocol->read_field_end = thrift_binary_protocol_read_field_end;
  protocol->read_map_begin = thrift_binary_protocol_read_map_begin;
  protocol->read_map_end = thrift_binary_protocol_read_map_end;
  protocol->read_list_begin = thrift_binary_protocol_read_list_begin;
  protocol->read_list_end = thrift_binary_protocol_read_list_end;
  protocol->read_set_begin = thrift_binary_protocol_read_set_begin;
  protocol->read_set_end = thrift_binary_protocol_read_set_end;
  protocol->read_bool = thrift_binary_protocol_read_bool;
  protocol->read_byte = thrift_binary_protocol_read_byte;
  protocol->read_i16 = thrift_binary_protocol_read_i16;
  protocol->read_i32 = thrift_binary_protocol_read_i32;
  protocol->read_i64 = thrift_binary_protocol_read_i64;
  protocol->read_u16 = thrift_binary_protocol_read_u16;
  protocol->read_u32 = thrift_binary_protocol_read_u32;
  protocol->read_u64 = thrift_binary_protocol_read_u64;
  protocol->read_ipv4 = thrift_binary_protocol_read_ipv4;
  protocol->read_double = thrift_binary_protocol_read_double;
  protocol->read_string = thrift_binary_protocol_read_string;
  protocol->read_binary = thrift_binary_protocol_read_binary;
  protocol->read_xml = thrift_binary_protocol_read_string;
  protocol->read_uuid_t = thrift_binary_protocol_read_uuid_t;
}
