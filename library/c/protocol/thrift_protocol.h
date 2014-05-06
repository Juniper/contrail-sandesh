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

#ifndef _THRIFT_PROTOCOL_H
#define _THRIFT_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

/*! \file thrift_protocol.h
 *  \brief Abstract class for Thrift protocol implementations.
 */

/**
 * Enumerated definition of the types that the Thrift protocol supports.
 * Take special note of the T_END type which is used specifically to mark
 * the end of a sequence of fields.
 */
typedef enum {
  T_STOP   = 0,
  T_VOID   = 1,
  T_BOOL   = 2,
  T_BYTE   = 3,
  T_I08    = 3,
  T_I16    = 6,
  T_I32    = 8,
  T_U64    = 9,
  T_I64    = 10,
  T_DOUBLE = 4,
  T_STRING = 11,
  T_UTF7   = 11,
  T_STRUCT = 12,
  T_MAP    = 13,
  T_SET    = 14,
  T_LIST   = 15,
  T_UTF8   = 16,
  T_UTF16  = 17,
  T_U16    = 19,
  T_U32    = 20,
  T_XML    = 21,
  T_IPV4   = 22,
} ThriftType;

/**
 * Enumerated definition of the message types that the Thrift protocol
 * supports.
 */
typedef enum {
  T_CALL      = 1,
  T_REPLY     = 2,
  T_EXCEPTION = 3,
  T_ONEWAY    = 4
} ThriftMessageType;

/**
 * Enumerated definitions of supported protocols
 */
typedef enum {
  T_PROTOCOL_BINARY = 1,
} ThriftProtocolType;

/*
 * Thrift Protocol
 */
struct _ThriftProtocol
{
  ThriftProtocolType ptype;
  ThriftTransport *transport;
  int32_t (*write_sandesh_begin) (struct _ThriftProtocol *protocol, const char *name,
                                  int *error);
  int32_t (*write_sandesh_end) (struct _ThriftProtocol *protocol, int *error);
  int32_t (*write_message_begin) (struct _ThriftProtocol *protocol, const char *name,
                                  const ThriftMessageType message_type,
                                  const int32_t seqid, int *error);
  int32_t (*write_message_end) (struct _ThriftProtocol *protocol, int *error);
  int32_t (*write_struct_begin) (struct _ThriftProtocol *protocol, const char *name,
                                 int *error);
  int32_t (*write_struct_end) (struct _ThriftProtocol *protocol, int *error);
  int32_t (*write_field_begin) (struct _ThriftProtocol *protocol, const char *name,
                               const ThriftType field_type,
                               const int16_t field_id, int *error);
  int32_t (*write_field_end) (struct _ThriftProtocol *protocol, int *error);
  int32_t (*write_field_stop) (struct _ThriftProtocol *protocol, int *error);
  int32_t (*write_map_begin) (struct _ThriftProtocol *protocol,
                              const ThriftType key_type,
                              const ThriftType value_type,
                              const u_int32_t size, int *error);
  int32_t (*write_map_end) (struct _ThriftProtocol *protocol, int *error);
  int32_t (*write_list_begin) (struct _ThriftProtocol *protocol,
                               const ThriftType element_type,
                               const u_int32_t size, int *error);
  int32_t (*write_list_end) (struct _ThriftProtocol *protocol, int *error);
  int32_t (*write_set_begin) (struct _ThriftProtocol *protocol,
                              const ThriftType element_type,
                              const u_int32_t size, int *error);
  int32_t (*write_set_end) (struct _ThriftProtocol *protocol, int *error);
  int32_t (*write_bool) (struct _ThriftProtocol *protocol, const u_int8_t value,
                         int *error);
  int32_t (*write_byte) (struct _ThriftProtocol *protocol, const int8_t value,
                         int *error);
  int32_t (*write_i16) (struct _ThriftProtocol *protocol, const int16_t value,
                        int *error);
  int32_t (*write_i32) (struct _ThriftProtocol *protocol, const int32_t value,
                        int *error);
  int32_t (*write_i64) (struct _ThriftProtocol *protocol, const int64_t value,
                        int *error);
  int32_t (*write_u16) (struct _ThriftProtocol *protocol, const u_int16_t value,
                        int *error);
  int32_t (*write_u32) (struct _ThriftProtocol *protocol, const u_int32_t value,
                        int *error);
  int32_t (*write_u64) (struct _ThriftProtocol *protocol, const u_int64_t value,
                        int *error);
  int32_t (*write_ipv4) (struct _ThriftProtocol *protocol, const u_int32_t value,
                        int *error);
  int32_t (*write_double) (struct _ThriftProtocol *protocol, const double value,
                           int *error);
  int32_t (*write_string) (struct _ThriftProtocol *protocol, const char *str,
                           int *error);
  int32_t (*write_binary) (struct _ThriftProtocol *protocol, const void *buf,
                          const u_int32_t len, int *error);
  int32_t (*write_xml) (struct _ThriftProtocol *protocol, const char *str,
                        int *error);

  int32_t (*read_sandesh_begin) (struct _ThriftProtocol *protocol, char **name,
                                 int *error);
  int32_t (*read_sandesh_end) (struct _ThriftProtocol *protocol, int *error);
  int32_t (*read_message_begin) (struct _ThriftProtocol *protocol, char **name,
                                 ThriftMessageType *message_type,
                                 int32_t *seqid, int *error);
  int32_t (*read_message_end) (struct _ThriftProtocol *protocol, int *error);
  int32_t (*read_struct_begin) (struct _ThriftProtocol *protocol, char **name,
                                int *error);
  int32_t (*read_struct_end) (struct _ThriftProtocol *protocol, int *error);
  int32_t (*read_field_begin) (struct _ThriftProtocol *protocol, char **name,
                               ThriftType *field_type, int16_t *field_id,
                               int *error);
  int32_t (*read_field_end) (struct _ThriftProtocol *protocol, int *error);
  int32_t (*read_map_begin) (struct _ThriftProtocol *protocol, ThriftType *key_type,
                             ThriftType *value_type, u_int32_t *size,
                             int *error);
  int32_t (*read_map_end) (struct _ThriftProtocol *protocol, int *error);
  int32_t (*read_list_begin) (struct _ThriftProtocol *protocol, ThriftType *element_type,
                              u_int32_t *size, int *error);
  int32_t (*read_list_end) (struct _ThriftProtocol *protocol, int *error);
  int32_t (*read_set_begin) (struct _ThriftProtocol *protocol, ThriftType *element_type,
                             u_int32_t *size, int *error);
  int32_t (*read_set_end) (struct _ThriftProtocol *protocol, int *error);
  int32_t (*read_bool) (struct _ThriftProtocol *protocol, u_int8_t *value,
                        int *error);
  int32_t (*read_byte) (struct _ThriftProtocol *protocol, int8_t *value, int *error);
  int32_t (*read_i16) (struct _ThriftProtocol *protocol, int16_t *value, int *error);
  int32_t (*read_i32) (struct _ThriftProtocol *protocol, int32_t *value, int *error);
  int32_t (*read_i64) (struct _ThriftProtocol *protocol, int64_t *value, int *error);
  int32_t (*read_u16) (struct _ThriftProtocol *protocol, u_int16_t *value, int *error);
  int32_t (*read_u32) (struct _ThriftProtocol *protocol, u_int32_t *value, int *error);
  int32_t (*read_u64) (struct _ThriftProtocol *protocol, u_int64_t *value, int *error);
  int32_t (*read_ipv4) (struct _ThriftProtocol *protocol, u_int32_t *value, int *error);
  int32_t (*read_double) (struct _ThriftProtocol *protocol, double *value,
                          int *error);
  int32_t (*read_string) (struct _ThriftProtocol *protocol, char **str, int *error);
  int32_t (*read_binary) (struct _ThriftProtocol *protocol, void **buf,
                          u_int32_t *len, int *error);
  int32_t (*read_xml) (struct _ThriftProtocol *protocol, char **str, int *error);
};
typedef struct _ThriftProtocol ThriftProtocol;

/* used by THRIFT_TYPE_PROTOCOL */
ThriftProtocolType thrift_protocol_get_type (ThriftProtocol *protocol);

/* virtual public methods */
int32_t thrift_protocol_write_message_begin (ThriftProtocol *protocol,
           const char *name, const ThriftMessageType message_type,
           const int32_t seqid, int *error);

int32_t thrift_protocol_write_message_end (ThriftProtocol *protocol,
                                           int *error);

int32_t thrift_protocol_write_sandesh_begin (ThriftProtocol *protocol,
           const char *name, int *error);

int32_t thrift_protocol_write_sandesh_end (ThriftProtocol *protocol,
                                           int *error);

int32_t thrift_protocol_write_message_end (ThriftProtocol *protocol,
                                           int *error);

int32_t thrift_protocol_write_struct_begin (ThriftProtocol *protocol,
                                            const char *name,
                                            int *error);

int32_t thrift_protocol_write_struct_end (ThriftProtocol *protocol,
                                          int *error);

int32_t thrift_protocol_write_field_begin (ThriftProtocol *protocol,
                                           const char *name,
                                           const ThriftType field_type,
                                           const int16_t field_id,
                                           int *error);

int32_t thrift_protocol_write_field_end (ThriftProtocol *protocol,
                                         int *error);

int32_t thrift_protocol_write_field_stop (ThriftProtocol *protocol,
                                          int *error);

int32_t thrift_protocol_write_map_begin (ThriftProtocol *protocol,
                                         const ThriftType key_type,
                                         const ThriftType value_type,
                                         const u_int32_t size, int *error);

int32_t thrift_protocol_write_map_end (ThriftProtocol *protocol, int *error);

int32_t thrift_protocol_write_list_begin (ThriftProtocol *protocol,
                                          const ThriftType element_type,
                                          const u_int32_t size, int *error);

int32_t thrift_protocol_write_list_end (ThriftProtocol *protocol,
                                        int *error);

int32_t thrift_protocol_write_set_begin (ThriftProtocol *protocol,
                                         const ThriftType element_type,
                                         const u_int32_t size, int *error);

int32_t thrift_protocol_write_set_end (ThriftProtocol *protocol,
                                       int *error);

int32_t thrift_protocol_write_bool (ThriftProtocol *protocol,
                                    const u_int8_t value, int *error);

int32_t thrift_protocol_write_byte (ThriftProtocol *protocol, const int8_t value,
                                    int *error);

int32_t thrift_protocol_write_i16 (ThriftProtocol *protocol, const int16_t value,
                                   int *error);

int32_t thrift_protocol_write_i32 (ThriftProtocol *protocol, const int32_t value,
                                   int *error);

int32_t thrift_protocol_write_i64 (ThriftProtocol *protocol, const int64_t value,
                                   int *error);

int32_t thrift_protocol_write_u16 (ThriftProtocol *protocol, const u_int16_t value,
                                   int *error);

int32_t thrift_protocol_write_u32 (ThriftProtocol *protocol, const u_int32_t value,
                                   int *error);

int32_t thrift_protocol_write_u64 (ThriftProtocol *protocol, const u_int64_t value,
                                   int *error);

int32_t thrift_protocol_write_ipv4 (ThriftProtocol *protocol, const u_int32_t value,
                                   int *error);

int32_t thrift_protocol_write_double (ThriftProtocol *protocol,
                                      const double value, int *error);

int32_t thrift_protocol_write_string (ThriftProtocol *protocol,
                                      const char *str, int *error);

int32_t thrift_protocol_write_binary (ThriftProtocol *protocol,
                                      const void *buf,
                                      const u_int32_t len, int *error);

int32_t thrift_protocol_write_xml (ThriftProtocol *protocol,
                                   const char *str, int *error);

int32_t thrift_protocol_read_message_begin (ThriftProtocol *thrift_protocol,
                                            char **name,
                                            ThriftMessageType *message_type,
                                            int32_t *seqid, int *error);

int32_t thrift_protocol_read_message_end (ThriftProtocol *protocol,
                                          int *error);

int32_t thrift_protocol_read_sandesh_begin (ThriftProtocol *thrift_protocol,
                                            char **name,
                                            int *error);

int32_t thrift_protocol_read_sandesh_end (ThriftProtocol *protocol,
                                          int *error);

int32_t thrift_protocol_read_struct_begin (ThriftProtocol *protocol,
                                           char **name,
                                           int *error);

int32_t thrift_protocol_read_struct_end (ThriftProtocol *protocol,
                                         int *error);

int32_t thrift_protocol_read_field_begin (ThriftProtocol *protocol,
                                          char **name,
                                          ThriftType *field_type,
                                          int16_t *field_id,
                                          int *error);

int32_t thrift_protocol_read_field_end (ThriftProtocol *protocol,
                                        int *error);

int32_t thrift_protocol_read_map_begin (ThriftProtocol *protocol,
                                        ThriftType *key_type,
                                        ThriftType *value_type, u_int32_t *size,
                                        int *error);

int32_t thrift_protocol_read_map_end (ThriftProtocol *protocol, int *error);

int32_t thrift_protocol_read_list_begin (ThriftProtocol *protocol,
                                         ThriftType *element_type,
                                         u_int32_t *size, int *error);

int32_t thrift_protocol_read_list_end (ThriftProtocol *protocol, int *error);

int32_t thrift_protocol_read_set_begin (ThriftProtocol *protocol,
                                        ThriftType *element_type,
                                        u_int32_t *size, int *error);

int32_t thrift_protocol_read_set_end (ThriftProtocol *protocol, int *error);

int32_t thrift_protocol_read_bool (ThriftProtocol *protocol, u_int8_t *value,
                                   int *error);

int32_t thrift_protocol_read_byte (ThriftProtocol *protocol, int8_t *value,
                                   int *error);

int32_t thrift_protocol_read_i16 (ThriftProtocol *protocol, int16_t *value,
                                  int *error);

int32_t thrift_protocol_read_i32 (ThriftProtocol *protocol, int32_t *value,
                                  int *error);

int32_t thrift_protocol_read_i64 (ThriftProtocol *protocol, int64_t *value,
                                  int *error);

int32_t thrift_protocol_read_u16 (ThriftProtocol *protocol, u_int16_t *value,
                                  int *error);

int32_t thrift_protocol_read_u32 (ThriftProtocol *protocol, u_int32_t *value,
                                  int *error);

int32_t thrift_protocol_read_u64 (ThriftProtocol *protocol, u_int64_t *value,
                                  int *error);

int32_t thrift_protocol_read_ipv4 (ThriftProtocol *protocol, u_int32_t *value,
                                  int *error);

int32_t thrift_protocol_read_double (ThriftProtocol *protocol,
                                     double *value, int *error);

int32_t thrift_protocol_read_string (ThriftProtocol *protocol,
                                     char **str, int *error);

int32_t thrift_protocol_read_binary (ThriftProtocol *protocol,
                                     void **buf, u_int32_t *len,
                                     int *error);

int32_t thrift_protocol_read_xml (ThriftProtocol *protocol,
                                  char **str, int *error);

int32_t thrift_protocol_skip (ThriftProtocol *protocol, ThriftType type,
                              int *error);

void thrift_protocol_init (ThriftProtocol *protocol,
                           ThriftProtocolType ptype,
                           ThriftTransport *transport);

/* define error types */
typedef enum
{
  THRIFT_PROTOCOL_ERROR_UNKNOWN,
  THRIFT_PROTOCOL_ERROR_INVALID_DATA,
  THRIFT_PROTOCOL_ERROR_NEGATIVE_SIZE,
  THRIFT_PROTOCOL_ERROR_SIZE_LIMIT,
  THRIFT_PROTOCOL_ERROR_BAD_VERSION,
  THRIFT_PROTOCOL_ERROR_NOT_IMPLEMENTED
} ThriftProtocolError;

#ifdef __cplusplus
}
#endif

#endif /* _THRIFT_PROTOCOL_H */
