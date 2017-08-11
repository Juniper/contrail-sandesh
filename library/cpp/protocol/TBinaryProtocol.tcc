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

#ifndef _SANDESH_PROTOCOL_TBINARYPROTOCOL_TCC_
#define _SANDESH_PROTOCOL_TBINARYPROTOCOL_TCC_ 1

#include "TBinaryProtocol.h"

#include <limits>


namespace contrail { namespace sandesh { namespace protocol {

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeMessageBegin(const std::string& name,
                                                        const TMessageType messageType,
                                                        const int32_t seqid) {
  if (this->strict_write_) {
    int32_t version = (VERSION_1) | ((int32_t)messageType);
    int32_t wsize = 0;
    int32_t ret;
    if ((ret = writeI32(version)) < 0) {
      return ret;
    }
    wsize += ret;
    if ((ret = writeString(name)) < 0) {
      return ret;
    }
    wsize += ret;
    if ((ret = writeI32(seqid)) < 0) {
      return ret;
    }
    wsize += ret;  
    return wsize;
  } else {
    int32_t wsize = 0;
    int32_t ret;
    if ((ret = writeString(name)) < 0) {
      return ret;
    }
    wsize += ret;
    if ((ret = writeByte((int8_t)messageType)) < 0) {
      return ret;
    }
    wsize += ret;
    if ((ret = writeI32(seqid)) < 0) {
      return ret;
    }
    wsize += ret;
    return wsize;
  }
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeMessageEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeStructBegin(const char* name) {
  (void) name;
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeStructEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeSandeshBegin(const char* name) {
  return writeString(name);
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeSandeshEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeContainerElementBegin() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeContainerElementEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeFieldBegin(const char* name,
                                                      const TType fieldType,
                                                      const int16_t fieldId,
                                                      const std::map<std::string, std::string> *const amap) {
  (void) name;
  (void) amap;
  int32_t wsize = 0;
  int32_t ret;
  if ((ret = writeByte((int8_t)fieldType)) < 0) {
    return ret;
  }
  wsize += ret;
  if ((ret = writeI16(fieldId)) < 0) {
    return ret;
  }
  wsize += ret;
  return wsize;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeFieldEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeFieldStop() {
  return
    writeByte((int8_t)T_STOP);
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeMapBegin(const TType keyType,
                                                    const TType valType,
                                                    const uint32_t size) {
  int32_t wsize = 0;
  int32_t ret;
  if ((ret = writeByte((int8_t)keyType)) < 0) {
    return ret;
  }
  wsize += ret;
  if ((ret = writeByte((int8_t)valType)) < 0) {
    return ret;
  }
  wsize += ret;
  if ((ret = writeI32((int32_t)size)) < 0) {
    return ret;
  }
  wsize += ret;
  return wsize;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeMapEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeListBegin(const TType elemType,
                                                     const uint32_t size) {
  int32_t wsize = 0;
  int32_t ret;
  if ((ret = writeByte((int8_t) elemType)) < 0) {
    return ret;
  }
  wsize += ret;
  if ((ret = writeI32((int32_t)size)) < 0) {
    return ret;
  }
  wsize += ret;
  return wsize;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeListEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeSetBegin(const TType elemType,
                                                    const uint32_t size) {
  uint32_t wsize = 0;
  int32_t ret;
  if ((ret = writeByte((int8_t)elemType)) < 0) {
    return ret;
  }
  wsize += ret;
  if ((ret = writeI32((int32_t)size)) < 0) {
    return ret;
  }
  wsize += ret;
  return wsize;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeSetEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeBool(const bool value) {
  uint8_t tmp =  value ? 1 : 0;
  this->trans_->write(&tmp, 1);
  return 1;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeByte(const int8_t byte) {
  this->trans_->write((uint8_t*)&byte, 1);
  return 1;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeI16(const int16_t i16) {
  int16_t net = (int16_t)htons(i16);
  this->trans_->write((uint8_t*)&net, 2);
  return 2;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeI32(const int32_t i32) {
  int32_t net = (int32_t)htonl(i32);
  this->trans_->write((uint8_t*)&net, 4);
  return 4;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeI64(const int64_t i64) {
  int64_t net = (int64_t)htonll(i64);
  this->trans_->write((uint8_t*)&net, 8);
  return 8;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeU16(const uint16_t u16) {
  uint16_t net = (uint16_t)htons(u16);
  this->trans_->write((uint8_t*)&net, 2);
  return 2;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeU32(const uint32_t u32) {
  uint32_t net = (uint32_t)htonl(u32);
  this->trans_->write((uint8_t*)&net, 4);
  return 4;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeU64(const uint64_t u64) {
  uint64_t net = (uint64_t)htonll(u64);
  this->trans_->write((uint8_t*)&net, 8);
  return 8;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeIPV4(const uint32_t ip4) {
  uint32_t net = (uint32_t)htonl(ip4);
  this->trans_->write((uint8_t*)&net, 4);
  return 4;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeIPADDR(
                            const boost::asio::ip::address& ipaddress) {
  int32_t ret;
  if (ipaddress.is_v4()) {
    // encode the ip version
    if ((ret = writeByte(AF_INET)) < 0) {
      return ret;
    }
    this->trans_->write(ipaddress.to_v4().to_bytes().data(), 4);
    return ret+4;
  } else if (ipaddress.is_v6()) {
    // encode the ip version
    if ((ret = writeByte(AF_INET6)) < 0) {
      return ret;
    }
    this->trans_->write(ipaddress.to_v6().to_bytes().data(), 16);
    return ret+16;
  }
  return -1;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeDouble(const double dub) {
  BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
  BOOST_STATIC_ASSERT(std::numeric_limits<double>::is_iec559);

  uint64_t bits = bitwise_cast<uint64_t>(dub);
  bits = htonll(bits);
  this->trans_->write((uint8_t*)&bits, 8);
  return 8;
}


template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeString(const std::string& str) {
  uint32_t size = str.size();
  int32_t result;
  if ((result = writeI32((int32_t)size)) < 0) {
    return result;
  }
  if (size > 0) {
    this->trans_->write((uint8_t*)str.data(), size);
  }
  return result + size;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeBinary(const std::string& str) {
  return TBinaryProtocolT<Transport_>::writeString(str);
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeXML(const std::string& str) {
  return TBinaryProtocolT<Transport_>::writeString(str);
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::writeUUID(const boost::uuids::uuid& uuid) {
  this->trans_->write((uint8_t*)&uuid, 16);
  return 16;
}

/**
 * Reading functions
 */

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readMessageBegin(std::string& name,
                                                       TMessageType& messageType,
                                                       int32_t& seqid) {
  int32_t result = 0;
  int32_t sz;
  int ret;
  if ((ret = readI32(sz)) < 0) {
    return ret;
  }
  result += ret;

  if (sz < 0) {
    // Check for correct version number
    int32_t version = sz & VERSION_MASK;
    if (version != VERSION_1) {
      LOG(ERROR, __func__ << ": Bad version identifier");
      return -1;
    }
    messageType = (TMessageType)(sz & 0x000000ff);
    if ((ret = readString(name)) < 0) {
      return ret;
    }
    result += ret;
    if ((ret = readI32(seqid)) < 0) {
      return ret;
    }
    result += ret;
  } else {
    if (this->strict_read_) {
      LOG(ERROR, __func__ << ": No version identifier... old protocol client in strict mode?");
      return -1;
    } else {
      // Handle pre-versioned input
      int8_t type;
      if ((ret = readStringBody(name, sz)) < 0) {
        return ret;
      }
      result += ret;
      if ((ret = readByte(type)) < 0) {
        return ret;
      }
      result += ret;
      messageType = (TMessageType)type;
      if ((ret = readI32(seqid)) < 0) {
        return ret;
      }
      result += ret;
    }
  }
  return result;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readMessageEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readSandeshBegin(std::string& name) {
  return readString(name);
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readSandeshEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readContainerElementBegin() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readContainerElementEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readStructBegin(std::string& name) {
  name = "";
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readStructEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readFieldBegin(std::string& name,
                                                     TType& fieldType,
                                                     int16_t& fieldId) {
  (void) name;
  int32_t result = 0;
  int8_t type;
  int32_t ret;
  if ((ret = readByte(type)) < 0) {
    return ret;
  }
  result += ret;
  fieldType = (TType)type;
  if (fieldType == T_STOP) {
    fieldId = 0;
    return result;
  }
  if ((ret = readI16(fieldId)) < 0) {
    return ret;
  }
  result += ret;
  return result;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readFieldEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readMapBegin(TType& keyType,
                                                   TType& valType,
                                                   uint32_t& size) {
  int8_t k, v;
  int32_t result = 0;
  int32_t sizei;
  int32_t ret;
  if ((ret = readByte(k)) < 0) {
    return ret;
  }
  result += ret;
  keyType = (TType)k;
  if ((ret = readByte(v)) < 0) {
    return ret;
  }
  result += ret;
  valType = (TType)v;
  if ((ret = readI32(sizei)) < 0) {
    return ret;
  }
  result += ret;
  if (sizei < 0) {
    LOG(ERROR, __func__ << ": Negative size " << sizei);
    return -1;
  } else if (this->container_limit_ && sizei > this->container_limit_) {
    LOG(ERROR, __func__ << ": Size " << sizei << " greater than limit " 
        << this->container_limit_);
    return -1;    
  }
  size = (uint32_t)sizei;
  return result;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readMapEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readListBegin(TType& elemType,
                                                    uint32_t& size) {
  int8_t e;
  int32_t result = 0;
  int32_t sizei;
  int32_t ret;
  if ((ret = readByte(e)) < 0) {
    return ret;
  }
  result += ret;
  elemType = (TType)e;
  if ((ret = readI32(sizei)) < 0) {
    return ret;
  }
  result += ret;
  if (sizei < 0) {
    LOG(ERROR, __func__ << ": Negative size " << sizei);
    return -1;
  } else if (this->container_limit_ && sizei > this->container_limit_) {
    LOG(ERROR, __func__ << ": Size " << sizei << " greater than limit " 
        << this->container_limit_);
    return -1;  
  }
  size = (uint32_t)sizei;
  return result;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readListEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readSetBegin(TType& elemType,
                                                   uint32_t& size) {
  int8_t e;
  int32_t result = 0;
  int32_t sizei;
  int32_t ret;
  if ((ret = readByte(e)) < 0) {
    return ret;
  }
  result += ret;
  elemType = (TType)e;
  if ((ret = readI32(sizei)) < 0) {
    return ret;
  }
  result += ret;
  if (sizei < 0) {
    LOG(ERROR, __func__ << ": Negative size " << sizei);  
  } else if (this->container_limit_ && sizei > this->container_limit_) {
    LOG(ERROR, __func__ << ": Size " << sizei << " greater than limit " 
        << this->container_limit_);
    return -1;  
  }
  size = (uint32_t)sizei;
  return result;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readSetEnd() {
  return 0;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readBool(bool& value) {
  uint8_t b[1];
  this->trans_->readAll(b, 1);
  value = *(int8_t*)b != 0;
  return 1;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readByte(int8_t& byte) {
  uint8_t b[1];
  this->trans_->readAll(b, 1);
  byte = *(int8_t*)b;
  return 1;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readI16(int16_t& i16) {
  union bytes {
    uint8_t b[2];
    int16_t all;
  } theBytes;
  this->trans_->readAll(theBytes.b, 2);
  i16 = (int16_t)ntohs(theBytes.all);
  return 2;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readI32(int32_t& i32) {
  union bytes {
    uint8_t b[4];
    int32_t all;
  } theBytes;
  this->trans_->readAll(theBytes.b, 4);
  i32 = (int32_t)ntohl(theBytes.all);
  return 4;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readI64(int64_t& i64) {
  union bytes {
    uint8_t b[8];
    int64_t all;
  } theBytes;
  this->trans_->readAll(theBytes.b, 8);
  i64 = (int64_t)ntohll(theBytes.all);
  return 8;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readU16(uint16_t& u16) {
  union bytes {
    uint8_t b[2];
    uint16_t all;
  } theBytes;
  this->trans_->readAll(theBytes.b, 2);
  u16 = (uint16_t)ntohs(theBytes.all);
  return 2;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readU32(uint32_t& u32) {
  union bytes {
    uint8_t b[4];
    uint32_t all;
  } theBytes;
  this->trans_->readAll(theBytes.b, 4);
  u32 = (uint32_t)ntohl(theBytes.all);
  return 4;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readU64(uint64_t& u64) {
  union bytes {
    uint8_t b[8];
    int64_t all;
  } theBytes;
  this->trans_->readAll(theBytes.b, 8);
  u64 = (uint64_t)ntohll(theBytes.all);
  return 8;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readIPV4(uint32_t& ip4) {
  union bytes {
    uint8_t b[4];
    uint32_t all;
  } theBytes;
  this->trans_->readAll(theBytes.b, 4);
  ip4 = (uint32_t)ntohl(theBytes.all);
  return 4;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readIPADDR(
                        boost::asio::ip::address& ipaddress) {
  int32_t ret;
  int8_t version;
  // decode the ip version
  if ((ret = readByte(version)) < 0) {
    return ret;
  }
  if (version == AF_INET) {
    boost::asio::ip::address_v4::bytes_type ipv4;
#ifdef BOOST_ASIO_HAS_STD_ARRAY
    this->trans_->readAll(ipv4.data(), 4);
#else
    this->trans_->readAll(ipv4.c_array(), 4);
#endif
    ipaddress = boost::asio::ip::address_v4(ipv4);
    return ret+4;
  } else if (version == AF_INET6) {
    boost::asio::ip::address_v6::bytes_type ipv6;
#ifdef BOOST_ASIO_HAS_STD_ARRAY
    this->trans_->readAll(ipv6.data(), 16);
#else
    this->trans_->readAll(ipv6.c_array(), 16);
#endif
    ipaddress = boost::asio::ip::address_v6(ipv6);
    return ret+16;
  }
  return -1;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readDouble(double& dub) {
  BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
  BOOST_STATIC_ASSERT(std::numeric_limits<double>::is_iec559);

  union bytes {
    uint8_t b[8];
    uint64_t all;
  } theBytes;
  this->trans_->readAll(theBytes.b, 8);
  theBytes.all = ntohll(theBytes.all);
  dub = bitwise_cast<double>(theBytes.all);
  return 8;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readString(std::string& str) {
  int32_t result = 0;
  int32_t size;
  int32_t ret;
  if ((ret = readI32(size)) < 0) {
    return ret;
  }
  result += ret;
  if ((ret = readStringBody(str, size)) < 0) {
    return ret;
  }
  result += ret;
  return result;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readBinary(std::string& str) {
  return TBinaryProtocolT<Transport_>::readString(str);
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readXML(std::string& str) {
  return TBinaryProtocolT<Transport_>::readString(str);
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readUUID(boost::uuids::uuid& uuid) {
  this->trans_->readAll(uuid.data, 16);
  return 16;
}

template <class Transport_>
int32_t TBinaryProtocolT<Transport_>::readStringBody(std::string& str,
                                                     int32_t size) {
  int32_t result = 0;

  // Catch error cases
  if (size < 0) {
    LOG(ERROR, __func__ << ": Negative size " << size);
    return -1;
  }
  if (this->string_limit_ > 0 && size > this->string_limit_) {
      LOG(ERROR, __func__ << ": Size " << size << " greater than limit " 
          << this->string_limit_);
      return -1;  
  }

  // Catch empty string case
  if (size == 0) {
    str = "";
    return result;
  }

  // Try to borrow first
  const uint8_t* borrow_buf;
  uint32_t got = size;
  if ((borrow_buf = this->trans_->borrow(NULL, &got))) {
    str.assign((const char*)borrow_buf, size);
    this->trans_->consume(size);
    return size;
  }

  // Use the heap here to prevent stack overflow for v. large strings
  if (size > this->string_buf_size_ || this->string_buf_ == NULL) {
    void* new_string_buf = std::realloc(this->string_buf_, (uint32_t)size);
    if (new_string_buf == NULL) {
      LOG(ERROR, __func__ << ": Realloc size " << (uint32_t)size << " FAILED");
      return -1;
    }
    this->string_buf_ = (uint8_t*)new_string_buf;
    this->string_buf_size_ = size;
  }
  this->trans_->readAll(this->string_buf_, size);
  str = std::string((char*)this->string_buf_, size);
  return size;
}

}}} // contrail::sandesh::protocol

#endif // #ifndef _SANDESH_PROTOCOL_TBINARYPROTOCOL_TCC_
