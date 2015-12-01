/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef _SANDESH_PROTOCOL_TXMLPROTOCOL_H_
#define _SANDESH_PROTOCOL_TXMLPROTOCOL_H_ 1

#include <string.h>
#include "TVirtualProtocol.h"

#include <boost/shared_ptr.hpp>
#include <boost/tokenizer.hpp>

namespace contrail { namespace sandesh { namespace protocol {

/**
 * Protocol that prints the payload in XML format.
 */
class TXMLProtocol : public TVirtualProtocol<TXMLProtocol> {
 private:
  enum write_state_t
  { UNINIT
  , STRUCT
  , LIST
  , SET
  , MAP
  , SNDESH
  };

 public:
  TXMLProtocol(boost::shared_ptr<TTransport> trans)
    : TVirtualProtocol<TXMLProtocol>(trans)
    , trans_(trans.get())
    , string_limit_(DEFAULT_STRING_LIMIT)
    , string_prefix_size_(DEFAULT_STRING_PREFIX_SIZE)
    , reader_(*trans)
  {
    write_state_.push_back(UNINIT);
  }

  static const int32_t DEFAULT_STRING_LIMIT = 256;
  static const int32_t DEFAULT_STRING_PREFIX_SIZE = 16;

  void setStringSizeLimit(int32_t string_limit) {
    string_limit_ = string_limit;
  }

  void setStringPrefixSize(int32_t string_prefix_size) {
    string_prefix_size_ = string_prefix_size;
  }

  static std::string escapeXMLControlCharsInternal(const std::string& str) {
    std::string xmlstr;
    xmlstr.reserve(str.length());
    for (std::string::const_iterator it = str.begin();
         it != str.end(); ++it) {
      switch(*it) {
       case '&':  xmlstr += "&amp;";  break;
       case '\'': xmlstr += "&apos;"; break;
       case '<':  xmlstr += "&lt;";   break;
       case '>':  xmlstr += "&gt;";   break;
       default:   xmlstr += *it;
      }
    }
    return xmlstr;
  }

  static std::string escapeXMLControlChars(const std::string& str) {
    if (strpbrk(str.c_str(), "&'<>") != NULL) {
      return escapeXMLControlCharsInternal(str);
    } else {
      return str;
    }
  }

  static void unescapeXMLControlChars(std::string& str) {
      boost::replace_all(str, "&amp;", "&");
      boost::replace_all(str, "&apos;", "\'");
      boost::replace_all(str, "&lt;", "<");
      boost::replace_all(str, "&gt;", ">");
  }

  /**
   * Writing functions
   */

  int32_t writeMessageBegin(const std::string& name,
                            const TMessageType messageType,
                            const int32_t seqid);

  int32_t writeMessageEnd();

  int32_t writeStructBegin(const char* name);

  int32_t writeStructEnd();

  int32_t writeSandeshBegin(const char* name);

  int32_t writeSandeshEnd();

  int32_t writeContainerElementBegin();

  int32_t writeContainerElementEnd();

  int32_t writeFieldBegin(const char* name,
                          const TType fieldType,
                          const int16_t fieldId,
                          const std::map<std::string, std::string> *const amap = NULL);

  int32_t writeFieldEnd();

  int32_t writeFieldStop();

  int32_t writeMapBegin(const TType keyType,
                        const TType valType,
                        const uint32_t size);

  int32_t writeMapEnd();

  int32_t writeListBegin(const TType elemType,
                         const uint32_t size);

  int32_t writeListEnd();

  int32_t writeSetBegin(const TType elemType,
                        const uint32_t size);

  int32_t writeSetEnd();

  int32_t writeBool(const bool value);

  int32_t writeByte(const int8_t byte);

  int32_t writeI16(const int16_t i16);

  int32_t writeI32(const int32_t i32);

  int32_t writeI64(const int64_t i64);

  int32_t writeU16(const uint16_t u16);

  int32_t writeU32(const uint32_t u32);
  
  int32_t writeU64(const uint64_t u64);

  int32_t writeIPV4(const uint32_t ip4);

  int32_t writeIPADDR(const boost::asio::ip::address& ipaddress);
  
  int32_t writeDouble(const double dub);

  int32_t writeString(const std::string& str);

  int32_t writeBinary(const std::string& str);

  int32_t writeXML(const std::string& str);

  int32_t writeUUID(const boost::uuids::uuid& uuid);

  /**
   * Reading functions
   */

  int32_t readMessageBegin(std::string& name,
                           TMessageType& messageType,
                           int32_t& seqid);

  int32_t readMessageEnd();

  int32_t readSandeshBegin(std::string& name);

  int32_t readSandeshEnd();

  int32_t readStructBegin(std::string& name);

  int32_t readStructEnd();

  int32_t readContainerElementBegin();

  int32_t readContainerElementEnd();

  int32_t readFieldBegin(std::string& name,
                         TType& fieldType,
                         int16_t& fieldId);

  int32_t readFieldEnd();

  int32_t readMapBegin(TType& keyType,
                       TType& valType,
                       uint32_t& size);

  int32_t readMapEnd();

  int32_t readListBegin(TType& elemType,
                        uint32_t& size);

  int32_t readListEnd();

  int32_t readSetBegin(TType& elemType,
                       uint32_t& size);

  int32_t readSetEnd();

  int32_t readBool(bool& value);

  // Provide the default readBool() implementation for std::vector<bool>
  using TVirtualProtocol<TXMLProtocol>::readBool;

  int32_t readByte(int8_t& byte);

  int32_t readI16(int16_t& i16);

  int32_t readI32(int32_t& i32);

  int32_t readI64(int64_t& i64);

  int32_t readU16(uint16_t& u16);

  int32_t readU32(uint32_t& u32);
  
  int32_t readU64(uint64_t& u64);

  int32_t readIPV4(uint32_t& ip4);

  int32_t readIPADDR(boost::asio::ip::address& ipaddress);

  int32_t readDouble(double& dub);

  int32_t readString(std::string& str);

  int32_t readBinary(std::string& str);

  int32_t readXML(std::string& str);

  int32_t readUUID(boost::uuids::uuid& uuid);

  class LookaheadReader {

   public:

    LookaheadReader(TTransport &trans) :
      trans_(&trans),
      hasData_(false),
      has2Data_(false),
      firstRead_(false) {
    }

    uint8_t read() {
      if (hasData_) {
        hasData_ = false;
      }
      else {
        if (has2Data_) {
          if (firstRead_) {
            has2Data_ = false;
            firstRead_ = false;
            return data2_[1];
          } else {
            firstRead_ = true;
            return data2_[0];
          }
        } else {
          trans_->readAll(&data_, 1);
        }
      }
      return data_;
    }

    uint8_t peek() {
      if (!hasData_) {
        trans_->readAll(&data_, 1);
      }
      hasData_ = true;
      return data_;
    }

    uint8_t peek2() {
      bool first = false;
      if (!has2Data_) {
        trans_->readAll(data2_, 2);
        first = true;
      }
      has2Data_ = true;
      return first ? data2_[0] : data2_[1];
    }

   private:
    TTransport *trans_;
    bool hasData_;
    uint8_t data_;
    bool has2Data_;
    uint8_t data2_[2];
    bool firstRead_;
  };

 private:
  typedef boost::tokenizer<boost::char_separator<char> >
      tokenizer;

  int32_t readXMLSyntaxChar(uint8_t ch);

  int32_t readXMLSyntaxString(const std::string &str);

  int32_t readXMLString(std::string &str);

  int32_t readXMLTag(std::string &str, bool endTag = false);

  int32_t readXMLNumericChars(std::string &str);

  int32_t readXMLCDATA(std::string &str);

  template <typename NumberType>
  int32_t readXMLInteger(NumberType &num);

  int32_t readXMLDouble(double &num);

  int32_t readXMLUuid(boost::uuids::uuid &uuid);

  void indentUp();
  void indentDown();
  int32_t writePlain(const std::string& str);
  int32_t writeIndented(const std::string& str);

  static const std::string& fieldTypeName(TType type);
  static TType getTypeIDForTypeName(const std::string &name);

  TTransport* trans_;

  int32_t string_limit_;
  int32_t string_prefix_size_;

  std::string indent_str_;

  static const int indent_inc = 2;

  std::vector<write_state_t> write_state_;

  std::vector<std::string> xml_state_;
  LookaheadReader reader_;
};

/**
 * Constructs XML protocol handlers
 */
class TXMLProtocolFactory : public TProtocolFactory {
 public:
  TXMLProtocolFactory() {}
  virtual ~TXMLProtocolFactory() {}

  boost::shared_ptr<TProtocol> getProtocol(boost::shared_ptr<TTransport> trans) {
    return boost::shared_ptr<TProtocol>(new TXMLProtocol(trans));
  }

};

}}} // contrail::sandesh::protocol

#endif // #ifndef _SANDESH_PROTOCOL_TXMLPROTOCOL_H_


