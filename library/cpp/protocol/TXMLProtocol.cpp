/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#include <cassert>
#include <cctype>
#include <cstdio>
#include <stdexcept>
#include <boost/static_assert.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <base/logging.h>
#include <base/util.h>
#include <base/string_util.h>

#include "TXMLProtocol.h"

using std::string;

#ifdef TXMLPROTOCOL_DEBUG_PRETTY_PRINT
static const string endl = "\n";
#else
static const string endl = "";
#endif // !TXMLPROTOCOL_DEBUG_PRETTY_PRINT

namespace contrail { namespace sandesh { namespace protocol {

// Static data

static const uint8_t kcXMLTagO = '<';
static const uint8_t kcXMLTagC = '>';
static const uint8_t kcXMLSlash = '/';
static const uint8_t kcXMLSBracketC = ']';

static const std::string kXMLTagO("<");
static const std::string kXMLTagC(">");
static const std::string kXMLSoloTagC("/>");
static const std::string kXMLEndTagO("</");
static const std::string kXMLSlash("/");
static const std::string kXMLType("type");
static const std::string kXMLIdentifier("identifier");
static const std::string kXMLName("name");
static const std::string kXMLKey("key");
static const std::string kXMLElementTagO("<element>");
static const std::string kXMLElementTagC("</element>");
static const std::string kXMLValue("value");
static const std::string kXMLSize("size");
static const std::string kXMLBoolTrue("true");
static const std::string kXMLBoolFalse("false");
static const std::string kXMLCDATAO("<![CDATA[");
static const std::string kXMLCDATAC("]]>");

static const std::string kTypeNameBool("bool");
static const std::string kTypeNameByte("byte");
static const std::string kTypeNameI16("i16");
static const std::string kTypeNameI32("i32");
static const std::string kTypeNameI64("i64");
static const std::string kTypeNameU16("u16");
static const std::string kTypeNameU32("u32");
static const std::string kTypeNameU64("u64");
static const std::string kTypeNameIPV4("ipv4");
static const std::string kTypeNameDouble("double");
static const std::string kTypeNameStruct("struct");
static const std::string kTypeNameString("string");
static const std::string kTypeNameXML("xml");
static const std::string kTypeNameUUID("uuid_t");
static const std::string kTypeNameMap("map");
static const std::string kTypeNameList("list");
static const std::string kTypeNameSet("set");
static const std::string kTypeNameSandesh("sandesh");
static const std::string kTypeNameUnknown("unknown");

static const std::string kAttrTypeSandesh("type=\"sandesh\"");
static const std::string kXMLListTagO("<list ");
static const std::string kXMLListTagC("</list>" + endl);
static const std::string kXMLSetTagO("<set ");
static const std::string kXMLSetTagC("</set>" + endl);
static const std::string kXMLMapTagO("<map ");
static const std::string kXMLMapTagC("</map>" + endl);

const std::string& TXMLProtocol::fieldTypeName(TType type) {
  switch (type) {
    case T_BOOL   : return kTypeNameBool   ;
    case T_BYTE   : return kTypeNameByte   ;
    case T_I16    : return kTypeNameI16    ;
    case T_I32    : return kTypeNameI32    ;
    case T_I64    : return kTypeNameI64    ;
    case T_U16    : return kTypeNameU16    ;
    case T_U32    : return kTypeNameU32    ;
    case T_U64    : return kTypeNameU64    ;
    case T_IPV4   : return kTypeNameIPV4   ;
    case T_DOUBLE : return kTypeNameDouble ;
    case T_STRING : return kTypeNameString ;
    case T_STRUCT : return kTypeNameStruct ;
    case T_MAP    : return kTypeNameMap    ;
    case T_SET    : return kTypeNameSet    ;
    case T_LIST   : return kTypeNameList   ;
    case T_SANDESH: return kTypeNameSandesh;
    case T_XML    : return kTypeNameXML    ;
    case T_UUID   : return kTypeNameUUID   ;
    default: return kTypeNameUnknown;
  }
}

TType TXMLProtocol::getTypeIDForTypeName(const std::string &name) {
  TType result = T_STOP; // Sentinel value
  if (name.length() > 1) {
    switch (name[0]) {
    case 'b':
        switch (name[1]) {
        case 'o':
            result = T_BOOL;
            break;
        case 'y':
            result = T_BYTE;
            break;
        }
        break;
    case 'd':
      result = T_DOUBLE;
      break;
    case 'i':
      switch (name[1]) {
      case '1':
        result = T_I16;
        break;
      case '3':
        result = T_I32;
        break;
      case '6':
        result = T_I64;
        break;
      case 'p':
        result = T_IPV4;
        break;
      }
      break;
    case 'l':
      result = T_LIST;
      break;
    case 'm':
      result = T_MAP;
      break;
    case 's':
        switch (name[1]) {
        case 'a':
            result = T_SANDESH;
            break;
        case 'e':
            result = T_SET;
            break;
        case 't':
            switch (name[3]) {
            case 'i':
                result = T_STRING;
                break;
            case 'u':
                result = T_STRUCT;
                break;
            }
            break;
        }
        break;
    case 'u':
      switch(name[1]) {
        case '1':
          result = T_U16;
          break;
        case '3':
          result = T_U32;
          break;
        case '6':
          result = T_U64;
          break;
        case 'u':
          result = T_UUID;
          break;
      }
      break;
    case 'x':
      result = T_XML;
      break;
    }
  }
  if (result == T_STOP) {
    LOG(ERROR, __func__ << "Unrecognized type: " << name);
  }
  return result;
}

static inline void formXMLAttr(std::string &dest, const std::string& name, 
                               const std::string & value) {
  dest += name;
  dest += "=\"";
  dest += value; 
  dest += "\"";
}

void TXMLProtocol::indentUp() {
#if TXMLPROTOCOL_DEBUG_PRETTY_PRINT
  indent_str_ += string(indent_inc, ' ');
#endif // !TXMLPROTOCOL_DEBUG_PRETTY_PRINT
}

void TXMLProtocol::indentDown() {
#if TXMLPROTOCOL_DEBUG_PRETTY_PRINT
  if (indent_str_.length() < (string::size_type)indent_inc) {
    LOG(ERROR, __func__ << "Indent string length " <<
        indent_str_.length() << " less than indent length " <<
        (string::size_type)indent_inc);
    return;
  }
  indent_str_.erase(indent_str_.length() - indent_inc);
#endif // !TXMLPROTOOL_DEBUG_PRETTY_PRINT
}

// Returns the number of bytes written on success, -1 otherwise
int32_t TXMLProtocol::writePlain(const string& str) {
  int ret = trans_->write((uint8_t*)str.data(), str.length());
  if (ret) {
    return -1;
  }
  return str.length();
}

// Returns the number of bytes written on success, -1 otherwise
int32_t TXMLProtocol::writeIndented(const string& str) {
  int ret;
#if TXMLPROTOCOL_DEBUG_PRETTY_PRINT
  ret = trans_->write((uint8_t*)indent_str_.data(), indent_str_.length());
  if (ret) {
    return -1;
  }
#endif // !TXMLPROTOCOL_DEBUG_PRETTY_PRINT
  ret = trans_->write((uint8_t*)str.data(), str.length());
  if (ret) {
    return -1;
  }
  return indent_str_.length() + str.length();
}

int32_t TXMLProtocol::writeMessageBegin(const std::string& name,
                                        const TMessageType messageType,
                                        const int32_t seqid) {
  return 0;
}

int32_t TXMLProtocol::writeMessageEnd() {
  return 0;
}

int32_t TXMLProtocol::writeStructBegin(const char* name) {
  int32_t size = 0, ret;
  string sname(name);
  string xml;
  xml.reserve(512);
  // Form the xml tag
  xml += kXMLTagO;
  xml += sname;
  xml += kXMLTagC;
  xml += endl;
  // Write to transport
  if ((ret = writeIndented(xml)) < 0) {
    LOG(ERROR, __func__ << ": " << sname << " FAILED");
    return ret;
  }
  size += ret;
  xml_state_.push_back(sname);
  indentUp(); 
  return size;
}

int32_t TXMLProtocol::writeStructEnd() {
  indentDown();
  int32_t size = 0, ret;
  string etag(xml_state_.back());
  xml_state_.pop_back();
  string xml;
  xml.reserve(128);
  // Form the xml tag
  xml += kXMLEndTagO;
  xml += etag;
  xml += kXMLTagC;
  xml += endl;
  // Write to transport
  if ((ret = writeIndented(xml)) < 0) {
    LOG(ERROR, __func__ << ": " << etag << " FAILED");
    return ret;
  }
  size += ret;
  return size;
}

int32_t TXMLProtocol::writeSandeshBegin(const char* name) {
  int32_t size = 0, ret;
  string sname(name);
  string xml;
  xml.reserve(512);
  // Form the xml tag
  xml += kXMLTagO;
  xml += sname;
  xml += " ";
  xml += kAttrTypeSandesh;
  xml += kXMLTagC;
  xml += endl;
  // Write to transport
  ret = writeIndented(xml);
  if (ret < 0) {
    LOG(ERROR, __func__ << ": " << sname << " FAILED");
    return ret;
  }
  size += ret;
  xml_state_.push_back(sname);
  indentUp();
  return size;
}

int32_t TXMLProtocol::writeSandeshEnd() {
  indentDown();
  int32_t size = 0, ret;
  string etag(xml_state_.back());
  xml_state_.pop_back();
  string exml;
  exml.reserve(128);
  // Form the xml tag
  exml += kXMLEndTagO;
  exml += etag;
  exml += kXMLTagC;
  exml += endl;
  // Write to transport
  if ((ret = writeIndented(exml)) < 0) {
    LOG(ERROR, __func__ << ": " << etag << " FAILED");
    return ret;
  }
  size += ret;
  return size;
}

int32_t TXMLProtocol::writeContainerElementBegin() {
  return writeIndented(kXMLElementTagO);
}

int32_t TXMLProtocol::writeContainerElementEnd() {
  return writeIndented(kXMLElementTagC);
}

int32_t TXMLProtocol::writeFieldBegin(const char *name,
                                      const TType fieldType,
                                      const int16_t fieldId,
                                      const std::map<std::string, std::string> *const amap) {
  int32_t size = 0, ret;
  string sname(name);
  string xml;
  xml.reserve(512);
  // Form the xml tag
  xml += kXMLTagO;
  xml += sname;
  xml += " ";
  formXMLAttr(xml, kXMLType, fieldTypeName(fieldType));
  xml += " ";
  formXMLAttr(xml, kXMLIdentifier, integerToString(fieldId));
  if (amap != NULL) {
    for (std::map<string, string>::const_iterator iter = amap->begin(); 
         iter != amap->end(); iter++) {
      xml += " ";
      formXMLAttr(xml, (*iter).first, (*iter).second);
    }
  }
  xml += kXMLTagC;
  // Write to transport
  if ((ret = writeIndented(xml)) < 0) {
    LOG(ERROR, __func__ << ": " << xml << " FAILED");
    return ret;
  }
  size += ret;
  xml_state_.push_back(sname);
  if (fieldType == T_STRUCT) {
	write_state_.push_back(STRUCT);
#ifdef TXMLPROTOCOL_DEBUG_PRETTY_PRINT
	if ((ret = writePlain(endl)) < 0) {
	  LOG(ERROR, __func__ << ": " << fieldTypeName(fieldType) << " FAILED");
	  return ret;
	}
	size += ret;
	indentUp();
#endif // !TXMLPROTOCOL_DEBUG_PRETTY_PRINT
  } else if (fieldType == T_SANDESH) {
	write_state_.push_back(SNDESH);
#ifdef TXMLPROTOCOL_DEBUG_PRETTY_PRINT
    if ((ret = writePlain(endl)) < 0) {
      LOG(ERROR, __func__ << ": " << fieldTypeName(fieldType) << " FAILED");
      return ret;
    }
    size += ret;
#endif // !TXMLPROTOCOL_DEBUG_PRETTY_PRINT
  } else if (fieldType == T_LIST) {
	write_state_.push_back(LIST);
#ifdef TXMLPROTOCOL_DEBUG_PRETTY_PRINT
    if ((ret = writePlain(endl)) < 0) {
      LOG(ERROR, __func__ << ": " << fieldTypeName(fieldType) << " FAILED");
      return ret;
    }
    size += ret;
#endif // !TXMLPROTOCOL_DEBUG_PRETTY_PRINT
  } else if (fieldType == T_MAP) {
	write_state_.push_back(MAP);
#ifdef TXMLPROTOCOL_DEBUG_PRETTY_PRINT
    if ((ret = writePlain(endl)) < 0) {
      LOG(ERROR, __func__ << ": " << fieldTypeName(fieldType) << " FAILED");
      return ret;
    }
    size += ret;
#endif // !TXMLPROTOCOL_DEBUG_PRETTY_PRINT
  } else if (fieldType == T_SET) {
	write_state_.push_back(SET);
#ifdef TXMLPROTOCOL_DEBUG_PRETTY_PRINT
    if ((ret = writePlain(endl)) < 0) {
      LOG(ERROR, __func__ << ": " << fieldTypeName(fieldType) << " FAILED");
      return ret;
    }
    size += ret;
#endif // !TXMLPROTOCOL_DEBUG_PRETTY_PRINT
  } else {
    write_state_.push_back(UNINIT);
  }
  return size; 
}

int32_t TXMLProtocol::writeFieldEnd() {
  int32_t size = 0, ret;
  string etag(xml_state_.back());
  xml_state_.pop_back();
  string exml;
  exml.reserve(128);
  // Form the xml tag
  exml += kXMLEndTagO;
  exml += etag;
  exml += kXMLTagC;
  exml += endl;
  // Write to transport
  write_state_t state = write_state_.back();
  if (state != STRUCT &&
      state != SNDESH &&
      state != LIST   &&
      state != MAP    &&
      state != SET) {
    if ((ret = writePlain(exml)) < 0) {
      LOG(ERROR, __func__ << ": " << etag << " " << state <<
          " FAILED");
      return ret;
    }
    size += ret;
  } else {
    if (state == STRUCT) {
      indentDown();
    }
    if ((ret = writeIndented(exml)) < 0) {
      LOG(ERROR, __func__ << ": " << etag << " " << state <<
          " FAILED");
      return ret;
    }
    size += ret;
  }
  write_state_.pop_back();
  return size;
}

int32_t TXMLProtocol::writeFieldStop() {
  return 0;
}

int32_t TXMLProtocol::writeMapBegin(const TType keyType,
                                    const TType valType,
                                    const uint32_t size) {
  int32_t bsize = 0, ret;
  string xml;
  xml.reserve(256);
  // Form the xml tag
  xml += kXMLMapTagO;
  formXMLAttr(xml, kXMLKey, fieldTypeName(keyType));
  xml += " ";
  formXMLAttr(xml, kXMLValue, fieldTypeName(valType));
  xml += " ";
  formXMLAttr(xml, kXMLSize, integerToString(size));
  xml += kXMLTagC;
  xml += endl;
  // Write to transport
  ret = writeIndented(xml);
  if (ret < 0) {
    LOG(ERROR, __func__ << ": Key: " << fieldTypeName(keyType) <<
        " Value: " << fieldTypeName(valType) << " FAILED");
    return ret;
  }
  bsize += ret;
  indentUp();
  return bsize;
}

int32_t TXMLProtocol::writeMapEnd() {
  int32_t size = 0, ret;
  indentDown();
  if ((ret = writeIndented(kXMLMapTagC)) < 0) {
    LOG(ERROR, __func__ << " FAILED");
    return ret;
  }
  size += ret;
  return size;
}

int32_t TXMLProtocol::writeListBegin(const TType elemType,
                                     const uint32_t size) {
  int32_t bsize = 0, ret;
  string xml;
  xml.reserve(256);
  // Form the xml tag
  xml += kXMLListTagO;
  formXMLAttr(xml, kXMLType, fieldTypeName(elemType));
  xml += " ";
  formXMLAttr(xml, kXMLSize, integerToString(size));
  xml += kXMLTagC;
  xml += endl;
  // Write to transport
  ret = writeIndented(xml);
  if (ret < 0) {
    LOG(ERROR, __func__ << ": " << fieldTypeName(elemType) <<
        " FAILED");
    return ret;
  }
  bsize += ret;
  indentUp();
  return bsize;
}

int32_t TXMLProtocol::writeListEnd() {
  int32_t size = 0, ret;
  indentDown();
  ret = writeIndented(kXMLListTagC);
  if (ret < 0) {
    LOG(ERROR, __func__ << " FAILED");
    return ret;
  }
  size += ret;
  return size;
}

int32_t TXMLProtocol::writeSetBegin(const TType elemType,
                                    const uint32_t size) {
  int32_t bsize = 0, ret;
  string xml;
  xml.reserve(256);
  // Form the xml tag
  xml += kXMLSetTagO;
  formXMLAttr(xml, kXMLType, fieldTypeName(elemType));
  xml += " ";
  formXMLAttr(xml, kXMLSize, integerToString(size));
  xml += kXMLTagC;
  xml += endl;
  // Write to transport
  ret = writeIndented(xml);
  if (ret < 0) {
    LOG(ERROR, __func__ << ": " << fieldTypeName(elemType) <<
        " FAILED");
    return ret;
  }
  bsize += ret;
  indentUp();
  return bsize;
}

int32_t TXMLProtocol::writeSetEnd() {
  int32_t size = 0, ret;
  indentDown();
  if ((ret = writeIndented(kXMLSetTagC)) < 0) {
    LOG(ERROR, __func__ << " FAILED");
    return ret;
  }
  size += ret;
  return size;
}

int32_t TXMLProtocol::writeBool(const bool value) {
  return writePlain(value ? kXMLBoolTrue : kXMLBoolFalse);
}

int32_t TXMLProtocol::writeByte(const int8_t byte) {
  return writePlain(integerToString(byte));
}

int32_t TXMLProtocol::writeI16(const int16_t i16) {
  return writePlain(integerToString(i16));
}

int32_t TXMLProtocol::writeI32(const int32_t i32) {
  return writePlain(integerToString(i32));
}

int32_t TXMLProtocol::writeI64(const int64_t i64) {
  return writePlain(integerToString(i64));
}

int32_t TXMLProtocol::writeU16(const uint16_t u16) {
  return writePlain(integerToString(u16));
}

int32_t TXMLProtocol::writeU32(const uint32_t u32) {
  return writePlain(integerToString(u32));
}

int32_t TXMLProtocol::writeU64(const uint64_t u64) {
  return writePlain(integerToString(u64));
}

int32_t TXMLProtocol::writeIPV4(const uint32_t ip4) {
  return writePlain(integerToString(ip4));
}

int32_t TXMLProtocol::writeDouble(const double dub) {
  return writePlain(integerToString(dub));
}

int32_t TXMLProtocol::writeString(const string& str) {
  // Escape XML control characters in the string before writing
  return writePlain(escapeXMLControlChars(str));
}

int32_t TXMLProtocol::writeBinary(const string& str) {
  // XXX Hex?
  return writeString(str);
}

int32_t TXMLProtocol::writeXML(const string& str) {
  std::string xmlstr;
  xmlstr.reserve(str.length() + kXMLCDATAO.length() + kXMLCDATAC.length());
  xmlstr += kXMLCDATAO;
  xmlstr += str;
  xmlstr += kXMLCDATAC;
  return writePlain(xmlstr);
}

int32_t TXMLProtocol::writeUUID(const boost::uuids::uuid& uuid) {
  const std::string str = boost::uuids::to_string(uuid);
  return writeString(str);
}
/**
 * Reading functions
 */

// Return true if the character ch is in [-+0-9]; false otherwise
static bool isXMLNumeric(uint8_t ch) {
  switch (ch) {
  case '+':
  case '-':
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    return true;
  }
  return false;
}

// Read 1 character from the transport trans and verify that it is the
// expected character ch. Returns 1 if does, -1 if not
static int32_t readSyntaxChar(TXMLProtocol::LookaheadReader &reader,
                               uint8_t ch) {
  uint8_t ch2 = reader.read();
  if (ch2 != ch) {
    LOG(ERROR, __func__ << ": Expected \'" << std::string((char *)&ch, 1) <<
        "\'; got \'" << std::string((char *)&ch2, 1) << "\'.");
    return -1;
  }
  return 1;
}

// Reads string from the transport trans and verify that it is the
// expected string str. Returns 1 if does, -1 if not
static int32_t readSyntaxString(TXMLProtocol::LookaheadReader &reader,
                                const std::string str) {
  int32_t result = 0, ret;
  for (std::string::const_iterator it = str.begin(); it != str.end(); it++) {
    if ((ret = readSyntaxChar(reader, *it)) < 0) {
      return ret;
    }
    result += ret;
  }
  return result;
}

// Reads 1 byte and verifies that it matches ch. Returns 1 if it does,
// -1 otherwise
int32_t TXMLProtocol::readXMLSyntaxChar(uint8_t ch) {
  return readSyntaxChar(reader_, ch);
}

// Reads a XML number or string and interprets it as a double.
int32_t TXMLProtocol::readXMLDouble(double &num) {
  LOG(ERROR, __func__ << ": Not implemented in TXMLProtocol");
  assert(false);
  return -1;
}

// Reads string and verifies that it matches str. Returns 1 if it does,
// -1 otherwise
int32_t TXMLProtocol::readXMLSyntaxString(const std::string &str) {
  return readSyntaxString(reader_, str);
}

int32_t TXMLProtocol::readXMLCDATA(std::string &str) {
  int32_t result = 0, ret;
  str.clear();
  // Read <![CDATA[
  if ((ret = readXMLSyntaxString(kXMLCDATAO)) < 0) {
    return ret;
  }
  result += ret;
  uint8_t ch = 0, ch2 = 0, ch3;
  bool ch_read = false, ch2_read = false, ch3_read = false;
  while (true) {
    if (!ch_read) {
      ch = reader_.peek();
      reader_.read();
      ++result;
      ch_read = true;
    }
    // Did we read ]]> ?
    if (ch == kcXMLSBracketC) {
      if (!ch2_read) {
        ch2 = reader_.peek();
        reader_.read();
        ++result;
        ch2_read = true;
      }
      if (ch2 == kcXMLSBracketC) {
        if (!ch3_read) {
          ch3 = reader_.peek();
          reader_.read();
          ++result;
          ch3_read = true;
        }
        if (ch3 == kcXMLTagC) {
          break;
        } else {
          // Consume ch
          str += ch;
          // Rotate ch2 and ch3 and repeat
          ch = ch2;
          ch2 = ch3;
          ch3_read = false;
          continue;
        }
      } else {
        // Consume ch
        str += ch;
        // Rotate ch2 and repeat
        ch = ch2;
        ch2_read = false;
        continue;
      }
    } else {
      str += ch;
      ch_read = false;
    }
  }
  return result;
}

// Reads a sequence of characters, stopping at the first occurrence of xml
// tag open delimiter which signals end of field and returns the string
// via str.
int32_t TXMLProtocol::readXMLString(std::string &str) {
  int32_t result = 0;
  str.clear();
  while (true) {
    uint8_t ch = reader_.peek();
    if (ch == kcXMLTagO) {
      break;
    }
    reader_.read();
    str += ch;
    ++result;
  }
  return result;
}

// Decodes an XML tag and returns the string without the xml open
// and close delimiters via str
int32_t TXMLProtocol::readXMLTag(std::string &str, bool endTag) {
  int32_t result = 0, ret;
  uint8_t ch;
  str.clear();
  if ((ret = readXMLSyntaxChar(kcXMLTagO)) < 0) {
    return ret;
  }
  result += ret;
  if (endTag) {
    if ((ret = readXMLSyntaxChar(kcXMLSlash)) < 0) {
      return ret;
    }
    result += ret;
  }
  while (true) {
    ch = reader_.read();
    ++result;
    if (ch == kcXMLTagC) {
      break;
    }
    str += ch;
  }
  return result;
}

// Reads a sequence of characters, stopping at the first one that is not
// a valid numeric character.
int32_t TXMLProtocol::readXMLNumericChars(std::string &str) {
  uint32_t result = 0;
  str.clear();
  while (true) {
    uint8_t ch = reader_.peek();
    if (!isXMLNumeric(ch)) {
      break;
    }
    reader_.read();
    str += ch;
    ++result;
  }
  return result;
}

// Reads a sequence of characters and assembles them into a number,
// returning them via num
template <typename NumberType>
int32_t TXMLProtocol::readXMLInteger(NumberType &num) {
  int32_t result = 0, ret;
  std::string str;
  if ((ret = readXMLNumericChars(str)) < 0) {
    return ret;
  }
  result += ret;
  stringToInteger(str, num);
  return result;
}

int32_t TXMLProtocol::readMessageBegin(std::string& name,
                                       TMessageType& messageType,
                                       int32_t& seqid) {
  LOG(ERROR, __func__ << ": Not implemented in TXMLProtocol");
  assert(false);
  return -1;
}

int32_t TXMLProtocol::readMessageEnd() {
  LOG(ERROR, __func__ << ": Not implemented in TXMLProtocol");
  assert(false);
  return -1;
}

int32_t TXMLProtocol::readSandeshBegin(std::string& name) {
  std::string str;
  int32_t result = 0, ret;
  if ((ret = readXMLTag(str)) < 0) {
    LOG(ERROR, __func__ << ": FAILED");
    return ret;
  }
  result += ret;
  boost::char_separator<char> sep("=\" ");
  tokenizer tokens(str, sep);
  // Extract the field name
  tokenizer::iterator it = tokens.begin();
  name = *it;
  ++it;
  for (; it != tokens.end(); ++it) {
    if (*it == kXMLType) {
      ++it;
      if (kTypeNameSandesh != *it) {
        LOG(ERROR, __func__ << ": Expected " << kTypeNameSandesh <<
            "; got " << *it);
        return -1;
      }
    }
  }
  return result;
}

int32_t TXMLProtocol::readSandeshEnd() {
  std::string name;
  return readXMLTag(name, true);
}

int32_t TXMLProtocol::readStructBegin(std::string& name) {
  return readXMLTag(name);
}

int32_t TXMLProtocol::readStructEnd() {
  std::string name;
  return readXMLTag(name, true);
}

int32_t TXMLProtocol::readContainerElementBegin() {
  std::string name;
  return readXMLTag(name);
}

int32_t TXMLProtocol::readContainerElementEnd() {
  std::string name;
  return readXMLTag(name, true);
}

int32_t TXMLProtocol::readFieldBegin(std::string& name,
                                     TType& fieldType,
                                     int16_t& fieldId) {
  int32_t result = 0, ret;
  // Check if we hit the end of the list
  uint8_t ch = reader_.peek2();
  uint8_t ch1 = reader_.peek2();
  if (ch == kcXMLTagO && ch1 == kcXMLSlash) {
    fieldType = contrail::sandesh::protocol::T_STOP;
    return result;
  }
  std::string str;
  if ((ret = readXMLTag(str)) < 0) {
    LOG(ERROR, __func__ << ": FAILED");
    return ret;
  }
  result += ret;
  boost::char_separator<char> sep("=\" ");
  tokenizer tokens(str, sep);
  // Extract the field name
  tokenizer::iterator it = tokens.begin();
  name = *it;
  ++it;
  for (; it != tokens.end(); ++it) {
    if (*it == kXMLType) {
      ++it;
      fieldType = getTypeIDForTypeName(*it);
    }
    if (*it == kXMLIdentifier) {
      ++it;
      stringToInteger(*it, fieldId);
    }
  }
  return result;
}

int32_t TXMLProtocol::readFieldEnd() {
  string str;
  return readXMLTag(str, true);
}

int32_t TXMLProtocol::readMapBegin(TType& keyType,
                                   TType& valType,
                                   uint32_t& size) {
  int32_t result = 0, ret;
  std::string str;
  if ((ret = readXMLTag(str)) < 0) {
    LOG(ERROR, __func__ << ": FAILED");
    return ret;
  }
  result += ret;
  boost::char_separator<char> sep("=\" ");
  tokenizer tokens(str, sep);
  // Extract the field name
  tokenizer::iterator it = tokens.begin();
  if (*it != kTypeNameMap) {
    LOG(ERROR, __func__ << ": Expected \"" << kTypeNameMap <<
        "\"; got \"" << *it << "\"");
    return -1;
  }
  ++it;
  for (; it != tokens.end(); ++it) {
    if (*it == kXMLKey) {
      ++it;
      keyType = getTypeIDForTypeName(*it);
    }
    if (*it == kXMLValue) {
      ++it;
      valType = getTypeIDForTypeName(*it);
    }
    if (*it == kXMLSize) {
      ++it;
      stringToInteger(*it, size);
    }
  }
  return result;
}

int32_t TXMLProtocol::readMapEnd() {
  std::string str;
  return readXMLTag(str, true);
}

int32_t TXMLProtocol::readListBegin(TType& elemType,
                                    uint32_t& size) {
  int32_t result = 0, ret;
  std::string str;
  if ((ret = readXMLTag(str)) < 0) {
    LOG(ERROR, __func__ << ": FAILED");
    return ret;
  }
  result += ret;
  boost::char_separator<char> sep("=\" ");
  tokenizer tokens(str, sep);
  // Extract the field name
  tokenizer::iterator it = tokens.begin();
  if (*it != kTypeNameList) {
    LOG(ERROR, __func__ << ": Expected \"" << kTypeNameList <<
        "\"; got \"" << *it << "\"");
    return -1;
  }
  ++it;
  for (; it != tokens.end(); ++it) {
    if (*it == kXMLType) {
      ++it;
      elemType = getTypeIDForTypeName(*it);
    }
    if (*it == kXMLSize) {
      ++it;
      stringToInteger(*it, size);
    }
  }
  return result;
}

int32_t TXMLProtocol::readListEnd() {
  std::string str;
  return readXMLTag(str, true);
}

int32_t TXMLProtocol::readSetBegin(TType& elemType,
                                   uint32_t& size) {
  int32_t result = 0, ret;
  std::string str;
  if ((ret = readXMLTag(str)) < 0) {
    LOG(ERROR, __func__ << ": FAILED");
    return ret;
  }
  result += ret;
  boost::char_separator<char> sep("=\" ");
  tokenizer tokens(str, sep);
  // Extract the field name
  tokenizer::iterator it = tokens.begin();
  if (*it != kTypeNameSet) {
    LOG(ERROR, __func__ << ": Expected \"" << kTypeNameSet <<
        "\"; got \"" << *it << "\"");
    return -1;
  }
  ++it;
  for (; it != tokens.end(); ++it) {
    if (*it == kXMLType) {
      ++it;
      elemType = getTypeIDForTypeName(*it);
    }
    if (*it == kXMLSize) {
      ++it;
      stringToInteger(*it, size);
    }
  }
  return result;
}

int32_t TXMLProtocol::readSetEnd() {
  std::string str;
  return readXMLTag(str, true);
}

int32_t TXMLProtocol::readI16(int16_t& i16) {
  return readXMLInteger(i16);
}

int32_t TXMLProtocol::readI32(int32_t& i32) {
  return readXMLInteger(i32);
}

int32_t TXMLProtocol::readI64(int64_t& i64) {
  return readXMLInteger(i64);
}

int32_t TXMLProtocol::readU16(uint16_t& u16) {
  return readXMLInteger(u16);
}

int32_t TXMLProtocol::readU32(uint32_t& u32) {
  return readXMLInteger(u32);
}

int32_t TXMLProtocol::readU64(uint64_t& u64) {
  return readXMLInteger(u64);
}

int32_t TXMLProtocol::readIPV4(uint32_t& ip4) {
  return readXMLInteger(ip4);
}

int32_t TXMLProtocol::readDouble(double& dub) {
  return readXMLDouble(dub);
}

int32_t TXMLProtocol::readString(std::string &str) {
  readXMLString(str);
  unescapeXMLControlChars(str);
  return str.size(); 
}

int32_t TXMLProtocol::readBool(bool& value) {
  std::string str;
  int32_t result = 0, ret;
  if ((ret = readXMLString(str)) < 0) {
    return ret;
  }
  result += ret;
  if (str == kXMLBoolTrue) {
    value = true;
  } else if (str == kXMLBoolFalse) {
    value = false;
  } else {
    LOG(ERROR, __func__ << ": Expected \"" << kXMLBoolTrue <<
        "\" or \"" << kXMLBoolFalse << "\"; got \"" << str << "\"");
  }
  return result;
}

int32_t TXMLProtocol::readByte(int8_t& byte) {
  return readXMLInteger(byte);
}

int32_t TXMLProtocol::readBinary(std::string &str) {
  return readXMLString(str);
}

int32_t TXMLProtocol::readXML(std::string &str) {
  return readXMLCDATA(str);
}

int32_t TXMLProtocol::readUUID(boost::uuids::uuid &uuid) {
  int32_t ret;
  std::string str;
  if ((ret = readXMLString(str)) < 0) {
    return ret;
  }
  std::stringstream ss;
  ss << str;
  ss >> uuid;
  return ret;
}

}}} // contrail::sandesh::protocol
