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

void
thrift_protocol_init (ThriftProtocol *protocol,
                      ThriftProtocolType ptype,
                      ThriftTransport *transport)
{
  protocol->ptype = ptype;
  protocol->transport = transport;
  switch (ptype) {
  case T_PROTOCOL_BINARY:
    // Initialize the protocol methods
    thrift_binary_protocol_init(protocol);
    break;
  default:
    break;
  }
}

ThriftProtocolType
thrift_protocol_get_type (ThriftProtocol *protocol)
{
  return protocol->ptype;
}

int32_t
thrift_protocol_write_message_begin (ThriftProtocol *protocol, 
                                     const char *name,
                                     const ThriftMessageType message_type,
                                     const int32_t seqid, int *error)
{
  return protocol->write_message_begin (protocol, name, message_type, seqid,
                                        error);
}

int32_t
thrift_protocol_write_message_end (ThriftProtocol *protocol, int *error)
{
  return protocol->write_message_end (protocol, error);
}

int32_t
thrift_protocol_write_sandesh_begin (ThriftProtocol *protocol,
                                     const char *name,
                                     int *error)
{
  return protocol->write_sandesh_begin (protocol, name, error);
}

int32_t
thrift_protocol_write_sandesh_end (ThriftProtocol *protocol, int *error)
{
  return protocol->write_sandesh_end (protocol, error);
}

int32_t
thrift_protocol_write_struct_begin (ThriftProtocol *protocol, const char *name,
                                    int *error)
{
  return protocol->write_struct_begin (protocol, name, error);
}

int32_t
thrift_protocol_write_struct_end (ThriftProtocol *protocol, int *error)
{
  return protocol->write_struct_end (protocol, error);
}

int32_t
thrift_protocol_write_field_begin (ThriftProtocol *protocol,
                                   const char *name,
                                   const ThriftType field_type,
                                   const int16_t field_id,
                                   int *error)
{
  return protocol->write_field_begin (protocol, name, field_type,
                                      field_id, error);
}

int32_t
thrift_protocol_write_field_end (ThriftProtocol *protocol, int *error)
{
  return protocol->write_field_end (protocol, error);
}

int32_t
thrift_protocol_write_field_stop (ThriftProtocol *protocol, int *error)
{
  return protocol->write_field_stop (protocol, error);
}

int32_t
thrift_protocol_write_map_begin (ThriftProtocol *protocol,
                                 const ThriftType key_type,
                                 const ThriftType value_type,
                                 const u_int32_t size, int *error)
{
  return protocol->write_map_begin (protocol, key_type, value_type,
                                    size, error);
}

int32_t
thrift_protocol_write_map_end (ThriftProtocol *protocol, int *error)
{
  return protocol->write_map_end (protocol, error);
}

int32_t
thrift_protocol_write_list_begin (ThriftProtocol *protocol,
                                  const ThriftType element_type,
                                  const u_int32_t size, int *error)
{
  return protocol->write_list_begin (protocol, element_type, size, error);
}

int32_t
thrift_protocol_write_list_end (ThriftProtocol *protocol, int *error)
{
  return protocol->write_list_end (protocol, error);
}

int32_t
thrift_protocol_write_set_begin (ThriftProtocol *protocol,
                                 const ThriftType element_type,
                                 const u_int32_t size, int *error)
{
  return protocol->write_set_begin (protocol, element_type, size, error);
}

int32_t
thrift_protocol_write_set_end (ThriftProtocol *protocol, int *error)
{
  return protocol->write_set_end (protocol, error);
}

int32_t
thrift_protocol_write_bool (ThriftProtocol *protocol,
                            const u_int8_t value, int *error)
{
  return protocol->write_bool (protocol, value, error);
}

int32_t
thrift_protocol_write_byte (ThriftProtocol *protocol, const int8_t value,
                            int *error)
{
  return protocol->write_byte (protocol, value, error);
}

int32_t
thrift_protocol_write_i16 (ThriftProtocol *protocol, const int16_t value,
                           int *error)
{
  return protocol->write_i16 (protocol, value, error);
}

int32_t
thrift_protocol_write_i32 (ThriftProtocol *protocol, const int32_t value,
                           int *error)
{
  return protocol->write_i32 (protocol, value, error);
}

int32_t
thrift_protocol_write_i64 (ThriftProtocol *protocol, const int64_t value,
                           int *error)
{
  return protocol->write_i64 (protocol, value, error);
}

int32_t
thrift_protocol_write_u16 (ThriftProtocol *protocol, const u_int16_t value,
                           int *error)
{
  return protocol->write_u16 (protocol, value, error);
}

int32_t
thrift_protocol_write_u32 (ThriftProtocol *protocol, const u_int32_t value,
                           int *error)
{
  return protocol->write_u32 (protocol, value, error);
}

int32_t
thrift_protocol_write_u64 (ThriftProtocol *protocol, const u_int64_t value,
                           int *error)
{
  return protocol->write_u64 (protocol, value, error);
}

int32_t
thrift_protocol_write_ipv4 (ThriftProtocol *protocol, const u_int32_t value,
                           int *error)
{
  return protocol->write_ipv4 (protocol, value, error);
}

int32_t
thrift_protocol_write_double (ThriftProtocol *protocol,
                              const double value, int *error)
{
  return protocol->write_double (protocol, value, error);
}

int32_t
thrift_protocol_write_string (ThriftProtocol *protocol,
                              const char *str, int *error)
{
  return protocol->write_string (protocol, str, error);
}

int32_t
thrift_protocol_write_binary (ThriftProtocol *protocol, const void *buf,
                              const u_int32_t len, int *error)
{
  return protocol->write_binary (protocol, buf, len, error);
}

int32_t
thrift_protocol_write_xml (ThriftProtocol *protocol,
                           const char *str, int *error)
{
  return protocol->write_xml (protocol, str, error);
}

int32_t
thrift_protocol_write_uuid (ThriftProtocol *protocol,
                           const uuid_t value, int *error)
{
  return protocol->write_uuid (protocol, value, error);
}

int32_t
thrift_protocol_read_message_begin (ThriftProtocol *protocol,
                                    char **name,
                                    ThriftMessageType *message_type,
                                    int32_t *seqid, int *error)
{
  return protocol->read_message_begin (protocol, name, message_type,
                                       seqid, error);
}

int32_t
thrift_protocol_read_message_end (ThriftProtocol *protocol,
                                  int *error)
{
  return protocol->read_message_end (protocol, error);
}

int32_t
thrift_protocol_read_sandesh_begin (ThriftProtocol *protocol,
                                    char **name,
                                    int *error)
{
  return protocol->read_sandesh_begin (protocol, name, error);
}

int32_t
thrift_protocol_read_sandesh_end (ThriftProtocol *protocol,
                                  int *error)
{
  return protocol->read_sandesh_end (protocol, error);
}

int32_t
thrift_protocol_read_struct_begin (ThriftProtocol *protocol,
                                   char **name,
                                   int *error)
{
  return protocol->read_struct_begin (protocol, name, error);
}

int32_t
thrift_protocol_read_struct_end (ThriftProtocol *protocol, int *error)
{
  return protocol->read_struct_end (protocol, error);
}

int32_t
thrift_protocol_read_field_begin (ThriftProtocol *protocol,
                                  char **name,
                                  ThriftType *field_type,
                                  int16_t *field_id,
                                  int *error)
{
  return protocol->read_field_begin (protocol, name, field_type, field_id,
                                     error);
}

int32_t
thrift_protocol_read_field_end (ThriftProtocol *protocol,
                                int *error)
{
  return protocol->read_field_end (protocol, error);
}

int32_t
thrift_protocol_read_map_begin (ThriftProtocol *protocol,
                                ThriftType *key_type,
                                ThriftType *value_type, u_int32_t *size,
                                int *error)
{
  return protocol->read_map_begin (protocol, key_type, value_type, size,
                                   error);
}

int32_t
thrift_protocol_read_map_end (ThriftProtocol *protocol, int *error)
{
  return protocol->read_map_end (protocol, error);
}

int32_t
thrift_protocol_read_list_begin (ThriftProtocol *protocol,
                                 ThriftType *element_type,
                                 u_int32_t *size, int *error)
{
  return protocol->read_list_begin (protocol, element_type, size, error);
}

int32_t
thrift_protocol_read_list_end (ThriftProtocol *protocol, int *error)
{
  return protocol->read_list_end (protocol, error);
}

int32_t
thrift_protocol_read_set_begin (ThriftProtocol *protocol,
                                ThriftType *element_type,
                                u_int32_t *size, int *error)
{
  return protocol->read_set_begin (protocol, element_type, size, error);
}

int32_t
thrift_protocol_read_set_end (ThriftProtocol *protocol, int *error)
{
  return protocol->read_set_end (protocol, error);
}

int32_t
thrift_protocol_read_bool (ThriftProtocol *protocol, u_int8_t *value,
                           int *error)
{
  return protocol->read_bool (protocol, value, error);
}

int32_t
thrift_protocol_read_byte (ThriftProtocol *protocol, int8_t *value,
                           int *error)
{
  return protocol->read_byte (protocol, value, error);
}

int32_t
thrift_protocol_read_i16 (ThriftProtocol *protocol, int16_t *value,
                          int *error)
{
  return protocol->read_i16 (protocol, value, error);
}

int32_t
thrift_protocol_read_i32 (ThriftProtocol *protocol, int32_t *value,
                          int *error)
{
  return protocol->read_i32 (protocol, value, error);
}

int32_t
thrift_protocol_read_i64 (ThriftProtocol *protocol, int64_t *value,
                          int *error)
{
  return protocol->read_i64 (protocol, value, error);
}

int32_t 
thrift_protocol_read_u16 (ThriftProtocol *protocol, u_int16_t *value,
                          int *error)
{
  return protocol->read_u16 (protocol, value, error);
}

int32_t
thrift_protocol_read_u32 (ThriftProtocol *protocol, u_int32_t *value,
                          int *error)
{
  return protocol->read_u32 (protocol, value, error);
}

int32_t
thrift_protocol_read_u64 (ThriftProtocol *protocol, u_int64_t *value,
                          int *error)
{
  return protocol->read_u64 (protocol, value, error);
}

int32_t
thrift_protocol_read_ipv4 (ThriftProtocol *protocol, u_int32_t *value,
                          int *error)
{
  return protocol->read_ipv4 (protocol, value, error);
}

int32_t
thrift_protocol_read_double (ThriftProtocol *protocol,
                             double *value, int *error)
{
  return protocol->read_double (protocol, value, error);
}

int32_t
thrift_protocol_read_string (ThriftProtocol *protocol,
                             char **str, int *error)
{
  return protocol->read_string (protocol, str, error);
}

int32_t
thrift_protocol_read_binary (ThriftProtocol *protocol, void **buf,
                             u_int32_t *len, int *error)
{
  return protocol->read_binary (protocol, buf, len, error);
}

int32_t
thrift_protocol_read_xml (ThriftProtocol *protocol,
                          char **str, int *error)
{
  return protocol->read_xml (protocol, str, error);
}

int32_t
thrift_protocol_read_uuid (ThriftProtocol *protocol,
                          uuid_t *value, int *error)
{
  return protocol->read_uuid (protocol, value, error);
}

int32_t
thrift_protocol_skip (ThriftProtocol *protocol, ThriftType type, int *error)
{
  switch (type)
  {
    case T_BOOL:
      {
        u_int8_t boolv;
        return thrift_protocol_read_bool (protocol, &boolv, error);
      }
    case T_BYTE:
      {
        int8_t bytev;
        return thrift_protocol_read_byte (protocol, &bytev, error);
      }

    case T_I16:
      {
        int16_t i16;
        return thrift_protocol_read_i16 (protocol, &i16, error);
      }
    case T_I32:
      {
        int32_t i32;
        return thrift_protocol_read_i32 (protocol, &i32, error);
      }
    case T_I64:
      {
        int64_t i64;
        return thrift_protocol_read_i64 (protocol, &i64, error);
      }
    case T_U16:
      {
        u_int16_t u16;
        return thrift_protocol_read_u16 (protocol, &u16, error);
      }
    case T_U32:
      {
        u_int32_t u32;
        return thrift_protocol_read_u32 (protocol, &u32, error);
      }
    case T_U64:
      {
        u_int64_t u64;
        return thrift_protocol_read_u64 (protocol, &u64, error);
      }
    case T_IPV4:
      {
        u_int32_t ip4;
        return thrift_protocol_read_ipv4 (protocol, &ip4, error);
      }
    case T_DOUBLE:
      {
        double dub;
        return thrift_protocol_read_double (protocol, &dub, error);
      }
    case T_STRING:
    case T_XML:
      {
        void *data;
        u_int32_t len;
        u_int32_t ret = thrift_protocol_read_binary (protocol, &data, &len, error);
        os_free (data);
        return ret;
      }
    case T_UUID:
      {
        uuid_t uuid;
        return thrift_protocol_read_uuid (protocol, &uuid, error);
      }
    case T_STRUCT:
      {
        u_int32_t result = 0;
        char *name;
        int16_t fid;
        ThriftType ftype;
        result += thrift_protocol_read_struct_begin (protocol, &name, error);

        while (1)
        {
          result += thrift_protocol_read_field_begin (protocol, &name, &ftype,
                                                      &fid, error);
          if (ftype == T_STOP)
          {
            break;
          }
          result += thrift_protocol_skip (protocol, ftype, error);
          result += thrift_protocol_read_field_end (protocol, error);
        }
        result += thrift_protocol_read_struct_end (protocol, error);
        return result;
      }
    case T_MAP:
      {
        u_int32_t result = 0;
        ThriftType elem_type;
        u_int32_t i, size;
        result += thrift_protocol_read_set_begin (protocol, &elem_type, &size,
                                                  error);
        for (i = 0; i < size; i++)
        {
          result += thrift_protocol_skip (protocol, elem_type, error);
        }
        result += thrift_protocol_read_set_end (protocol, error);
        return result;
      }
    case T_LIST:
      {
        u_int32_t result = 0;
        ThriftType elem_type;
        u_int32_t i, size;
        result += thrift_protocol_read_list_begin (protocol, &elem_type, &size,
                                                   error);
        for (i = 0; i < size; i++)
        {
          result += thrift_protocol_skip (protocol, elem_type, error);
        }
        result += thrift_protocol_read_list_end (protocol, error);
        return result;
      }
    default:
      return 0;
  }
}
