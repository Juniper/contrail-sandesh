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

#ifndef T_BASE_TYPE_H
#define T_BASE_TYPE_H

#include <cstdlib>
#include "t_type.h"

/**
 * A thrift base type, which must be one of the defined enumerated types inside
 * this definition.
 *
 */
class t_base_type : public t_type {
 public:
  /**
   * Enumeration of thrift base types
   */
  enum t_base {
    TYPE_VOID,
    TYPE_STRING,
#ifdef SANDESH
    TYPE_STATIC_CONST_STRING,
    TYPE_SANDESH_SYSTEM,
    TYPE_SANDESH_REQUEST,
    TYPE_SANDESH_RESPONSE,
    TYPE_SANDESH_TRACE,
    TYPE_SANDESH_TRACE_OBJECT,
    TYPE_SANDESH_BUFFER,
    TYPE_SANDESH_UVE,
    TYPE_SANDESH_OBJECT,
    TYPE_SANDESH_FLOW,
#endif
    TYPE_BOOL,
    TYPE_BYTE,
    TYPE_I16,
    TYPE_I32,
    TYPE_I64,
#ifdef SANDESH
    TYPE_U16,
    TYPE_U32,
    TYPE_U64,
    TYPE_XML,
    TYPE_IPV4,
    TYPE_UUID,
#endif
    TYPE_DOUBLE
  };

  t_base_type(std::string name, t_base base) :
    t_type(name),
    base_(base),
    string_list_(false),
    binary_(false),
    string_enum_(false){}

  t_base get_base() const {
    return base_;
  }

  bool is_void() const {
    return base_ == TYPE_VOID;
  }

  bool is_string() const {
    return base_ == TYPE_STRING;
  }

  bool is_bool() const {
    return base_ == TYPE_BOOL;
  }

#ifdef SANDESH
  bool is_uuid_t() const {
    return base_ == TYPE_UUID;
  }
  bool is_static_const_string() const {
    return base_ == TYPE_STATIC_CONST_STRING;
  }

  bool is_sandesh_system() const {
    return base_ == TYPE_SANDESH_SYSTEM;
  }

  bool is_sandesh_request() const {
    return base_ == TYPE_SANDESH_REQUEST;
  }

  bool is_sandesh_response() const {
    return base_ == TYPE_SANDESH_RESPONSE;
  }

  bool is_sandesh_trace() const {
    return base_ == TYPE_SANDESH_TRACE;
  }

  bool is_sandesh_trace_object() const {
    return base_ == TYPE_SANDESH_TRACE_OBJECT;
  }

  bool is_sandesh_buffer() const {
    return base_ == TYPE_SANDESH_BUFFER;
  }

  bool is_sandesh_uve() const {
    return base_ == TYPE_SANDESH_UVE;
  }

  bool is_sandesh_object() const {
    return base_ == TYPE_SANDESH_OBJECT;
  }

  bool is_sandesh_flow() const {
    return base_ == TYPE_SANDESH_FLOW;
  }

  bool is_xml() const {
    return base_ == TYPE_XML;
  }

  bool is_integer() const {
    return base_ == TYPE_BOOL ||
           base_ == TYPE_BYTE ||
           base_ == TYPE_I16 ||
           base_ == TYPE_I32 ||
           base_ == TYPE_I64 ||
           base_ == TYPE_U16 ||
           base_ == TYPE_U32 ||
           base_ == TYPE_U64;
  }
#endif

  void set_string_list(bool val) {
    string_list_ = val;
  }

  bool is_string_list() const {
    return (base_ == TYPE_STRING) && string_list_;
  }

  void set_binary(bool val) {
    binary_ = val;
  }

  bool is_binary() const {
    return (base_ == TYPE_STRING) && binary_;
  }

  void set_string_enum(bool val) {
    string_enum_ = val;
  }

  bool is_string_enum() const {
    return base_ == TYPE_STRING && string_enum_;
  }

  void add_string_enum_val(std::string val) {
    string_enum_vals_.push_back(val);
  }

  const std::vector<std::string>& get_string_enum_vals() const {
    return string_enum_vals_;
  }

  bool is_base_type() const {
    return true;
  }

  virtual std::string get_fingerprint_material() const {
    std::string rv = t_base_name(base_);
    if (rv == "(unknown)") {
      throw "BUG: Can't get fingerprint material for this base type.";
    }
    return rv;
  }

  static std::string t_base_name(t_base tbase) {
    switch (tbase) {
      case TYPE_VOID   : return      "void"; break;
      case TYPE_STRING : return    "string"; break;
      case TYPE_BOOL   : return      "bool"; break;
      case TYPE_BYTE   : return      "byte"; break;
      case TYPE_I16    : return       "i16"; break;
      case TYPE_I32    : return       "i32"; break;
      case TYPE_I64    : return       "i64"; break;
      case TYPE_DOUBLE : return    "double"; break;
#ifdef SANDESH
      case TYPE_U16    : return       "u16"; break;
      case TYPE_U32    : return       "u32"; break;
      case TYPE_U64    : return       "u64"; break;
      case TYPE_IPV4   : return       "ipv4"; break;
      case TYPE_XML    : return       "xml"; break;
      case TYPE_UUID   : return       "uuid_t"; break;
      case TYPE_STATIC_CONST_STRING :
          return    "static const string"; break;
      case TYPE_SANDESH_SYSTEM   : return      "system"; break;
      case TYPE_SANDESH_REQUEST  : return     "request"; break;
      case TYPE_SANDESH_RESPONSE : return    "response"; break;
      case TYPE_SANDESH_TRACE    : return       "trace"; break;
      case TYPE_SANDESH_TRACE_OBJECT : return "traceobject"; break;
      case TYPE_SANDESH_BUFFER   : return      "buffer"; break;
      case TYPE_SANDESH_UVE      : return         "uve"; break;
      case TYPE_SANDESH_OBJECT   : return      "object"; break;
      case TYPE_SANDESH_FLOW     : return        "flow"; break;
#endif
      default          : return "(unknown)"; break;
    }
  }

 private:
  t_base base_;

  bool string_list_;
  bool binary_;
  bool string_enum_;
  std::vector<std::string> string_enum_vals_;
};

#endif
