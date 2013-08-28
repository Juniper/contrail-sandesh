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

#ifndef T_SANDESH_H
#define T_SANDESH_H

#include <algorithm>
#include <vector>
#include <utility>
#include <string>

#include "t_type.h"
#include "t_base_type.h"
#include "t_field.h"

// Forward declare that puppy
class t_program;

/**
 * A sandesh is a container for a set of member fields that has a name.
 *
 */
class t_sandesh : public t_type {
 public:
  typedef std::vector<t_field*> members_type;

  t_sandesh(t_program* program) :
    t_type(program),
    type_(NULL) {}

  t_sandesh(t_program* program, const std::string& name) :
    t_type(program, name),
    type_(NULL) {}

  void set_name(const std::string& name) {
    name_ = name;
  }

  void set_type(t_type* type) {
	type_ = type;
  }

  bool exist_opt_field() {
    members_type::const_iterator m_iter;
    for (m_iter = members_in_id_order_.begin(); m_iter != members_in_id_order_.end(); ++m_iter) {
       if ((*m_iter)->get_req() == t_field::T_OPTIONAL) {
           return true;
       }
    }
    return false;
  }

  bool append(t_field* elem) {
    members_.push_back(elem);

    typedef members_type::iterator iter_type;
    std::pair<iter_type, iter_type> bounds = std::equal_range(
            members_in_id_order_.begin(), members_in_id_order_.end(), elem, t_field::key_compare()
        );
    if (bounds.first != bounds.second) {
      return false;
    }
    members_in_id_order_.insert(bounds.second, elem);
    return true;
  }

  virtual std::string get_fingerprint_material() const {
    std::string rv = "{";
    members_type::const_iterator m_iter;
    for (m_iter = members_in_id_order_.begin(); m_iter != members_in_id_order_.end(); ++m_iter) {
      rv += (*m_iter)->get_fingerprint_material();
      rv += ";";
    }
    rv += "}";
    return rv;
  }

  virtual void generate_fingerprint() {
    t_type::generate_fingerprint();
    members_type::const_iterator m_iter;
    for (m_iter = members_in_id_order_.begin(); m_iter != members_in_id_order_.end(); ++m_iter) {
      (*m_iter)->get_type()->generate_fingerprint();
    }
  }

  const members_type& get_members() {
    return members_;
  }

  const members_type& get_sorted_members() {
    return members_in_id_order_;
  }

  const t_type* get_type() {
      return type_;
  }

  virtual bool has_key_annotation() const;

 private:

  t_type*      type_;
  members_type members_;
  members_type members_in_id_order_;
};

#endif
