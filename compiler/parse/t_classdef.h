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

#ifndef T_CLASSDEF_H
#define T_CLASSDEF_H

#include "t_function.h"
#include "t_struct.h"
#include <vector>

class t_program;

/**
 * A class def consists of a set of functions and structs.
 *
 */
class t_classdef : public t_type {
 public:
  t_classdef(t_program* program) :
    t_type(program) {};

  bool is_classdef() const {
    return true;
  }

  void add_function(t_function* func) {
    std::vector<t_function*>::const_iterator iter;
    for (iter = functions_.begin(); iter != functions_.end(); iter++) {
      if (func->get_name() == (*iter)->get_name()) {
        throw "Function " + func->get_name() + " is already defined";
      }
    }
    functions_.push_back(func);
  }

  const std::vector<t_function*>& get_functions() const {
    return functions_;
  }

  void add_structs(t_struct* structs) {
    structs_.push_back(structs);
  }

  const std::vector<t_struct*>& get_structs() const {
    return structs_;
  }

  virtual std::string get_fingerprint_material() const {
    // Classdef should never be used in fingerprints.
    throw "BUG: Can't get fingerprint material for class def.";
  }

 private:
  std::vector<t_function*> functions_;
  std::vector<t_struct*> structs_;
};

#endif
