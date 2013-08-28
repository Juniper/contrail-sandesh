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

#ifndef _SANDESH_PROTOCOL_TBINARYPROTOCOL_H_
#define _SANDESH_PROTOCOL_TBINARYPROTOCOL_H_ 1

#include "TProtocol.h"
#include "TVirtualProtocol.h"

#include <boost/shared_ptr.hpp>

#include <base/logging.h>

namespace contrail { namespace sandesh { namespace protocol {

/**
 * The default binary protocol for thrift. Writes all data in a very basic
 * binary format, essentially just spitting out the raw bytes.
 *
 */
template <class Transport_>
class TBinaryProtocolT
  : public TVirtualProtocol< TBinaryProtocolT<Transport_> > {
 protected:
  static const int32_t VERSION_MASK = 0xffff0000;
  static const int32_t VERSION_1 = 0x80010000;
  // VERSION_2 (0x80020000)  is taken by TDenseProtocol.

 public:
  TBinaryProtocolT(boost::shared_ptr<Transport_> trans) :
    TVirtualProtocol< TBinaryProtocolT<Transport_> >(trans),
    trans_(trans.get()),
    string_limit_(0),
    container_limit_(0),
    strict_read_(false),
    strict_write_(true),
    string_buf_(NULL),
    string_buf_size_(0) {}

  TBinaryProtocolT(boost::shared_ptr<Transport_> trans,
                   int32_t string_limit,
                   int32_t container_limit,
                   bool strict_read,
                   bool strict_write) :
    TVirtualProtocol< TBinaryProtocolT<Transport_> >(trans),
    trans_(trans.get()),
    string_limit_(string_limit),
    container_limit_(container_limit),
    strict_read_(strict_read),
    strict_write_(strict_write),
    string_buf_(NULL),
    string_buf_size_(0) {}

  ~TBinaryProtocolT() {
    if (string_buf_ != NULL) {
      std::free(string_buf_);
      string_buf_size_ = 0;
    }
  }

  void setStringSizeLimit(int32_t string_limit) {
    string_limit_ = string_limit;
  }

  void setContainerSizeLimit(int32_t container_limit) {
    container_limit_ = container_limit;
  }

  void setStrict(bool strict_read, bool strict_write) {
    strict_read_ = strict_read;
    strict_write_ = strict_write;
  }

  /**
   * Writing functions.
   */

  /*ol*/ int32_t writeMessageBegin(const std::string& name,
                                   const TMessageType messageType,
                                   const int32_t seqid);

  /*ol*/ int32_t writeMessageEnd();


  inline int32_t writeStructBegin(const char* name);

  inline int32_t writeStructEnd();

  inline int32_t writeSandeshBegin(const char* name);

  inline int32_t writeSandeshEnd();

  inline int32_t writeContainerElementBegin();

  inline int32_t writeContainerElementEnd();

  inline int32_t writeFieldBegin(const char* name,
                                 const TType fieldType,
                                 const int16_t fieldId,
                                 const std::map<std::string, std::string> *const amap);

  inline int32_t writeFieldEnd();

  inline int32_t writeFieldStop();

  inline int32_t writeMapBegin(const TType keyType,
                               const TType valType,
                               const uint32_t size);

  inline int32_t writeMapEnd();

  inline int32_t writeListBegin(const TType elemType, const uint32_t size);

  inline int32_t writeListEnd();

  inline int32_t writeSetBegin(const TType elemType, const uint32_t size);

  inline int32_t writeSetEnd();

  inline int32_t writeBool(const bool value);

  inline int32_t writeByte(const int8_t byte);

  inline int32_t writeI16(const int16_t i16);

  inline int32_t writeI32(const int32_t i32);

  inline int32_t writeI64(const int64_t i64);

  inline int32_t writeU16(const uint16_t u16);

  inline int32_t writeU32(const uint32_t u32);

  inline int32_t writeU64(const uint64_t u64);
  
  inline int32_t writeDouble(const double dub);

  inline int32_t writeString(const std::string& str);

  inline int32_t writeBinary(const std::string& str);

  inline int32_t writeXML(const std::string& str);

  /**
   * Reading functions
   */


  /*ol*/ int32_t readMessageBegin(std::string& name,
                                  TMessageType& messageType,
                                  int32_t& seqid);

  /*ol*/ int32_t readMessageEnd();

  inline int32_t readStructBegin(std::string& name);

  inline int32_t readStructEnd();

  int32_t readSandeshBegin(std::string& name);

  int32_t readSandeshEnd();

  inline int32_t readContainerElementBegin();

  inline int32_t readContainerElementEnd();

  inline int32_t readFieldBegin(std::string& name,
                                TType& fieldType,
                                int16_t& fieldId);

  inline int32_t readFieldEnd();

  inline int32_t readMapBegin(TType& keyType,
                               TType& valType,
                               uint32_t& size);

  inline int32_t readMapEnd();

  inline int32_t readListBegin(TType& elemType, uint32_t& size);

  inline int32_t readListEnd();

  inline int32_t readSetBegin(TType& elemType, uint32_t& size);

  inline int32_t readSetEnd();

  inline int32_t readBool(bool& value);
  // Provide the default readBool() implementation for std::vector<bool>
  using TVirtualProtocol< TBinaryProtocolT<Transport_> >::readBool;

  inline int32_t readByte(int8_t& byte);

  inline int32_t readI16(int16_t& i16);

  inline int32_t readI32(int32_t& i32);

  inline int32_t readI64(int64_t& i64);

  inline int32_t readU16(uint16_t& u16);

  inline int32_t readU32(uint32_t& u32);

  inline int32_t readU64(uint64_t& u64);
  
  inline int32_t readDouble(double& dub);

  inline int32_t readString(std::string& str);

  inline int32_t readBinary(std::string& str);

  inline int32_t readXML(std::string& str);

 protected:
  int32_t readStringBody(std::string& str, int32_t sz);

  Transport_* trans_;

  int32_t string_limit_;
  int32_t container_limit_;

  // Enforce presence of version identifier
  bool strict_read_;
  bool strict_write_;

  // Buffer for reading strings, save for the lifetime of the protocol to
  // avoid memory churn allocating memory on every string read
  uint8_t* string_buf_;
  int32_t string_buf_size_;

};

typedef TBinaryProtocolT<TTransport> TBinaryProtocol;

/**
 * Constructs binary protocol handlers
 */
template <class Transport_>
class TBinaryProtocolFactoryT : public TProtocolFactory {
 public:
  TBinaryProtocolFactoryT() :
    string_limit_(0),
    container_limit_(0),
    strict_read_(false),
    strict_write_(true) {}

  TBinaryProtocolFactoryT(int32_t string_limit, int32_t container_limit,
                          bool strict_read, bool strict_write) :
    string_limit_(string_limit),
    container_limit_(container_limit),
    strict_read_(strict_read),
    strict_write_(strict_write) {}

  virtual ~TBinaryProtocolFactoryT() {}

  void setStringSizeLimit(int32_t string_limit) {
    string_limit_ = string_limit;
  }

  void setContainerSizeLimit(int32_t container_limit) {
    container_limit_ = container_limit;
  }

  void setStrict(bool strict_read, bool strict_write) {
    strict_read_ = strict_read;
    strict_write_ = strict_write;
  }

  boost::shared_ptr<TProtocol> getProtocol(boost::shared_ptr<TTransport> trans) {
    boost::shared_ptr<Transport_> specific_trans =
      boost::dynamic_pointer_cast<Transport_>(trans);
    TProtocol* prot;
    if (specific_trans) {
      prot = new TBinaryProtocolT<Transport_>(specific_trans, string_limit_,
                                              container_limit_, strict_read_,
                                              strict_write_);
    } else {
      prot = new TBinaryProtocol(trans, string_limit_, container_limit_,
                                 strict_read_, strict_write_);
    }

    return boost::shared_ptr<TProtocol>(prot);
  }

 private:
  int32_t string_limit_;
  int32_t container_limit_;
  bool strict_read_;
  bool strict_write_;

};

typedef TBinaryProtocolFactoryT<TTransport> TBinaryProtocolFactory;

}}} // contrail::sandesh::protocol

#include "sandesh/library/cpp/protocol/TBinaryProtocol.tcc"

#endif // #ifndef _SANDESH_PROTOCOL_TBINARYPROTOCOL_H_
