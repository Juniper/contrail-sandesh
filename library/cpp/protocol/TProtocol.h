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

#ifndef _SANDESH_PROTOCOL_TPROTOCOL_H_
#define _SANDESH_PROTOCOL_TPROTOCOL_H_ 1

#include <map>
#include <sandesh/transport/TTransport.h>

#include <boost/shared_ptr.hpp>
#include <boost/static_assert.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/asio/ip/address.hpp>

namespace contrail { namespace sandesh { namespace protocol {

using contrail::sandesh::transport::TTransport;

// Use this to get around strict aliasing rules.
// For example, uint64_t i = bitwise_cast<uint64_t>(returns_double());
// The most obvious implementation is to just cast a pointer,
// but that doesn't work.
// For a pretty in-depth explanation of the problem, see
// http://www.cellperformance.com/mike_acton/2006/06/ (...)
// understanding_strict_aliasing.html
template <typename To, typename From>
static inline To bitwise_cast(From from) {
  BOOST_STATIC_ASSERT(sizeof(From) == sizeof(To));

  // BAD!!!  These are all broken with -O2.
  //return *reinterpret_cast<To*>(&from);  // BAD!!!
  //return *static_cast<To*>(static_cast<void*>(&from));  // BAD!!!
  //return *(To*)(void*)&from;  // BAD!!!

  // Super clean and paritally blessed by section 3.9 of the standard.
  //unsigned char c[sizeof(from)];
  //memcpy(c, &from, sizeof(from));
  //To to;
  //memcpy(&to, c, sizeof(c));
  //return to;

  // Slightly more questionable.
  // Same code emitted by GCC.
  //To to;
  //memcpy(&to, &from, sizeof(from));
  //return to;

  // Technically undefined, but almost universally supported,
  // and the most efficient implementation.
  union {
    From f;
    To t;
  } u;
  u.f = from;
  return u.t;
}

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifndef __BYTE_ORDER
# if defined(BYTE_ORDER) && defined(LITTLE_ENDIAN) && defined(BIG_ENDIAN)
#  define __BYTE_ORDER BYTE_ORDER
#  define __LITTLE_ENDIAN LITTLE_ENDIAN
#  define __BIG_ENDIAN BIG_ENDIAN
# else
#  include <boost/config.hpp>
#  include <boost/detail/endian.hpp>
#  define __BYTE_ORDER BOOST_BYTE_ORDER
#  ifdef BOOST_LITTLE_ENDIAN
#   define __LITTLE_ENDIAN __BYTE_ORDER
#   define __BIG_ENDIAN 0
#  else
#   define __LITTLE_ENDIAN 0
#   define __BIG_ENDIAN __BYTE_ORDER
#  endif
# endif
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
#  define ntohll(n) (n)
#  define htonll(n) (n)
# if defined(__GNUC__) && defined(__GLIBC__)
#  include <byteswap.h>
#  define htolell(n) bswap_64(n)
#  define letohll(n) bswap_64(n)
# else /* GNUC & GLIBC */
#  define bswap_64(n) \
      ( (((n) & 0xff00000000000000ull) >> 56) \
      | (((n) & 0x00ff000000000000ull) >> 40) \
      | (((n) & 0x0000ff0000000000ull) >> 24) \
      | (((n) & 0x000000ff00000000ull) >> 8)  \
      | (((n) & 0x00000000ff000000ull) << 8)  \
      | (((n) & 0x0000000000ff0000ull) << 24) \
      | (((n) & 0x000000000000ff00ull) << 40) \
      | (((n) & 0x00000000000000ffull) << 56) )
#  define htolell(n) bswap_64(n)
#  define letohll(n) bswap_64(n)
# endif /* GNUC & GLIBC */
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#  define htolell(n) (n)
#  define letohll(n) (n)
# if defined(__GNUC__) && defined(__GLIBC__)
#  include <byteswap.h>
#  define ntohll(n) bswap_64(n)
#  define htonll(n) bswap_64(n)
# else /* GNUC & GLIBC */
#  define ntohll(n) ( (((uint64_t)ntohl(n)) << 32) + ntohl(n >> 32) )
#  define htonll(n) ( (((uint64_t)htonl(n)) << 32) + htonl(n >> 32) )
# endif /* GNUC & GLIBC */
#else /* __BYTE_ORDER */
# error "Can't define htonll or ntohll!"
#endif

/**
 * Enumerated definition of the types that the Thrift protocol supports.
 * Take special note of the T_END type which is used specifically to mark
 * the end of a sequence of fields.
 */
enum TType {
  T_STOP       = 0,
  T_VOID       = 1,
  T_BOOL       = 2,
  T_BYTE       = 3,
  T_I08        = 3,
  T_I16        = 6,
  T_I32        = 8,
  T_U64        = 9,
  T_I64        = 10,
  T_DOUBLE     = 4,
  T_STRING     = 11,
  T_UTF7       = 11,
  T_STRUCT     = 12,
  T_MAP        = 13,
  T_SET        = 14,
  T_LIST       = 15,
  T_UTF8       = 16,
  T_UTF16      = 17,
  T_SANDESH    = 18,
  T_U16        = 19,
  T_U32        = 20,
  T_XML        = 21,
  T_IPV4       = 22,
  T_UUID       = 23,
  T_IPADDR     = 24
};

/**
 * Enumerated definition of the message types that the Thrift protocol
 * supports.
 */
enum TMessageType {
  T_CALL       = 1,
  T_REPLY      = 2,
  T_EXCEPTION  = 3,
  T_ONEWAY     = 4
};


/**
 * Helper template for implementing TProtocol::skip().
 *
 * Templatized to avoid having to make virtual function calls.
 */
template <class Protocol_>
int32_t skip(Protocol_& prot, TType type) {
  switch (type) {
  case T_BOOL:
    {
      bool boolv;
      return prot.readBool(boolv);
    }
  case T_BYTE:
    {
      int8_t bytev;
      return prot.readByte(bytev);
    }
  case T_I16:
    {
      int16_t i16;
      return prot.readI16(i16);
    }
  case T_I32:
    {
      int32_t i32;
      return prot.readI32(i32);
    }
  case T_I64:
    {
      int64_t i64;
      return prot.readI64(i64);
    }
  case T_U16:
    {
      uint16_t u16;
      return prot.readU16(u16);
    }
  case T_U32:
    {
      uint32_t u32;
      return prot.readU32(u32);
    }
  case T_U64:
    {
      uint64_t u64;
      return prot.readU64(u64);
    }
  case T_IPV4:
    {
      uint32_t ip4;
      return prot.readIPV4(ip4);
    }
  case T_IPADDR:
    {
      boost::asio::ip::address ipaddress;
      return prot.readIPADDR(ipaddress);
    }
  case T_DOUBLE:
    {
      double dub;
      return prot.readDouble(dub);
    }
  case T_STRING:
    {
      std::string str;
      return prot.readBinary(str);
    }
  case T_XML:
    {
      std::string str;
      return prot.readXML(str);
    }
  case T_UUID:
    {
      boost::uuids::uuid uuid;
      return prot.readUUID(uuid);
    }
  case T_STRUCT:
    {
      int32_t result = 0;
      int32_t ret;
      std::string name;
      int16_t fid;
      TType ftype;
      if ((ret = prot.readStructBegin(name)) < 0) {
        return ret;
      }
      result += ret;
      while (true) {
        if ((ret = prot.readFieldBegin(name, ftype, fid)) < 0) {
          return ret;
        }
        result += ret;
        if (ftype == T_STOP) {
          break;
        }
        if ((ret = skip(prot, ftype)) < 0) {
          return ret;
        }
        result += ret;
        if ((ret = prot.readFieldEnd()) < 0) {
          return ret;
        }
        result += ret;
      }
      if ((ret = prot.readStructEnd()) < 0) {
        return ret;
      }
      result += ret;
      return result;
    }
  case T_SANDESH:
    {
      int32_t result = 0;
      int32_t ret;
      std::string name;
      int16_t fid;
      TType ftype;
      if ((ret = prot.readSandeshBegin(name)) < 0) {
        return ret;
      }
      result += ret;
      while (true) {
        if ((ret = prot.readFieldBegin(name, ftype, fid)) < 0) {
          return ret;
        }
        result += ret;
        if (ftype == T_STOP) {
          break;
        }
        if ((ret = skip(prot, ftype)) < 0) {
          return ret;
        }
        result += ret;
        if ((ret = prot.readFieldEnd()) < 0) {
          return ret;
        }
        result += ret;
      }
      if ((ret = prot.readSandeshEnd()) < 0) {
        return ret;
      }
      result += ret;
      return result;
    }
  case T_MAP:
    {
      int32_t result = 0;
      int32_t ret;
      TType keyType;
      TType valType;
      uint32_t i, size;
      if ((ret = prot.readMapBegin(keyType, valType, size)) < 0) {
        return ret;
      }
      result += ret;
      for (i = 0; i < size; i++) {
        if ((ret = skip(prot, keyType)) < 0) {
          return ret;
        }
        result += ret;
        if ((ret = skip(prot, valType)) < 0) {
          return ret;
        }
        result += ret;
      }
      if ((ret = prot.readMapEnd()) < 0) {
        return ret;
      }
      result += ret;
      return result;
    }
  case T_SET:
    {
      int32_t result = 0;
      int32_t ret;
      TType elemType;
      uint32_t i, size;
      if ((ret = prot.readSetBegin(elemType, size)) < 0) {
        return ret;
      }
      result += ret;
      for (i = 0; i < size; i++) {
        if ((ret = skip(prot, elemType)) < 0) {
          return ret;
        }
        result += ret;
      }
      if ((ret = prot.readSetEnd()) < 0) {
        return ret;
      }
      result += ret;
      return result;
    }
  case T_LIST:
    {
      int32_t result = 0;
      int32_t ret;
      TType elemType;
      uint32_t i, size;
      if ((ret = prot.readListBegin(elemType, size)) < 0) {
        return ret;
      }
      result += ret;
      for (i = 0; i < size; i++) {
        if ((ret = skip(prot, elemType)) < 0) {
          return ret;
        }
        result += ret;
      }
      if ((ret = prot.readListEnd()) < 0) {
        return ret;
      }
      result += ret;
      return result;
    }
  case T_STOP: case T_VOID: case T_UTF8: case T_UTF16:
    break;
  }
  return 0;
}

/**
 * Abstract class for a thrift protocol driver. These are all the methods that
 * a protocol must implement. Essentially, there must be some way of reading
 * and writing all the base types, plus a mechanism for writing out structs
 * with indexed fields.
 *
 * TProtocol objects should not be shared across multiple encoding contexts,
 * as they may need to maintain internal state in some protocols (i.e. XML).
 * Note that is is acceptable for the TProtocol module to do its own internal
 * buffered reads/writes to the underlying TTransport where appropriate (i.e.
 * when parsing an input XML stream, reading should be batched rather than
 * looking ahead character by character for a close tag).
 *
 */
class TProtocol {
 public:
  virtual ~TProtocol() {}

  /**
   * Writing functions.
   */

  virtual int32_t writeMessageBegin_virt(const std::string& name,
                                         const TMessageType messageType,
                                         const int32_t seqid) = 0;

  virtual int32_t writeMessageEnd_virt() = 0;


  virtual int32_t writeStructBegin_virt(const char* name) = 0;

  virtual int32_t writeStructEnd_virt() = 0;

  virtual int32_t writeSandeshBegin_virt(const char* name) = 0;

  virtual int32_t writeSandeshEnd_virt() = 0;

  virtual int32_t writeContainerElementBegin_virt() = 0;

  virtual int32_t writeContainerElementEnd_virt() = 0;

  virtual int32_t writeFieldBegin_virt(const char* name,
                                       const TType fieldType,
                                       const int16_t fieldId,
                                       const std::map<std::string, std::string> *const amap) = 0;

  virtual int32_t writeFieldEnd_virt() = 0;

  virtual int32_t writeFieldStop_virt() = 0;

  virtual int32_t writeMapBegin_virt(const TType keyType,
                                     const TType valType,
                                     const uint32_t size) = 0;

  virtual int32_t writeMapEnd_virt() = 0;

  virtual int32_t writeListBegin_virt(const TType elemType,
                                      const uint32_t size) = 0;

  virtual int32_t writeListEnd_virt() = 0;

  virtual int32_t writeSetBegin_virt(const TType elemType,
                                     const uint32_t size) = 0;

  virtual int32_t writeSetEnd_virt() = 0;

  virtual int32_t writeBool_virt(const bool value) = 0;

  virtual int32_t writeByte_virt(const int8_t byte) = 0;

  virtual int32_t writeI16_virt(const int16_t i16) = 0;

  virtual int32_t writeI32_virt(const int32_t i32) = 0;

  virtual int32_t writeI64_virt(const int64_t i64) = 0;

  virtual int32_t writeU16_virt(const uint16_t u16) = 0;

  virtual int32_t writeU32_virt(const uint32_t u32) = 0;
  
  virtual int32_t writeU64_virt(const uint64_t u64) = 0;

  virtual int32_t writeIPV4_virt(const uint32_t ip4) = 0;

  virtual int32_t writeIPADDR_virt(const boost::asio::ip::address& ipaddress) = 0;

  virtual int32_t writeDouble_virt(const double dub) = 0;

  virtual int32_t writeString_virt(const std::string& str) = 0;

  virtual int32_t writeBinary_virt(const std::string& str) = 0;

  virtual int32_t writeXML_virt(const std::string& str) = 0;

  virtual int32_t writeUUID_virt(const boost::uuids::uuid& uuid) = 0;

  int32_t writeMessageBegin(const std::string& name,
                            const TMessageType messageType,
                            const int32_t seqid) {
    return writeMessageBegin_virt(name, messageType, seqid);
  }

  int32_t writeMessageEnd() {
    return writeMessageEnd_virt();
  }

  int32_t writeStructBegin(const char* name) {
    return writeStructBegin_virt(name);
  }

  int32_t writeStructEnd() {
    return writeStructEnd_virt();
  }

  int32_t writeSandeshBegin(const char* name) {
    return writeSandeshBegin_virt(name);
  }

  int32_t writeSandeshEnd() {
    return writeSandeshEnd_virt();
  }

  int32_t writeContainerElementBegin() {
    return writeContainerElementBegin_virt();
  }

  int32_t writeContainerElementEnd() {
    return writeContainerElementEnd_virt();
  }

  int32_t writeFieldBegin(const char* name,
                          const TType fieldType,
                          const int16_t fieldId,
                          const std::map<std::string, std::string> *const amap = NULL) {
    return writeFieldBegin_virt(name, fieldType, fieldId, amap);
  }

  int32_t writeFieldEnd() {
    return writeFieldEnd_virt();
  }

  int32_t writeFieldStop() {
    return writeFieldStop_virt();
  }

  int32_t writeMapBegin(const TType keyType,
                        const TType valType,
                        const uint32_t size) {
    return writeMapBegin_virt(keyType, valType, size);
  }

  int32_t writeMapEnd() {
    return writeMapEnd_virt();
  }

  int32_t writeListBegin(const TType elemType, const uint32_t size) {
    return writeListBegin_virt(elemType, size);
  }

  int32_t writeListEnd() {
    return writeListEnd_virt();
  }

  int32_t writeSetBegin(const TType elemType, const uint32_t size) {
    return writeSetBegin_virt(elemType, size);
  }

  int32_t writeSetEnd() {
    return writeSetEnd_virt();
  }

  int32_t writeBool(const bool value) {
    return writeBool_virt(value);
  }

  int32_t writeByte(const int8_t byte) {
    return writeByte_virt(byte);
  }

  int32_t writeI16(const int16_t i16) {
    return writeI16_virt(i16);
  }

  int32_t writeI32(const int32_t i32) {
    return writeI32_virt(i32);
  }

  int32_t writeI64(const int64_t i64) {
    return writeI64_virt(i64);
  }

  int32_t writeU16(const uint16_t u16) {
    return writeU16_virt(u16);
  }

  int32_t writeU32(const uint32_t u32) {
    return writeU32_virt(u32);
  }

  int32_t writeU64(const uint64_t u64) {
    return writeU64_virt(u64);
  }

  int32_t writeIPV4(const uint32_t ip4) {
    return writeIPV4_virt(ip4);
  }

  int32_t writeIPADDR(const boost::asio::ip::address& ipaddress) {
    return writeIPADDR_virt(ipaddress);
  }

  int32_t writeDouble(const double dub) {
    return writeDouble_virt(dub);
  }

  int32_t writeString(const std::string& str) {
    return writeString_virt(str);
  }

  int32_t writeBinary(const std::string& str) {
    return writeBinary_virt(str);
  }

  int32_t writeXML(const std::string& str) {
    return writeXML_virt(str);
  }

  int32_t writeUUID(const boost::uuids::uuid& uuid) {
    return writeUUID_virt(uuid);
  }

  /**
   * Reading functions
   */

  virtual int32_t readMessageBegin_virt(std::string& name,
                                        TMessageType& messageType,
                                        int32_t& seqid) = 0;

  virtual int32_t readMessageEnd_virt() = 0;

  virtual int32_t readStructBegin_virt(std::string& name) = 0;

  virtual int32_t readStructEnd_virt() = 0;

  virtual int32_t readSandeshBegin_virt(std::string& name) = 0;

  virtual int32_t readSandeshEnd_virt() = 0;

  virtual int32_t readContainerElementBegin_virt() = 0;

  virtual int32_t readContainerElementEnd_virt() = 0;

  virtual int32_t readFieldBegin_virt(std::string& name,
                                      TType& fieldType,
                                      int16_t& fieldId) = 0;

  virtual int32_t readFieldEnd_virt() = 0;

  virtual int32_t readMapBegin_virt(TType& keyType,
                                    TType& valType,
                                    uint32_t& size) = 0;

  virtual int32_t readMapEnd_virt() = 0;

  virtual int32_t readListBegin_virt(TType& elemType,
                                     uint32_t& size) = 0;

  virtual int32_t readListEnd_virt() = 0;

  virtual int32_t readSetBegin_virt(TType& elemType,
                                    uint32_t& size) = 0;

  virtual int32_t readSetEnd_virt() = 0;

  virtual int32_t readBool_virt(bool& value) = 0;

  virtual int32_t readBool_virt(std::vector<bool>::reference value) = 0;

  virtual int32_t readByte_virt(int8_t& byte) = 0;

  virtual int32_t readI16_virt(int16_t& i16) = 0;

  virtual int32_t readI32_virt(int32_t& i32) = 0;

  virtual int32_t readI64_virt(int64_t& i64) = 0;

  virtual int32_t readU16_virt(uint16_t& u16) = 0;

  virtual int32_t readU32_virt(uint32_t& u32) = 0;
  
  virtual int32_t readU64_virt(uint64_t& u64) = 0;

  virtual int32_t readIPV4_virt(uint32_t& ip4) = 0;

  virtual int32_t readIPADDR_virt(boost::asio::ip::address& ipaddress) = 0;

  virtual int32_t readDouble_virt(double& dub) = 0;

  virtual int32_t readString_virt(std::string& str) = 0;

  virtual int32_t readBinary_virt(std::string& str) = 0;

  virtual int32_t readXML_virt(std::string& str) = 0;

  virtual int32_t readUUID_virt(boost::uuids::uuid& uuid) = 0;

  int32_t readMessageBegin(std::string& name,
                           TMessageType& messageType,
                           int32_t& seqid) {
    return readMessageBegin_virt(name, messageType, seqid);
  }

  int32_t readMessageEnd() {
    return readMessageEnd_virt();
  }

  int32_t readStructBegin(std::string& name) {
    return readStructBegin_virt(name);
  }

  int32_t readStructEnd() {
    return readStructEnd_virt();
  }

  int32_t readSandeshBegin(std::string& name) {
    return readSandeshBegin_virt(name);
  }

  int32_t readSandeshEnd() {
    return readSandeshEnd_virt();
  }

  int32_t readContainerElementBegin() {
    return readContainerElementBegin_virt();
  }

  int32_t readContainerElementEnd() {
    return readContainerElementEnd_virt();
  }

  int32_t readFieldBegin(std::string& name,
                         TType& fieldType,
                         int16_t& fieldId) {
    return readFieldBegin_virt(name, fieldType, fieldId);
  }

  int32_t readFieldEnd() {
    return readFieldEnd_virt();
  }

  int32_t readMapBegin(TType& keyType, TType& valType, uint32_t& size) {
    return readMapBegin_virt(keyType, valType, size);
  }

  int32_t readMapEnd() {
    return readMapEnd_virt();
  }

  int32_t readListBegin(TType& elemType, uint32_t& size) {
    return readListBegin_virt(elemType, size);
  }

  int32_t readListEnd() {
    return readListEnd_virt();
  }

  int32_t readSetBegin(TType& elemType, uint32_t& size) {
    return readSetBegin_virt(elemType, size);
  }

  int32_t readSetEnd() {
    return readSetEnd_virt();
  }

  int32_t readBool(bool& value) {
    return readBool_virt(value);
  }

  int32_t readByte(int8_t& byte) {
    return readByte_virt(byte);
  }

  int32_t readI16(int16_t& i16) {
    return readI16_virt(i16);
  }

  int32_t readI32(int32_t& i32) {
    return readI32_virt(i32);
  }

  int32_t readI64(int64_t& i64) {
    return readI64_virt(i64);
  }

  int32_t readU16(uint16_t& u16) {
    return readU16_virt(u16);
  }

  int32_t readU32(uint32_t& u32) {
    return readU32_virt(u32);
  }

  int32_t readU64(uint64_t& u64) {
    return readU64_virt(u64);
  }

  int32_t readIPV4(uint32_t& ip4) {
    return readIPV4_virt(ip4);
  }

  int32_t readIPADDR(boost::asio::ip::address& ipaddress) {
    return readIPADDR_virt(ipaddress);
  }

  int32_t readDouble(double& dub) {
    return readDouble_virt(dub);
  }

  int32_t readString(std::string& str) {
    return readString_virt(str);
  }

  int32_t readBinary(std::string& str) {
    return readBinary_virt(str);
  }

  int32_t readXML(std::string& str) {
    return readXML_virt(str);
  }

  int32_t readUUID(boost::uuids::uuid& uuid) {
    return readUUID_virt(uuid);
  }

  /*
   * std::vector is specialized for bool, and its elements are individual bits
   * rather than bools.   We need to define a different version of readBool()
   * to work with std::vector<bool>.
   */
  int32_t readBool(std::vector<bool>::reference value) {
     return readBool_virt(value);
  }

  /**
   * Method to arbitrarily skip over data.
   */
  int32_t skip(TType type) {
    return skip_virt(type);
  }
  virtual int32_t skip_virt(TType type) {
    return ::contrail::sandesh::protocol::skip(*this, type);
  }

  inline boost::shared_ptr<TTransport> getTransport() {
    return ptrans_;
  }

  protected:
  TProtocol(boost::shared_ptr<TTransport> ptrans):
    ptrans_(ptrans) {
  }

  boost::shared_ptr<TTransport> ptrans_;

 private:
  TProtocol() {}
};

/**
 * Constructs input and output protocol objects given transports.
 */
class TProtocolFactory {
public:
 TProtocolFactory() {}

 virtual ~TProtocolFactory() {}

 virtual boost::shared_ptr<TProtocol> getProtocol(boost::shared_ptr<TTransport> trans) = 0;
};

}}} // contrail::sandesh::protocol

#endif // #define _SANDESH_PROTOCOL_TPROTOCOL_H_ 1
