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
 *
 * Copyright 2006-2017 The Apache Software Foundation.
 * https://github.com/apache/thrift
 */

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>

#include <stdlib.h>
#include <sys/stat.h>
#include <sstream>
#include "t_generator.h"
#include "platform.h"
using namespace std;


/**
 * HTML code generator
 *
 * mostly copy/pasting/tweaking from mcslee's work.
 */
class t_html_generator : public t_generator {
 public:
  t_html_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& option_string)
    : t_generator(program)
  {
    (void) parsed_options;
    (void) option_string;  
    out_dir_base_ = "gen-html";
  }

  void generate_program();
  void generate_xml();

  /**
   * Program-level generation functions
   */
  void generate_field_xml(t_field * tfl);
  void generate_struct_xml(const string& stname);
  void generate_sandesh_xml(t_sandesh* tsnd);
  void generate_typedef (t_typedef*  ttypedef) {}
  void generate_enum    (t_enum*     tenum) {}
  void generate_const   (t_const*    tconst) {}
  void generate_struct  (t_struct*   tstruct) {}
#ifdef SANDESH
  void generate_sandesh (t_sandesh*  tsandesh) {}
#endif
  void generate_service (t_service*  tservice) {}

  std::ofstream f_out_;
};

void t_html_generator::generate_struct_xml(const string &stname) {
  vector<t_struct*> objects = program_->get_objects();
  vector<t_struct*>::iterator o_iter;
  for (o_iter = objects.begin(); o_iter != objects.end(); ++o_iter) {
    t_struct* tstruct = *o_iter;
    const string & sname = tstruct->get_name();
    if (0 == sname.compare(stname)){
      f_out_ << "<" << sname << ">";
      vector<t_field*> members = tstruct->get_members();
      vector<t_field*>::iterator mem_iter = members.begin();
      for ( ; mem_iter != members.end(); mem_iter++) {
          generate_field_xml(*mem_iter);
      }        
      f_out_ << "</" << sname << ">";
    }
  }
}

void t_html_generator::generate_field_xml(t_field * tfl) {
    string fdname = tfl->get_name();
    t_type * fdtype = tfl->get_type();
    string fdtname = fdtype->get_name();

    if (fdtype->is_base_type()) {
      f_out_ << "<" << fdname << " type=\"" << fdtype->get_name() <<
          "\">0</" << fdname << ">";
    } else if (fdtype->is_list()) {
#if 0
        t_list * tl = (t_list*) fdtype;
        if (tl->get_elem_type()->is_struct()) {
          t_struct * ts = (t_struct *)tl->get_elem_type();
          f_out_ << "<td><table border=\"1\"><tr>" << endl;
          vector<t_field*> smem = ts->get_members();
          vector<t_field*>::iterator smem_iter = smem.begin();
          for ( ; smem_iter != smem.end(); smem_iter++) {
            f_out_ << "<th>" << (*smem_iter)->get_name() << "</th>" << endl;
          }
          f_out_ << "</tr><xsl:apply-templates select=\"" <<
              fdname << "/list/" << ts->get_name() << "\"/></table></td>" ;
        } else {
          f_out_ << "<td style=\"color:blue\"><xsl:value-of select=\"" <<
              fdname << "\"/></td>" << endl;
        }
#endif
    } else {
      f_out_ << "<" << fdname << " type=\"struct\">";
      generate_struct_xml(fdtype->get_name());
      f_out_ << "</" << fdname << ">";
    }
}

void t_html_generator::generate_sandesh_xml(t_sandesh* tsnd) {
  string name = tsnd->get_name();
  f_out_ << "<" << name << " type=\"sandesh\">" << endl;
  vector<t_field*> members = tsnd->get_members();
  vector<t_field*>::iterator mem_iter = members.begin();
  for ( ; mem_iter != members.end(); mem_iter++) {
      if ((*mem_iter)->get_auto_generated()) continue;
      generate_field_xml(*mem_iter);
  }
  f_out_ << "</" << name << ">" << endl;
}

void t_html_generator::generate_xml() {
  // Make output directory
  MKDIR(get_out_dir().c_str());
  string fname = get_out_dir() + program_->get_name() + ".xml";
  f_out_.open(fname.c_str());
  f_out_ <<
      "<?xml-stylesheet type=\"text/xsl\" href=\"/universal_parse.xsl\"?>" <<
      endl;
  f_out_ << "<" << program_->get_name() << " type=\"rlist\">" << endl;

#ifdef SANDESH
  if (!program_->get_sandeshs().empty()) {
    vector<t_sandesh*> sandeshs = program_->get_sandeshs();
    vector<t_sandesh*>::iterator s_iter;
    for (s_iter = sandeshs.begin(); s_iter != sandeshs.end(); ++s_iter) {
      if (static_cast<const t_base_type*>((*s_iter)->get_type())->get_base() ==
              t_base_type::TYPE_SANDESH_REQUEST) 
          generate_sandesh_xml(*s_iter);
    }
  }
#endif
  f_out_ << "</" << program_->get_name() << ">" << endl;

  f_out_.close();
}

/**
 * Prepares for file generation by opening up the necessary file output
 * stream.
 */
void t_html_generator::generate_program() {
  generate_xml();
}

THRIFT_REGISTER_GENERATOR(html, "HTML", "")
