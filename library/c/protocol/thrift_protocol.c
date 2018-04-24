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
        return thrift_protocol_read_uuid_t (protocol, &uuid, error);
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

int32_t
thrift_binary_protocol_skip_from_buffer (uint8_t *buf, const uint32_t buflen,
                                         ThriftType type, int *error)
{
  switch (type)
  {
    case T_BOOL:
    case T_BYTE:
      {
        if (1 > buflen)
        {
          *error = THRIFT_TRANSPORT_ERROR_RECEIVE;
          os_log (OS_LOG_ERR, "Unable to read %d bytes from buffer of length %d",
            1, buflen);
          return -1;
        }
        return 1;
      }
    case T_I16:
    case T_U16:
      {
        if (2 > buflen)
        {
          *error = THRIFT_TRANSPORT_ERROR_RECEIVE;
          os_log (OS_LOG_ERR, "Unable to read %d bytes from buffer of length %d",
            2, buflen);
          return -1;
        }
        return 2;
      }
    case T_I32:
    case T_U32:
    case T_IPV4:
      {
        if (4 > buflen)
        {
          *error = THRIFT_TRANSPORT_ERROR_RECEIVE;
          os_log (OS_LOG_ERR, "Unable to read %d bytes from buffer of length %d",
            4, buflen);
          return -1;
        }
        return 4;
      }
    case T_I64:
    case T_U64:
    case T_DOUBLE:
      {
        if (8 > buflen)
        {
          *error = THRIFT_TRANSPORT_ERROR_RECEIVE;
          os_log (OS_LOG_ERR, "Unable to read %d bytes from buffer of length %d",
            8, buflen);
          return -1;
        }
        return 8;
      }
    case T_STRING:
    case T_XML:
      {
        uint32_t result = 0;
        union {
          void * b[4];
          int32_t all;
        } bytes;
        int32_t read_len;

        /* read the length into nread_len */
        if (4 > buflen)
        {
          *error = THRIFT_TRANSPORT_ERROR_RECEIVE;
          os_log (OS_LOG_ERR, "Unable to read %d bytes from buffer of length %d",
            4, buflen);
          return -1;
        }
        memcpy (bytes.b, buf, 4);
        result += 4;
        read_len = ntohl (bytes.all);
        if (read_len > 0)
        {
          if (result + read_len > buflen)
          {
            *error = THRIFT_TRANSPORT_ERROR_RECEIVE;
            os_log (OS_LOG_ERR, "Unable to read %d bytes from buffer of length %d",
              result + read_len, buflen);
            return -1;
          }
          result += read_len;
        }
        return result;
      }
    case T_UUID:
      {
        if (16 > buflen)
        {
          *error = THRIFT_TRANSPORT_ERROR_RECEIVE;
          os_log (OS_LOG_ERR, "Unable to read %d bytes from buffer of length %d",
            16, buflen);
          return -1;
        }
        return 16;
      }
    case T_STRUCT:
      {
        uint32_t result = 0;
        void * b[1];
        ThriftType ftype;

        while (1)
        {
          if (1 > buflen - result)
          {
            *error = THRIFT_TRANSPORT_ERROR_RECEIVE;
            os_log (OS_LOG_ERR, "Unable to read %d bytes from buffer of length %d",
              1, buflen - result);
            return -1;
          }
          memcpy (b, buf + result, 1);
          result += 1;
          ftype = *(int8_t *) b;
          if (ftype == T_STOP)
          {
            break;
          }
          if (2 > buflen - result)
          {
            *error = THRIFT_TRANSPORT_ERROR_RECEIVE;
            os_log (OS_LOG_ERR, "Unable to read %d bytes from buffer of length %d",
              2, buflen - result);
            return -1;
          }
          result += 2;
          result += thrift_binary_protocol_skip_from_buffer (buf + result,
              buflen - result, ftype, error);
        }
        return result;
      }
    case T_LIST:
      {
        uint32_t result = 0;
        ThriftType elem_type;
        int32_t i, size;
        void * b[1];
        union {
          void * b[4];
          int32_t all;
        } bytes;

        if (1 > buflen)
        {
            *error = THRIFT_TRANSPORT_ERROR_RECEIVE;
            os_log (OS_LOG_ERR, "Unable to read %d bytes from buffer of length %d",
              1, buflen);
            return -1;
        }
        memcpy (b, buf, 1);
        result += 1;
        elem_type = * (ThriftType *) b;
        if (result + 4 > buflen)
        {
            *error = THRIFT_TRANSPORT_ERROR_RECEIVE;
            os_log (OS_LOG_ERR, "Unable to read %d bytes from buffer of length %d",
              result + 4, buflen);
            return -1;
        }
        memcpy (bytes.b, buf + result, 4);
        result += 4;
        size = ntohl (bytes.all);
        if (size < 0)
        {
            *error = THRIFT_PROTOCOL_ERROR_NEGATIVE_SIZE;
            os_log (OS_LOG_ERR, "Got negative size of %d", size);
            return -1;
        }
        for (i = 0; i < size; i++)
        {
          result += thrift_binary_protocol_skip_from_buffer (buf + result,
              buflen - result, elem_type, error);
        }
        return result;
      }
    default:
      return 0;
  }
}
