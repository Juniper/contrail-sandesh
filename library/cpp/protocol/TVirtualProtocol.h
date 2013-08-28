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

#ifndef _SANDESH_PROTOCOL_TVIRTUALPROTOCOL_H_
#define _SANDESH_PROTOCOL_TVIRTUALPROTOCOL_H_ 1

#include <sandesh/protocol/TProtocol.h>

namespace contrail { namespace sandesh { namespace protocol {

/**
 * Helper class that provides default implementations of TProtocol methods.
 *
 * This class provides default implementations of the non-virtual TProtocol
 * methods.  It exists primarily so TVirtualProtocol can derive from it.  It
 * prevents TVirtualProtocol methods from causing infinite recursion if the
 * non-virtual methods are not overridden by the TVirtualProtocol subclass.
 *
 * You probably don't want to use this class directly.  Use TVirtualProtocol
 * instead.
 */
class TProtocolDefaults : public TProtocol {
 public:
  int32_t readMessageBegin(std::string& name,
                           TMessageType& messageType,
                           int32_t& seqid) {
    (void) name;
    (void) messageType;
    (void) seqid;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readMessageEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readStructBegin(std::string& name) {
    (void) name;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readStructEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readSandeshBegin(std::string& name) {
    (void) name;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readSandeshEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readContainerElementBegin() {
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readContainerElementEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readFieldBegin(std::string& name,
                         TType& fieldType,
                         int16_t& fieldId) {
    (void) name;
    (void) fieldType;
    (void) fieldId;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readFieldEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readMapBegin(TType& keyType, TType& valType, uint32_t& size) {
    (void) keyType;
    (void) valType;
    (void) size;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readMapEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readListBegin(TType& elemType, uint32_t& size) {
    (void) elemType;
    (void) size;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readListEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readSetBegin(TType& elemType, uint32_t& size) {
    (void) elemType;
    (void) size;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readSetEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readBool(bool& value) {
    (void) value;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readBool(std::vector<bool>::reference value) {
    (void) value;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readByte(int8_t& byte) {
    (void) byte;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readI16(int16_t& i16) {
    (void) i16;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readI32(int32_t& i32) {
    (void) i32;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readI64(int64_t& i64) {
    (void) i64;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readU16(uint16_t& u16) {
    (void) u16;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readU32(uint32_t& u32) {
    (void) u32;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }
  
  int32_t readU64(uint64_t& u64) {
    (void) u64;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }
  
  int32_t readDouble(double& dub) {
    (void) dub;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readString(std::string& str) {
    (void) str;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readBinary(std::string& str) {
    (void) str;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t readXML(std::string& str) {
    (void) str;
    LOG(ERROR, __func__ << ": This protocol does not support reading (yet).");
    assert(false);
    return -1;
  }

  int32_t writeMessageBegin(const std::string& name,
                             const TMessageType messageType,
                             const int32_t seqid) {
    (void) name;
    (void) messageType;
    (void) seqid;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeMessageEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeStructBegin(const char* name) {
    (void) name;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeStructEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeSandeshBegin(const char* name) {
    (void) name;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeSandeshEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeContainerElementBegin() {
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeContainerElementEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeFieldBegin(const char* name,
                          const TType fieldType,
                          const int16_t fieldId,
                          const std::map<std::string, std::string> *const amap = NULL) {
    (void) name;
    (void) fieldType;
    (void) fieldId;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeFieldEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeFieldStop() {
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeMapBegin(const TType keyType,
                         const TType valType,
                         const uint32_t size) {
    (void) keyType;
    (void) valType;
    (void) size;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeMapEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeListBegin(const TType elemType, const uint32_t size) {
    (void) elemType;
    (void) size;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeListEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeSetBegin(const TType elemType, const uint32_t size) {
    (void) elemType;
    (void) size;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeSetEnd() {
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeBool(const bool value) {
    (void) value;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeByte(const int8_t byte) {
    (void) byte;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeI16(const int16_t i16) {
    (void) i16;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeI32(const int32_t i32) {
    (void) i32;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeI64(const int64_t i64) {
    (void) i64;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeU16(const uint16_t i16) {
    (void) i16;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeU32(const uint32_t i32) {
    (void) i32;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeU64(const uint64_t i64) {
    (void) i64;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeDouble(const double dub) {
    (void) dub;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeString(const std::string& str) {
    (void) str;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeBinary(const std::string& str) {
    (void) str;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t writeXML(const std::string& str) {
    (void) str;
    LOG(ERROR, __func__ << ": This protocol does not support writing (yet).");
    assert(false);
    return -1;
  }

  int32_t skip(TType type) {
    return ::contrail::sandesh::protocol::skip(*this, type);
  }

 protected:
  TProtocolDefaults(boost::shared_ptr<TTransport> ptrans)
    : TProtocol(ptrans)
  {}
};

/**
 * Concrete TProtocol classes should inherit from TVirtualProtocol
 * so they don't have to manually override virtual methods.
 */
template <class Protocol_, class Super_=TProtocolDefaults>
class TVirtualProtocol : public Super_ {
 public:
  /**
   * Writing functions.
   */

  virtual int32_t writeMessageBegin_virt(const std::string& name,
                                         const TMessageType messageType,
                                         const int32_t seqid) {
    return static_cast<Protocol_*>(this)->writeMessageBegin(name, messageType,
                                                            seqid);
  }

  virtual int32_t writeMessageEnd_virt() {
    return static_cast<Protocol_*>(this)->writeMessageEnd();
  }


  virtual int32_t writeStructBegin_virt(const char* name) {
    return static_cast<Protocol_*>(this)->writeStructBegin(name);
  }

  virtual int32_t writeStructEnd_virt() {
    return static_cast<Protocol_*>(this)->writeStructEnd();
  }

  virtual int32_t writeSandeshBegin_virt(const char* name) {
    return static_cast<Protocol_*>(this)->writeSandeshBegin(name);
  }

  virtual int32_t writeSandeshEnd_virt() {
    return static_cast<Protocol_*>(this)->writeSandeshEnd();
  }

  virtual int32_t writeContainerElementBegin_virt() {
    return static_cast<Protocol_*>(this)->writeContainerElementBegin();
  }

  virtual int32_t writeContainerElementEnd_virt() {
    return static_cast<Protocol_*>(this)->writeContainerElementEnd();
  }

  virtual int32_t writeFieldBegin_virt(const char* name,
                                       const TType fieldType,
                                       const int16_t fieldId,
                                       const std::map<std::string, std::string> *const amap = NULL) {
    return static_cast<Protocol_*>(this)->writeFieldBegin(name, fieldType,
                                                          fieldId, amap);
  }

  virtual int32_t writeFieldEnd_virt() {
    return static_cast<Protocol_*>(this)->writeFieldEnd();
  }

  virtual int32_t writeFieldStop_virt() {
    return static_cast<Protocol_*>(this)->writeFieldStop();
  }

  virtual int32_t writeMapBegin_virt(const TType keyType,
                                     const TType valType,
                                     const uint32_t size) {
    return static_cast<Protocol_*>(this)->writeMapBegin(keyType, valType, size);
  }

  virtual int32_t writeMapEnd_virt() {
    return static_cast<Protocol_*>(this)->writeMapEnd();
  }

  virtual int32_t writeListBegin_virt(const TType elemType,
                                      const uint32_t size) {
    return static_cast<Protocol_*>(this)->writeListBegin(elemType, size);
  }

  virtual int32_t writeListEnd_virt() {
    return static_cast<Protocol_*>(this)->writeListEnd();
  }

  virtual int32_t writeSetBegin_virt(const TType elemType,
                                     const uint32_t size) {
    return static_cast<Protocol_*>(this)->writeSetBegin(elemType, size);
  }

  virtual int32_t writeSetEnd_virt() {
    return static_cast<Protocol_*>(this)->writeSetEnd();
  }

  virtual int32_t writeBool_virt(const bool value) {
    return static_cast<Protocol_*>(this)->writeBool(value);
  }

  virtual int32_t writeByte_virt(const int8_t byte) {
    return static_cast<Protocol_*>(this)->writeByte(byte);
  }

  virtual int32_t writeI16_virt(const int16_t i16) {
    return static_cast<Protocol_*>(this)->writeI16(i16);
  }

  virtual int32_t writeI32_virt(const int32_t i32) {
    return static_cast<Protocol_*>(this)->writeI32(i32);
  }

  virtual int32_t writeI64_virt(const int64_t i64) {
    return static_cast<Protocol_*>(this)->writeI64(i64);
  }

  virtual int32_t writeU16_virt(const uint16_t u16) {
    return static_cast<Protocol_*>(this)->writeU16(u16);
  }

  virtual int32_t writeU32_virt(const uint32_t u32) {
    return static_cast<Protocol_*>(this)->writeU32(u32);
  }

  virtual int32_t writeU64_virt(const uint64_t u64) {
    return static_cast<Protocol_*>(this)->writeU64(u64);
  }

  virtual int32_t writeDouble_virt(const double dub) {
    return static_cast<Protocol_*>(this)->writeDouble(dub);
  }

  virtual int32_t writeString_virt(const std::string& str) {
    return static_cast<Protocol_*>(this)->writeString(str);
  }

  virtual int32_t writeBinary_virt(const std::string& str) {
    return static_cast<Protocol_*>(this)->writeBinary(str);
  }

  virtual int32_t writeXML_virt(const std::string& str) {
    return static_cast<Protocol_*>(this)->writeXML(str);
  }

  /**
   * Reading functions
   */

  virtual int32_t readMessageBegin_virt(std::string& name,
                                        TMessageType& messageType,
                                        int32_t& seqid) {
    return static_cast<Protocol_*>(this)->readMessageBegin(name, messageType,
                                                           seqid);
  }

  virtual int32_t readMessageEnd_virt() {
    return static_cast<Protocol_*>(this)->readMessageEnd();
  }

  virtual int32_t readStructBegin_virt(std::string& name) {
    return static_cast<Protocol_*>(this)->readStructBegin(name);
  }

  virtual int32_t readStructEnd_virt() {
    return static_cast<Protocol_*>(this)->readStructEnd();
  }

  virtual int32_t readSandeshBegin_virt(std::string& name) {
    return static_cast<Protocol_*>(this)->readSandeshBegin(name);
  }

  virtual int32_t readSandeshEnd_virt() {
    return static_cast<Protocol_*>(this)->readSandeshEnd();
  }

  virtual int32_t readContainerElementBegin_virt() {
    return static_cast<Protocol_*>(this)->readContainerElementBegin();
  }

  virtual int32_t readContainerElementEnd_virt() {
    return static_cast<Protocol_*>(this)->readContainerElementEnd();
  }

  virtual int32_t readFieldBegin_virt(std::string& name,
                                      TType& fieldType,
                                      int16_t& fieldId) {
    return static_cast<Protocol_*>(this)->readFieldBegin(name, fieldType,
                                                         fieldId);
  }

  virtual int32_t readFieldEnd_virt() {
    return static_cast<Protocol_*>(this)->readFieldEnd();
  }

  virtual int32_t readMapBegin_virt(TType& keyType,
                                    TType& valType,
                                    uint32_t& size) {
    return static_cast<Protocol_*>(this)->readMapBegin(keyType, valType, size);
  }

  virtual int32_t readMapEnd_virt() {
    return static_cast<Protocol_*>(this)->readMapEnd();
  }

  virtual int32_t readListBegin_virt(TType& elemType,
                                     uint32_t& size) {
    return static_cast<Protocol_*>(this)->readListBegin(elemType, size);
  }

  virtual int32_t readListEnd_virt() {
    return static_cast<Protocol_*>(this)->readListEnd();
  }

  virtual int32_t readSetBegin_virt(TType& elemType,
                                    uint32_t& size) {
    return static_cast<Protocol_*>(this)->readSetBegin(elemType, size);
  }

  virtual int32_t readSetEnd_virt() {
    return static_cast<Protocol_*>(this)->readSetEnd();
  }

  virtual int32_t readBool_virt(bool& value) {
    return static_cast<Protocol_*>(this)->readBool(value);
  }

  virtual int32_t readBool_virt(std::vector<bool>::reference value) {
    return static_cast<Protocol_*>(this)->readBool(value);
  }

  virtual int32_t readByte_virt(int8_t& byte) {
    return static_cast<Protocol_*>(this)->readByte(byte);
  }

  virtual int32_t readI16_virt(int16_t& i16) {
    return static_cast<Protocol_*>(this)->readI16(i16);
  }

  virtual int32_t readI32_virt(int32_t& i32) {
    return static_cast<Protocol_*>(this)->readI32(i32);
  }

  virtual int32_t readI64_virt(int64_t& i64) {
    return static_cast<Protocol_*>(this)->readI64(i64);
  }

  virtual int32_t readU16_virt(uint16_t& u16) {
    return static_cast<Protocol_*>(this)->readU16(u16);
  }

  virtual int32_t readU32_virt(uint32_t& u32) {
    return static_cast<Protocol_*>(this)->readU32(u32);
  }

  virtual int32_t readU64_virt(uint64_t& u64) {
    return static_cast<Protocol_*>(this)->readU64(u64);
  }

  virtual int32_t readDouble_virt(double& dub) {
    return static_cast<Protocol_*>(this)->readDouble(dub);
  }

  virtual int32_t readString_virt(std::string& str) {
    return static_cast<Protocol_*>(this)->readString(str);
  }

  virtual int32_t readBinary_virt(std::string& str) {
    return static_cast<Protocol_*>(this)->readBinary(str);
  }

  virtual int32_t readXML_virt(std::string& str) {
    return static_cast<Protocol_*>(this)->readXML(str);
  }

  virtual int32_t skip_virt(TType type) {
    return static_cast<Protocol_*>(this)->skip(type);
  }

  /*
   * Provide a default skip() implementation that uses non-virtual read
   * methods.
   *
   * Note: subclasses that use TVirtualProtocol to derive from another protocol
   * implementation (i.e., not TProtocolDefaults) should beware that this may
   * override any non-default skip() implementation provided by the parent
   * transport class.  They may need to explicitly redefine skip() to call the
   * correct parent implementation, if desired.
   */
  int32_t skip(TType type) {
    Protocol_* const prot = static_cast<Protocol_*>(this);
    return ::contrail::sandesh::protocol::skip(*prot, type);
  }

  /*
   * Provide a default readBool() implementation for use with
   * std::vector<bool>, that behaves the same as reading into a normal bool.
   *
   * Subclasses can override this if desired, but there normally shouldn't
   * be a need to.
   */
  int32_t readBool(std::vector<bool>::reference value) {
    bool b = false;
    uint32_t ret = static_cast<Protocol_*>(this)->readBool(b);
    value = b;
    return ret;
  }
  using Super_::readBool; // so we don't hide readBool(bool&)

 protected:
 protected:
  TVirtualProtocol(boost::shared_ptr<TTransport> ptrans)
    : Super_(ptrans)
  {}
};

}}} // contrail::sandesh::protocol

#endif // #define _SANDESH_PROTOCOL_TVIRTUALPROTOCOL_H_ 1
