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

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <map>
#include <utility>

#include <stdlib.h>
#include <sys/stat.h>
#include <sstream>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include "t_generator.h"
#include "platform.h"
using namespace std;

/**
 * DOC generator
 */
class t_doc_generator : public t_generator {
 public:
  t_doc_generator(
      t_program* program,
      const map<string, string>& parsed_options,
      const string& option_string)
    : t_generator(program)
  {
    (void) parsed_options;
    (void) option_string;
    out_dir_base_ = "gen-doc";
    escape_.clear();
    escape_['&']  = "&amp;";
    escape_['<']  = "&lt;";
    escape_['>']  = "&gt;";
    escape_['"']  = "&quot;";
    escape_['\''] = "&apos;";
  }

  struct doc_ftype {
    enum type {
      LOGS,
      LOGS_LEVEL_INVALID,
      LOGS_LEVEL_DEBUG,
      LOGS_LEVEL_INFO,
      LOGS_LEVEL_NOTICE,
      LOGS_LEVEL_WARN,
      LOGS_LEVEL_ERR,
      LOGS_LEVEL_CRIT,
      LOGS_LEVEL_ALERT,
      LOGS_LEVEL_EMERG,
      UVES,
      TRACES,
      INTROSPECT,
    };
  };

  // Sync with SandeshLevel from tools/sandesh/library/common/sandesh.sandesh
  struct sandesh_level {
    enum type {
      INVALID,
      DBG,
      INFO,
      NOTICE,
      WARN,
      ERR,
      CRIT,
      ALERT,
      EMERG,
    };
  };

  sandesh_level::type string_to_sandesh_level(const string &level);
  sandesh_level::type get_sandesh_level(t_sandesh *tsandesh);
  bool is_sandesh_type(t_sandesh *tsandesh, doc_ftype::type dtype);
  string get_doc_file_suffix(doc_ftype::type dtype);
  string get_doc_file_description(doc_ftype::type dtype);
  void generate_index();
  void generate_stats_schema_program();
  bool generate_sandesh_program(ofstream &f_out, doc_ftype::type dtype);
  void generate_const_enum_typedef_object_program();
  void generate_doc_type(ofstream &f_out);
  void generate_program();
  void generate_program_toc(ofstream &f_out, string fsuffix,
         doc_ftype::type dtype);
  void generate_program_toc_row(t_program* tprog, ofstream &f_out,
         string fsuffix, doc_ftype::type dtype);
  void generate_program_list(t_program* tprog, ofstream &f_out, string fsuffix,
         doc_ftype::type dtype);

  /**
   * Program-level generation functions
   */

  template <typename T>
  void generate_stat_table_schema(string, t_field*, T*, string);
  template <typename T>
  void generate_stat_schema_map(string, t_type*, t_type*, t_field*, T*, bool);
  template <typename T>
  void generate_stat_schema_list(string, t_type*, t_field*, T*, bool);
  t_struct* find_struct_with_name(string sname);
  template <typename T>
  void generate_stat_schema_struct(string, t_field*, t_struct*, T*, bool);
  template <typename T>
  void extract_tags(T*, map<string, vector<string> >&, vector<string>&, vector<string>&);
  void generate_stat_schema_struct_base_member(string, t_field*, string);
  void generate_stat_table_schema_header(string, string, string, string);
  string get_datatype_from_tfield(t_type* tfield);
  template <typename T>
  string get_type_of_member(string, t_field*, t_struct*, T*);
  template <typename T>
  void populate_stat_schema_map_key_value(t_field* tfield, t_type*, t_type*, T*);
  void generate_stat_schema_struct_members(string name, t_field*, t_struct*, vector<string>, map<string, vector<string> >);
  void is_indexed_or_suffixed_field(string, vector<string>, map<string, vector<string> >, string&, bool&);
  template <typename T>
  void generate_stat_schema_suffixes(string, t_field*, t_struct*, T*, map<string, vector<string> >&);
  template <typename T>
  void generate_stat_schema_toplevel_tags(T*, vector<string>&);
  void generate_stat_tables_schema(t_sandesh*);
  template <typename T>
  void generate_stat_tables_schema(T*, const vector<t_field*>, string, string);
  string get_display_name_from_comments(t_field*);
  template <typename T>
  void find_recursive_tags(T*, t_field*, string, string);
  template <typename T>
  void find_recursive_tags_struct(T*, t_field*, string, string, string);
  template <typename T>
  void find_recursive_tags_list(T*, t_field*, string, t_type*, string);
  template <typename T>
  void find_recursive_tags_map(T*, t_field*, string, t_type*, t_type*, string);
  string find_obj_table_name(const vector<t_field*>);
  void generate_typedef (t_typedef*  ttypedef) {}
  void generate_enum    (t_enum*     tenum) {}
  void generate_const   (t_const*    tconst) {}
  void generate_struct  (t_struct*   tstruct) {}
  void generate_service (t_service*  tservice) {}
  void generate_sandesh (t_sandesh*  tsandesh) {}

  void print_doc        (t_doc* tdoc, ofstream &f_out);
  int  print_type       (t_type* ttype, ofstream &f_out);
  void print_const_value(t_const_value* tvalue, ofstream &f_out);
  void print_sandesh    (t_sandesh* tdoc, ofstream &f_out);
  void print_sandesh_message(t_sandesh* tdoc, ofstream &f_out);
  void print_sandesh_message_table(t_sandesh* tdoc, ofstream &f_out);
  void print_doc_string (string doc, ofstream &f_out);

  ofstream f_index_out_;
  ofstream f_messages_out_;
  bool f_log_initialized_;
  bool f_log_invalid_initialized_;
  bool f_log_debug_initialized_;
  bool f_log_info_initialized_;
  bool f_log_notice_initialized_;
  bool f_log_warn_initialized_;
  bool f_log_error_initialized_;
  bool f_log_crit_initialized_;
  bool f_log_alert_initialized_;
  bool f_log_emerg_initialized_;
  bool f_uve_initialized_;
  bool f_trace_initialized_;
  bool f_introspect_initialized_;
  ofstream f_out_;
  ofstream f_stats_tables_;

 private:
  void generate_typedef (t_typedef*  ttypedef, ofstream &f_out);
  void generate_enum    (t_enum*     tenum, ofstream &f_out);
  void generate_const   (t_const*    tconst, ofstream &f_out);
  void generate_consts  (vector<t_const*> consts, ofstream &f_out);
  void generate_struct  (t_struct*   tstruct, ofstream &f_out);
  void generate_sandesh (t_sandesh*  tsandesh, ofstream &f_out);

  bool stat_table_created_;
  bool first_member_;
};

bool t_doc_generator::is_sandesh_type(t_sandesh *tsandesh,
                                      doc_ftype::type dtype) {
  t_base_type *tbtype((t_base_type *)tsandesh->get_type());
  switch (dtype) {
    case doc_ftype::LOGS:
      if (tbtype->is_sandesh_system() || tbtype->is_sandesh_object()) {
        return true;
      }
      return false;
    case doc_ftype::LOGS_LEVEL_INVALID:
      if ((tbtype->is_sandesh_system() || tbtype->is_sandesh_object()) &&
          get_sandesh_level(tsandesh) == sandesh_level::INVALID) {
        return true;
      }
      return false;
    case doc_ftype::LOGS_LEVEL_DEBUG:
      if ((tbtype->is_sandesh_system() || tbtype->is_sandesh_object()) &&
          get_sandesh_level(tsandesh) == sandesh_level::DBG) {
        return true;
      }
      return false;
    case doc_ftype::LOGS_LEVEL_INFO:
      if ((tbtype->is_sandesh_system() || tbtype->is_sandesh_object()) &&
          get_sandesh_level(tsandesh) == sandesh_level::INFO) {
        return true;
      }
      return false;
    case doc_ftype::LOGS_LEVEL_NOTICE:
      if ((tbtype->is_sandesh_system() || tbtype->is_sandesh_object()) &&
          get_sandesh_level(tsandesh) == sandesh_level::NOTICE) {
        return true;
      }
      return false;
    case doc_ftype::LOGS_LEVEL_WARN:
      if ((tbtype->is_sandesh_system() || tbtype->is_sandesh_object()) &&
          get_sandesh_level(tsandesh) == sandesh_level::WARN) {
        return true;
      }
      return false;
    case doc_ftype::LOGS_LEVEL_ERR:
      if ((tbtype->is_sandesh_system() || tbtype->is_sandesh_object()) &&
          get_sandesh_level(tsandesh) == sandesh_level::ERR) {
        return true;
      }
      return false;
    case doc_ftype::LOGS_LEVEL_CRIT:
      if ((tbtype->is_sandesh_system() || tbtype->is_sandesh_object()) &&
          get_sandesh_level(tsandesh) == sandesh_level::CRIT) {
        return true;
      }
      return false;
    case doc_ftype::LOGS_LEVEL_ALERT:
      if ((tbtype->is_sandesh_system() || tbtype->is_sandesh_object()) &&
          get_sandesh_level(tsandesh) == sandesh_level::ALERT) {
        return true;
      }
      return false;
    case doc_ftype::LOGS_LEVEL_EMERG:
      if ((tbtype->is_sandesh_system() || tbtype->is_sandesh_object()) &&
          get_sandesh_level(tsandesh) == sandesh_level::EMERG) {
        return true;
      }
      return false;
    case doc_ftype::UVES:
      if (tbtype->is_sandesh_uve()) {
        return true;
      }
      return false;
    case doc_ftype::TRACES:
      if (tbtype->is_sandesh_trace() || tbtype->is_sandesh_trace_object()) {
        return true;
      }
      return false;
    case doc_ftype::INTROSPECT:
      if (tbtype->is_sandesh_request() || tbtype->is_sandesh_response()) {
        return true;
      }
      return false;
    default:
      return false;
  }
  return false;
}

string t_doc_generator::get_doc_file_suffix(doc_ftype::type dtype) {
  switch (dtype) {
    case doc_ftype::LOGS:
      return "_logs";
    case doc_ftype::LOGS_LEVEL_INVALID:
      return "_logs.invalid";
    case doc_ftype::LOGS_LEVEL_DEBUG:
      return "_logs.debug";
    case doc_ftype::LOGS_LEVEL_INFO:
      return "_logs.info";
    case doc_ftype::LOGS_LEVEL_NOTICE:
      return "_logs.notice";
    case doc_ftype::LOGS_LEVEL_WARN:
      return "_logs.warn";
    case doc_ftype::LOGS_LEVEL_ERR:
      return "_logs.error";
    case doc_ftype::LOGS_LEVEL_CRIT:
      return "_logs.crit";
    case doc_ftype::LOGS_LEVEL_ALERT:
      return "_logs.alert";
    case doc_ftype::LOGS_LEVEL_EMERG:
      return "_logs.emerg";
    case doc_ftype::UVES:
      return "_uves";
    case doc_ftype::TRACES:
      return "_traces";
    case doc_ftype::INTROSPECT:
      return "_introspect";
    default:
      return "";
  }
}

string t_doc_generator::get_doc_file_description(doc_ftype::type dtype) {
  switch (dtype) {
    case doc_ftype::LOGS:
      return "all systemlog and objectlog";
    case doc_ftype::LOGS_LEVEL_INVALID:
      return "systemlog and objectlog with unknown severity";
    case doc_ftype::LOGS_LEVEL_DEBUG:
      return "debug systemlog and objectlog";
    case doc_ftype::LOGS_LEVEL_INFO:
      return "informational systemlog and objectlog";
    case doc_ftype::LOGS_LEVEL_NOTICE:
      return "notice systemlog and objectlog";
    case doc_ftype::LOGS_LEVEL_WARN:
      return "warning systemlog and objectlog";
    case doc_ftype::LOGS_LEVEL_ERR:
      return "error systemlog and objectlog";
    case doc_ftype::LOGS_LEVEL_CRIT:
      return "critical systemlog and objectlog";
    case doc_ftype::LOGS_LEVEL_ALERT:
      return "alert systemlog and objectlog";
    case doc_ftype::LOGS_LEVEL_EMERG:
      return "emergency systemlog and objectlog";
    case doc_ftype::UVES:
      return "UVE";
    case doc_ftype::TRACES:
      return "traces";
    case doc_ftype::INTROSPECT:
      return "introspect";
    default:
      return "";
  }
}

void t_doc_generator::generate_program_list(t_program* tprog, ofstream &f_out,
    string fsuffix, doc_ftype::type dtype) {
  string fname = tprog->get_name() + fsuffix + ".html";
  if (!tprog->get_sandeshs().empty()) {
    vector<t_sandesh*> sandeshs = tprog->get_sandeshs();
    vector<t_sandesh*>::iterator snh_iter;
    for (snh_iter = sandeshs.begin(); snh_iter != sandeshs.end(); ++snh_iter) {
      string name = get_sandesh_name(*snh_iter);
      if (is_sandesh_type(*snh_iter, dtype)) {
        f_out << "<tr><td><a href=\"" << fname << "#Snh_" << name << "\">" << name
          << "</a></td></tr>" << endl;
      }
    }
  }
}

/**
 * Emits the Table of Contents links at the top of the module's page
 */
void t_doc_generator::generate_program_toc(ofstream &f_out, string fsuffix,
    doc_ftype::type dtype) {
  f_out << "<table><tr><th>Module</th><th>Messages</th></tr>" << endl;
  generate_program_toc_row(program_, f_out, fsuffix, dtype);
  f_out << "</table>" << endl;
}

/**
 * Emits the Table of Contents links at the top of the module's page
 */
void t_doc_generator::generate_program_toc_row(t_program* tprog, ofstream &f_out,
    string fsuffix, doc_ftype::type dtype) {
  string fname = tprog->get_name() + fsuffix + ".html";
  f_out << "<tr>" << endl << "<td>" << tprog->get_name() << "</td><td>";
  if (!tprog->get_sandeshs().empty()) {
    vector<t_sandesh*> sandeshs = tprog->get_sandeshs();
    vector<t_sandesh*>::iterator snh_iter;
    for (snh_iter = sandeshs.begin(); snh_iter != sandeshs.end(); ++snh_iter) {
      string name = get_sandesh_name(*snh_iter);
      if (is_sandesh_type(*snh_iter, dtype)) {
        f_out << "<a href=\"" << fname << "#Snh_" << name << "\">" << name
          << "</a><br/>" << endl;
      }
    }
  }
  f_out << "</td>" << endl;
  f_out << "</tr>";
}

void t_doc_generator::generate_doc_type(ofstream &f_out) {
  f_out << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"" << endl;
  f_out << "    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" << endl;
  f_out << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl;
  f_out << "<head>" << endl;
  f_out << "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />" << endl;
  f_out << "<link href=\"/doc-style.css\" rel=\"stylesheet\" type=\"text/css\"/>"
   << endl;
}

void t_doc_generator::generate_stats_schema_program() {
  string fname = get_out_dir() + program_->get_name() + "_stats_tables.json";
  f_stats_tables_.open(fname.c_str());
  stat_table_created_ = false;
  first_member_ = true;
  f_stats_tables_ << "{\"_STAT_TABLES\":[" << endl;
  if (!program_->get_sandeshs().empty()) {
    // Generate sandeshs
    vector<t_sandesh*> sandeshs = program_->get_sandeshs();
    vector<t_sandesh*>::iterator snh_iter;
    for (snh_iter = sandeshs.begin(); snh_iter != sandeshs.end(); ++snh_iter) {
      generate_stat_tables_schema(*snh_iter);
    }
  }
  f_stats_tables_ << "]" << endl << "}" << endl;
}

bool t_doc_generator::generate_sandesh_program(ofstream &f_out,
    doc_ftype::type dtype) {
  string fsuffix = get_doc_file_suffix(dtype);
  string fname = get_out_dir() + program_->get_name() + fsuffix + ".html";
  f_out.open(fname.c_str());

  generate_doc_type(f_out);

  string mdesc(get_doc_file_description(dtype));
  f_out << "<title>Documentation for " << mdesc << " messages in " <<
    "module: " << program_->get_name() << "</title></head><body>" << endl <<
    "<h1>Documentation for " << mdesc << " messages in module: "
    << program_->get_name() << "</h1>" << endl;

  print_doc(program_, f_out);

  generate_program_toc(f_out, fsuffix, dtype);

  bool init = false;
  if (!program_->get_sandeshs().empty()) {
    f_out << "<hr/><h2 id=\"Messages\">Messages</h2>" << endl;
    vector<t_sandesh*> sandeshs = program_->get_sandeshs();
    vector<t_sandesh*>::iterator snh_iter;
    for (snh_iter = sandeshs.begin(); snh_iter != sandeshs.end(); ++snh_iter) {
      if (!is_sandesh_type(*snh_iter, dtype)) {
        continue;
      }
      init = true;
      sandesh_name_ = get_sandesh_name(*snh_iter);
      f_out << "<div class=\"definition\">";
      generate_sandesh(*snh_iter, f_out);
      f_out << "</div>";
    }
  }
  f_out << "</body></html>" << endl;
  f_out.close();

  string fname_list = get_out_dir() + program_->get_name() + fsuffix + ".list.html";
  ofstream f_out_list;
  f_out_list.open(fname_list.c_str());
  generate_program_list(program_, f_out_list, fsuffix, dtype);
  f_out_list.close();
  return init;
}

void t_doc_generator::generate_const_enum_typedef_object_program() {
  string fsuffix = "";
  string fname = get_out_dir() + program_->get_name() + fsuffix + ".html";
  f_out_.open(fname.c_str());

  generate_doc_type(f_out_);
  f_out_ << "<title>Documentation for constants, enums, types, and data structures in " <<
    "module: " << program_->get_name() << "</title></head><body>" << endl <<
    "<h1>Documentation for constants, enums, types, and data structures in module: "
    << program_->get_name() << "</h1>" << endl;

  print_doc(program_, f_out_);

  if (!program_->get_consts().empty()) {
    f_out_ << "<hr/><h2 id=\"Constants\">Constants</h2>" << endl;
    vector<t_const*> consts = program_->get_consts();
    f_out_ << "<table>";
    f_out_ << "<tr><th>Constant</th><th>Type</th><th>Value</th></tr>" << endl;
    generate_consts(consts, f_out_);
    f_out_ << "</table>";
  }

  if (!program_->get_enums().empty()) {
    f_out_ << "<hr/><h2 id=\"Enumerations\">Enumerations</h2>" << endl;
    // Generate enums
    vector<t_enum*> enums = program_->get_enums();
    vector<t_enum*>::iterator en_iter;
    for (en_iter = enums.begin(); en_iter != enums.end(); ++en_iter) {
      generate_enum(*en_iter, f_out_);
    }
  }

  if (!program_->get_typedefs().empty()) {
    f_out_ << "<hr/><h2 id=\"Typedefs\">Type declarations</h2>" << endl;
    // Generate typedefs
    vector<t_typedef*> typedefs = program_->get_typedefs();
    vector<t_typedef*>::iterator td_iter;
    for (td_iter = typedefs.begin(); td_iter != typedefs.end(); ++td_iter) {
      generate_typedef(*td_iter, f_out_);
    }
  }

  if (!program_->get_objects().empty()) {
    f_out_ << "<hr/><h2 id=\"Structs\">Data structures</h2>" << endl;
    // Generate structs and exceptions in declared order
    vector<t_struct*> objects = program_->get_objects();
    vector<t_struct*>::iterator o_iter;
    for (o_iter = objects.begin(); o_iter != objects.end(); ++o_iter) {
      generate_struct(*o_iter, f_out_);
    }
  }

  f_out_ << "</body></html>" << endl;
  f_out_.close();
}

/**
 * Emits the index.html file for the recursive set of Thrift programs
 */
void t_doc_generator::generate_index() {
  string index_fname = get_out_dir() + program_->get_name()  + "_index.html";
  f_index_out_.open(index_fname.c_str());
  f_index_out_ << "<html>" << endl;
  f_index_out_ << "<head>Documentation for " << program_->get_name() << "</head>" << endl;
  f_index_out_ << "<link href=\"/doc-style.css\" rel=\"stylesheet\" type=\"text/css\"/>"
   << endl;
  f_index_out_ << "<table><tr><th>Message Types</th></tr>" << endl;
  if (f_log_initialized_) {
    f_index_out_ << "<tr><td><a href=" << program_->get_name() <<
      get_doc_file_suffix(doc_ftype::LOGS) << ".html>All Logs</a></td></tr>" << endl;
  }
  if (f_log_emerg_initialized_) {
    f_index_out_ << "<tr><td><a href=" << program_->get_name() <<
      get_doc_file_suffix(doc_ftype::LOGS_LEVEL_EMERG) << ".html>Emergency Logs</a></td></tr>" << endl;
  }
  if (f_log_alert_initialized_) {
    f_index_out_ << "<tr><td><a href=" << program_->get_name() <<
      get_doc_file_suffix(doc_ftype::LOGS_LEVEL_ALERT) << ".html>Alert Logs</a></td></tr>" << endl;
  }
  if (f_log_crit_initialized_) {
    f_index_out_ << "<tr><td><a href=" << program_->get_name() <<
      get_doc_file_suffix(doc_ftype::LOGS_LEVEL_CRIT) << ".html>Critical Logs</a></td></tr>" << endl;
  }
  if (f_log_error_initialized_) {
    f_index_out_ << "<tr><td><a href=" << program_->get_name() <<
      get_doc_file_suffix(doc_ftype::LOGS_LEVEL_ERR) << ".html>Error Logs</a></td></tr>" << endl;
  }
  if (f_log_warn_initialized_) {
    f_index_out_ << "<tr><td><a href=" << program_->get_name() <<
      get_doc_file_suffix(doc_ftype::LOGS_LEVEL_WARN) << ".html>Warning Logs</a></td></tr>" << endl;
  }
  if (f_log_notice_initialized_) {
    f_index_out_ << "<tr><td><a href=" << program_->get_name() <<
      get_doc_file_suffix(doc_ftype::LOGS_LEVEL_NOTICE) << ".html>Notice Logs</a></td></tr>" << endl;
  }
  if (f_log_info_initialized_) {
    f_index_out_ << "<tr><td><a href=" << program_->get_name() <<
      get_doc_file_suffix(doc_ftype::LOGS_LEVEL_INFO) << ".html>Informational Logs</a></td></tr>" << endl;
  }
  if (f_log_debug_initialized_) {
    f_index_out_ << "<tr><td><a href=" << program_->get_name() <<
      get_doc_file_suffix(doc_ftype::LOGS_LEVEL_DEBUG) << ".html>Debugging Logs</a></td></tr>" << endl;
  }
  if (f_log_invalid_initialized_) {
    f_index_out_ << "<tr><td><a href=" << program_->get_name() <<
      get_doc_file_suffix(doc_ftype::LOGS_LEVEL_INVALID) << ".html>Unknown severity logs</a></td></tr>" << endl;
  }
  if (f_uve_initialized_) {
    f_index_out_ << "<tr><td><a href=" << program_->get_name() << "_uves.html>UVEs</a></td></tr>" << endl;
  }
  if (f_trace_initialized_) {
    f_index_out_ << "<tr><td><a href=" << program_->get_name() << "_traces.html>Traces</a></td></tr>" << endl;
  }
  if (f_introspect_initialized_) {
    f_index_out_ << "<tr><td><a href=" << program_->get_name() << "_introspect.html>Request-Response</a></td></tr>" << endl;
  }
  f_index_out_ << "</table>" << endl;
  f_index_out_ << "</html>" << endl;
  f_index_out_.close();
}

/**
 * Prepares for file generation by opening up the necessary file output
 * stream.
 */
void t_doc_generator::generate_program() {
  // Make output directory
  MKDIR(get_out_dir().c_str());
  generate_stats_schema_program();
  f_log_initialized_ = generate_sandesh_program(f_messages_out_, doc_ftype::LOGS);
  f_log_invalid_initialized_ = generate_sandesh_program(f_messages_out_, doc_ftype::LOGS_LEVEL_INVALID);
  f_log_debug_initialized_ = generate_sandesh_program(f_messages_out_, doc_ftype::LOGS_LEVEL_DEBUG);
  f_log_info_initialized_ = generate_sandesh_program(f_messages_out_, doc_ftype::LOGS_LEVEL_INFO);
  f_log_notice_initialized_ = generate_sandesh_program(f_messages_out_, doc_ftype::LOGS_LEVEL_NOTICE);
  f_log_warn_initialized_ = generate_sandesh_program(f_messages_out_, doc_ftype::LOGS_LEVEL_WARN);
  f_log_error_initialized_ = generate_sandesh_program(f_messages_out_, doc_ftype::LOGS_LEVEL_ERR);
  f_log_crit_initialized_ = generate_sandesh_program(f_messages_out_, doc_ftype::LOGS_LEVEL_CRIT);
  f_log_alert_initialized_ = generate_sandesh_program(f_messages_out_, doc_ftype::LOGS_LEVEL_ALERT);
  f_log_emerg_initialized_ = generate_sandesh_program(f_messages_out_, doc_ftype::LOGS_LEVEL_EMERG);
  f_uve_initialized_ = generate_sandesh_program(f_messages_out_, doc_ftype::UVES);
  f_trace_initialized_ = generate_sandesh_program(f_messages_out_, doc_ftype::TRACES);
  f_introspect_initialized_ = generate_sandesh_program(f_messages_out_, doc_ftype::INTROSPECT);
  generate_index();
  generate_const_enum_typedef_object_program();
}

void t_doc_generator::print_doc_string(string doc, ofstream &f_out) {
  size_t index;
  while ((index = doc.find_first_of("\r\n")) != string::npos) {
    if (index == 0) {
      f_out << "<p/>" << endl;
    } else {
      f_out << doc.substr(0, index) << endl;
    }
    if (index + 1 < doc.size() && doc.at(index) != doc.at(index + 1) &&
        (doc.at(index + 1) == '\r' || doc.at(index + 1) == '\n')) {
      index++;
    }
    doc = doc.substr(index + 1);
  }
  f_out << doc << "<br/>";
}

/**
 * If the provided documentable object has documentation attached, this
 * will emit it to the output stream in HTML format.
 */
void t_doc_generator::print_doc(t_doc* tdoc, ofstream &f_out) {
  if (tdoc->has_doc()) {
    string doc = tdoc->get_doc();
    print_doc_string(doc, f_out);
  }
}

void t_doc_generator::print_sandesh_message(t_sandesh* tsandesh, ofstream &f_out) {
  bool first = true;
  // Print the message
  vector<t_field*> members = tsandesh->get_members();
  BOOST_FOREACH(t_field *tfield, members) {
    if (tfield->get_auto_generated()) {
      continue;
    }
    if (first) {
      f_out << "<tr><th>Message</th><td>";
    }
    t_type *type = tfield->get_type();
    if (type->is_static_const_string()) {
      t_const_value* default_val = tfield->get_value();
      if (!first) {
        f_out << " ";
      }
      f_out << get_escaped_string(default_val);
    } else {
      f_out << "<code>";
      if (!first) {
        f_out << " ";
      }
      f_out << tfield->get_name() << "</code>";
    }
    first = false;
  }
  if (!first) {
    f_out << "</td></tr>" << endl;
  }
}

void t_doc_generator::print_sandesh_message_table(t_sandesh* tsandesh, ofstream &f_out) {
  f_out << "<table>";
  print_sandesh_message(tsandesh, f_out);
  f_out << "</table><br/>" << endl;
}

t_doc_generator::sandesh_level::type t_doc_generator::string_to_sandesh_level(
    const string &ilevel) {
  string level(ilevel);
  // Trim and convert to lower case
  boost::algorithm::trim(level);
  if (level.empty()) {
    return t_doc_generator::sandesh_level::INVALID;
  }
  boost::algorithm::to_lower(level);
  if (level == "debug") {
    return t_doc_generator::sandesh_level::DBG;
  }
  if (level == "info") {
    return t_doc_generator::sandesh_level::INFO;
  }
  if (level == "notice") {
    return t_doc_generator::sandesh_level::NOTICE;
  }
  if (level == "warn") {
    return t_doc_generator::sandesh_level::WARN;
  }
  if (level == "error") {
    return t_doc_generator::sandesh_level::ERR;
  }
  if (level == "critical") {
    return t_doc_generator::sandesh_level::CRIT;
  }
  if (level == "alert") {
    return t_doc_generator::sandesh_level::ALERT;
  }
  if (level == "emerg") {
    return t_doc_generator::sandesh_level::EMERG;
  }
  return t_doc_generator::sandesh_level::INVALID;
}

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
t_doc_generator::sandesh_level::type t_doc_generator::get_sandesh_level(
    t_sandesh* tsandesh) {
  if (tsandesh->has_doc()) {
    string doc = tsandesh->get_doc();
    size_t index;
    if ((index = doc.find_first_of("@")) != string::npos) {
      // Skip leading documentation
      string fdoc(doc.substr(index + 1));
      // Extract tokens beginning with @
      boost::char_separator<char> fsep("@");
      tokenizer ftokens(fdoc, fsep);
      BOOST_FOREACH(const string &f, ftokens) {
        // Has 2 parts, the first ending with ':' or ' ' or '\t' is
        // the type and the next is the content
        size_t lindex;
        if ((lindex = f.find_first_of(": \t")) != string::npos) {
          string type(f.substr(0, lindex));
          if (lindex + 1 < f.size() && f.at(lindex) != f.at(lindex + 1) &&
              (f.at(lindex + 1) == ':' || f.at(lindex + 1) == '\t' ||
               f.at(lindex + 1) == ' ')) {
            lindex++;
          }
          string content(f.substr(lindex + 1));
          if (type == "severity") {
            return string_to_sandesh_level(content);
          }
        }
      }
    }
  }
  return sandesh_level::INVALID;
}

/**
 * If the provided documentable sandesh has documentation attached, this
 * will emit it to the output stream in HTML format and print the @variables
 * in a table format.
 */
void t_doc_generator::print_sandesh(t_sandesh* tsandesh, ofstream &f_out) {
  if (tsandesh->has_doc()) {
    bool table = false;
    string doc = tsandesh->get_doc();
    size_t index;
    if ((index = doc.find_first_of("@")) != string::npos) {
      // Print leading documentation
      f_out << doc.substr(0, index) << "<p/>" << endl;
      string fdoc(doc.substr(index + 1));
      if (!table) {
        f_out << "<table>" << endl;
        print_sandesh_message(tsandesh, f_out);
        table = true;
      }
      // Extract tokens beginning with @
      boost::char_separator<char> fsep("@");
      tokenizer ftokens(fdoc, fsep);
      BOOST_FOREACH(const string &f, ftokens) {
        // Has 2 parts, the first ending with ':' or ' ' or '\t' is
        // the type and the next is the content
        size_t lindex;
        if ((lindex = f.find_first_of(": \t")) != string::npos) {
          string type(f.substr(0, lindex));
          type.at(0) = toupper(type.at(0));
          f_out << "<tr><th>" << type << "</th>";
          if (lindex + 1 < f.size() && f.at(lindex) != f.at(lindex + 1) &&
              (f.at(lindex + 1) == ':' || f.at(lindex + 1) == '\t' ||
               f.at(lindex + 1) == ' ')) {
            lindex++;
          }
          string content(f.substr(lindex + 1));
          f_out << "<td>" << content << "</td></tr>" << endl;
        } else {
          f_out << "<td>" << f << "</td>" << endl;
        }
      }
    } else {
      f_out << doc << "<p/>" << endl;
      print_sandesh_message_table(tsandesh, f_out);
    }
    if (table) {
      f_out << "</table><br/>" << endl;
    }
  } else {
    print_sandesh_message_table(tsandesh, f_out);
  }
}

/**
 * Prints out the provided type in HTML
 */
int t_doc_generator::print_type(t_type* ttype, ofstream &f_out) {
  int len = 0;
  f_out << "<code>";
  if (ttype->is_container()) {
    if (ttype->is_list()) {
      f_out << "list&lt;";
      len = 6 + print_type(((t_list*)ttype)->get_elem_type(), f_out);
      f_out << "&gt;";
    } else if (ttype->is_set()) {
      f_out << "set&lt;";
      len = 5 + print_type(((t_set*)ttype)->get_elem_type(), f_out);
      f_out << "&gt;";
    } else if (ttype->is_map()) {
      f_out << "map&lt;";
      len = 5 + print_type(((t_map*)ttype)->get_key_type(), f_out);
      f_out << ", ";
      len += print_type(((t_map*)ttype)->get_val_type(), f_out);
      f_out << "&gt;";
    }
  } else if (ttype->is_base_type()) {
    f_out << (((t_base_type*)ttype)->is_binary() ? "binary" : ttype->get_name());
    len = ttype->get_name().size();
  } else {
    string prog_name = ttype->get_program()->get_name();
    string type_name = ttype->get_name();
    f_out << "<a href=\"" << prog_name << ".html#";
    if (ttype->is_typedef()) {
      f_out << "Typedef_";
    } else if (ttype->is_struct() || ttype->is_xception()) {
      f_out << "Struct_";
    } else if (ttype->is_enum()) {
      f_out << "Enum_";
    } else if (ttype->is_service()) {
      f_out << "Svc_";
    }
    f_out << type_name << "\">";
    len = type_name.size();
    if (ttype->get_program() != program_) {
      f_out << prog_name << ".";
      len += prog_name.size() + 1;
    }
    f_out << type_name << "</a>";
  }
  f_out << "</code>";
  return len;
}

/**
 * Prints out an HTML representation of the provided constant value
 */
void t_doc_generator::print_const_value(t_const_value* tvalue, ofstream &f_out) {
  bool first = true;
  switch (tvalue->get_type()) {
  case t_const_value::CV_INTEGER:
    f_out << tvalue->get_integer();
    break;
  case t_const_value::CV_DOUBLE:
    f_out << tvalue->get_double();
    break;
  case t_const_value::CV_STRING:
    f_out << '"' << get_escaped_string(tvalue) << '"';
    break;
  case t_const_value::CV_MAP:
    {
      f_out << "{ ";
      map<t_const_value*, t_const_value*> map_elems = tvalue->get_map();
      map<t_const_value*, t_const_value*>::iterator map_iter;
      for (map_iter = map_elems.begin(); map_iter != map_elems.end(); map_iter++) {
        if (!first) {
          f_out << ", ";
        }
        first = false;
        print_const_value(map_iter->first, f_out);
        f_out << " = ";
        print_const_value(map_iter->second, f_out);
      }
      f_out << " }";
    }
    break;
  case t_const_value::CV_LIST:
    {
      f_out << "{ ";
      vector<t_const_value*> list_elems = tvalue->get_list();;
      vector<t_const_value*>::iterator list_iter;
      for (list_iter = list_elems.begin(); list_iter != list_elems.end(); list_iter++) {
        if (!first) {
          f_out << ", ";
        }
        first = false;
        print_const_value(*list_iter, f_out);
      }
      f_out << " }";
    }
    break;
  default:
    f_out << "UNKNOWN";
    break;
  }
}

/**
 * Generates a typedef.
 *
 * @param ttypedef The type definition
 */
void t_doc_generator::generate_typedef(t_typedef* ttypedef, ofstream &f_out) {
  string name = ttypedef->get_name();
  f_out << "<div class=\"definition\">";
  f_out << "<h3 id=\"Typedef_" << name << "\">Typedef: " << name
   << "</h3>" << endl;
  f_out << "<p><strong>Base type:</strong>&nbsp;";
  print_type(ttypedef->get_type(), f_out);
  f_out << "</p>" << endl;
  print_doc(ttypedef, f_out);
  f_out << "</div>" << endl;
}

/**
 * Generates code for an enumerated type.
 *
 * @param tenum The enumeration
 */
void t_doc_generator::generate_enum(t_enum* tenum, ofstream &f_out) {
  string name = tenum->get_name();
  f_out << "<div class=\"definition\">";
  f_out << "<h3 id=\"Enum_" << name << "\">Enumeration: " << name
   << "</h3>" << endl;
  print_doc(tenum, f_out);
  vector<t_enum_value*> values = tenum->get_constants();
  vector<t_enum_value*>::iterator val_iter;
  f_out << "<br/><table>" << endl;
  for (val_iter = values.begin(); val_iter != values.end(); ++val_iter) {
    f_out << "<tr><td><code>";
    f_out << (*val_iter)->get_name();
    f_out << "</code></td><td><code>";
    f_out << (*val_iter)->get_value();
    f_out << "</code></td></tr>" << endl;
  }
  f_out << "</table></div>" << endl;
}

/**
 * Generates a constant value
 */
void t_doc_generator::generate_const(t_const* tconst, ofstream &f_out) {
  string name = tconst->get_name();
  f_out << "<tr id=\"Const_" << name << "\"><td><code>" << name
   << "</code></td><td><code>";
  print_type(tconst->get_type(), f_out);
  f_out << "</code></td><td><code>";
  print_const_value(tconst->get_value(), f_out);
  f_out << "</code></td></tr>";
  if (tconst->has_doc()) {
    f_out << "<tr><td colspan=\"3\"><blockquote>";
    print_doc(tconst, f_out);
    f_out << "</blockquote></td></tr>";
  }
}

void t_doc_generator::generate_consts(vector<t_const*> consts, ofstream &f_out) {
  vector<t_const*>::iterator c_iter;
  for (c_iter = consts.begin(); c_iter != consts.end(); ++c_iter) {
    generate_const(*c_iter, f_out);
  }
}

void t_doc_generator::generate_stat_table_schema_header(string type, string attr, string table_name, string display_name) {
    if (stat_table_created_ == false) {
        stat_table_created_ = true;
        f_stats_tables_ << "{\n\"stat_type\":\"" << type << "\"," << endl;
    } else {
        f_stats_tables_ << ",\n{\n\"stat_type\":\"" << type << "\"," << endl;
    }
    f_stats_tables_ << "\"stat_attr\":\"" << attr << "\"," << endl;
    f_stats_tables_ << "\"display_name\":\"" << display_name << "\"," << endl;
    f_stats_tables_ << "\"obj_table\": \"" << table_name << "\",\n";
    f_stats_tables_ << "\"attributes\":[" << endl;
    first_member_ = true;
}

template <typename T>
string t_doc_generator::get_type_of_member(string name, t_field* tfield, t_struct* cstruct, T* tstruct) {
    string datatype;
    vector<t_field*>::const_iterator m_iter;
    if (!boost::starts_with(name, ".")) {
        //find the member in top level structure
        const vector<t_field*>& tmembers = tstruct->get_members();
        for (m_iter = tmembers.begin(); m_iter != tmembers.end(); ++m_iter) {
            map<string, string>::iterator jt;
            if (name == (*m_iter)->get_name()) {
                return get_datatype_from_tfield((*m_iter)->get_type());
            }
        }
    } else if (name.compare(".__key") == 0) {
        if (tfield->get_type()->is_map()) {
            t_type* keytype= ((t_map*)(tfield->get_type()))->get_key_type();
            return get_datatype_from_tfield(keytype);
        }
    } else {
        const vector<t_field*>& tmembers = cstruct->get_members();
        for (m_iter = tmembers.begin(); m_iter != tmembers.end(); ++m_iter) {
            map<string, string>::iterator jt;
            if (name.substr(1) == (*m_iter)->get_name()) {
                return get_datatype_from_tfield((*m_iter)->get_type());
            }
        }
    }
    return datatype;
}

template <typename T>
void t_doc_generator::generate_stat_schema_toplevel_tags(T* tstruct, vector<string>& top_level_tags) {
    BOOST_FOREACH(const string &tag, top_level_tags) {
        //find the member in top level structure
        const vector<t_field*>& tmembers = tstruct->get_members();
        vector<t_field*>::const_iterator m_iter;
        for (m_iter = tmembers.begin(); m_iter != tmembers.end(); ++m_iter) {
            map<string, string>::iterator jt;
            if (tag == (*m_iter)->get_name()) {
                string fname;
                generate_stat_schema_struct_base_member(fname, *m_iter, "true");
                break;
            }
        }
    }
}

template <typename T>
void t_doc_generator::generate_stat_schema_suffixes(string prefix, t_field* tfield, t_struct* cstruct, T* tstruct, map<string, vector<string> >& suffixes) {
    map<string, vector<string> >::iterator it;
    for (it=suffixes.begin(); it!=suffixes.end(); ++it) {
        string fname = it->first;
        string datatype = get_type_of_member(fname, tfield, cstruct, tstruct);
        if (boost::starts_with(fname, ".")) {
            fname = tfield->get_name() + fname;
        }
        if (!prefix.empty()) {
            fname = prefix + "." + fname;
        }
        string index = "true";
        string suffix;
        vector<string>::iterator name_it;
        for (name_it = it->second.begin(); name_it!=it->second.end(); ++name_it) {
            string suffix_name = tfield->get_name() + *name_it;
            if (boost::starts_with(suffix_name, ".")) {
                suffix_name = tfield->get_name() + suffix_name;
            }
            if (!prefix.empty()) {
                suffix_name = prefix + "." + suffix_name;
            }
            if(name_it == it->second.begin()) {
                suffix = suffix + "\"" + suffix_name + "\"";
            } else {
                suffix = suffix + ", \"" + suffix_name + "\"";
            }
        }
        f_stats_tables_ << ",\n\t{\"name\":\"" << fname << "\",\"datatype\":\"" << datatype << "\",\"index\":" << index << ",\"suffixes\":[" << suffix << "]}";
    }
}

void t_doc_generator::is_indexed_or_suffixed_field(string fname, vector<string> tags, map<string, vector<string> > suffixes, string& index, bool& is_suffixed_field) {
    BOOST_FOREACH(const string &tag, tags) {
        string field_name = string(".") + fname;
        string trimmed_tag = tag;
        boost::trim(trimmed_tag);
        if (trimmed_tag == field_name) {
            index = "true";
            return;
        }
    }
    for(map<string, vector<string> >::iterator it = suffixes.begin(); it != suffixes.end(); ++it) {
        string field_name = string(".") + fname;
        string trimmed_tag = it->first;
        boost::trim(trimmed_tag);
        if (trimmed_tag == field_name) {
            is_suffixed_field = true;
            return;
        }
    }
}

void t_doc_generator::generate_stat_schema_struct_members(string prefix, t_field* tfield, t_struct* tstruct, vector<string> tags, map<string, vector<string> > suffixes) {
    const vector<t_field*> members = tstruct->get_members();
    vector<t_field*>::const_iterator m_iter;
    string name = tfield->get_name();
    if(!prefix.empty()) {
        prefix = prefix + "." + name;
    } else {
        prefix = name;
    }
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        string index = "false";
        bool is_suffixed_field = false;
        is_indexed_or_suffixed_field((*m_iter)->get_name(), tags, suffixes, index, is_suffixed_field);
        if (is_suffixed_field) {
            continue;
        }
        t_type* mtype = (*m_iter)->get_type();
        if(mtype->is_base_type()) {
            generate_stat_schema_struct_base_member(prefix, *m_iter, index);
        } else if (mtype->is_struct()) {
            string mtype_name = mtype->get_name();
            t_struct *cstruct = find_struct_with_name(mtype_name);
            if (cstruct) {
                generate_stat_schema_struct(prefix, (*m_iter), cstruct, tstruct, false);
            }
        } else if (mtype->is_list()) {
            t_type* etype= ((t_list*)mtype)->get_elem_type();
            generate_stat_schema_list(prefix, etype, (*m_iter), tstruct, false);
        } else if (mtype->is_map()) {
            t_type* valtype= ((t_map*)((*m_iter)->get_type()))->get_val_type();
            t_type* keytype= ((t_map*)((*m_iter)->get_type()))->get_key_type();
            generate_stat_schema_map(prefix, keytype, valtype, (*m_iter), tstruct, false);
        }
    }
}

string t_doc_generator::get_datatype_from_tfield(t_type* ttype) {
    string datatype = ttype->get_name();
    if ((datatype == "u32") || (datatype == "u16") || (datatype == "u64"))
        datatype = "int";
    return datatype;
}

template <typename T>
void t_doc_generator::generate_stat_schema_map(string prefix, t_type* keytype, t_type* valtype, t_field* tfield, T* tstruct, bool is_top_level) {
    map<string, vector<string> > suffixes;
    vector<string> top_level_tags;
    vector<string> member_tags;
    bool is_empty_tag = false;
    map<string, string>::iterator jt;
    string tname = tfield->get_name();
    jt = tfield->annotations_.find("tags");
    if (jt == tfield->annotations_.end())
        is_empty_tag = true;
    if (!is_top_level && !is_empty_tag) {
	return;
    }
    if(!prefix.empty()) {
        tname = prefix + "." + tname;
    }
    if (is_empty_tag == false) {
        extract_tags(tfield, suffixes, top_level_tags, member_tags);
    }
    string name = tname + string(".__key");
    string datatype = get_datatype_from_tfield(keytype);
    string index = "false";
    bool is_suffixed_field = false;
    is_indexed_or_suffixed_field(string("__key"), member_tags, suffixes, index, is_suffixed_field);
    if (!is_suffixed_field) {
        f_stats_tables_ << "\t{\"name\":\"" << name << "\",\"datatype\":\"" << datatype << "\",\"index\":" << index << "},\n";
    }
    if(valtype->is_base_type()) {
        name = tname + string(".__value");
        datatype = get_datatype_from_tfield(valtype);
        index = "false";
        BOOST_FOREACH(const string &f, member_tags) {
            if (f == string(".__value")) {
                index = "true";
            }
        }
        f_stats_tables_ << "\t{\"name\":\"" << name << "\",\"datatype\":\"" << datatype << "\",\"index\":" << index << "}";
    }

    string empty_prefix;
    if (valtype->is_struct()) {
        string sname = valtype->get_name();
        t_struct *cstruct = find_struct_with_name(sname);
        if (cstruct) {
	    if(!is_empty_tag) {
                generate_stat_schema_struct_members(empty_prefix, tfield, cstruct, member_tags, suffixes);
	    }
            generate_stat_schema_toplevel_tags(tstruct, top_level_tags);
            generate_stat_schema_suffixes(empty_prefix, tfield, cstruct, tstruct, suffixes);
        }
    } else {
        generate_stat_schema_toplevel_tags(tstruct, top_level_tags);
        generate_stat_schema_suffixes(empty_prefix, tfield, NULL, tstruct, suffixes);
    }
}

void t_doc_generator::generate_stat_schema_struct_base_member(string prefix, t_field* tfield, string index) {
    string fname = tfield->get_name();
    if (!prefix.empty()) {
        fname = prefix+ "." + fname;
    }
    string datatype = get_datatype_from_tfield(tfield->get_type());
    if (!first_member_) {
        f_stats_tables_ << ",\n\t{\"name\":\"" << fname << "\",\"datatype\":\"" << datatype << "\",\"index\":" << index << "}";
    } else {
        first_member_ = false;
        f_stats_tables_ << "\t{\"name\":\"" << fname << "\",\"datatype\":\"" << datatype << "\",\"index\":" << index << "}";
    }
}

template <typename T>
void t_doc_generator::extract_tags(T* tfield, map<string, vector<string> > &suffixes, vector<string> &top_level_tags, vector<string> &member_tags) {
    boost::char_separator<char> fsep(",");
    tokenizer tags(tfield->annotations_["tags"], fsep);
    BOOST_FOREACH(const string &tag, tags) {
        size_t index = tag.find(':');
        if(index != string::npos) {
            string fname = tag.substr(0, index);
            boost::trim(fname);
            string suffix_name = tag.substr(index+1);
            boost::trim(suffix_name);
            if(suffixes.find(fname) == suffixes.end()) {
                vector<string> new_suffix;
                new_suffix.push_back(suffix_name);
                suffixes.insert(make_pair(fname, new_suffix));
            } else {
                vector<string> old_suffixes = suffixes[fname];
                old_suffixes.push_back(suffix_name);
                suffixes[fname] = old_suffixes;
            }
        } else {
            string trimmed_tag = tag;
            boost::trim(trimmed_tag);
            if (!boost::starts_with(trimmed_tag, ".")) {
                top_level_tags.push_back(trimmed_tag);
            } else {
                member_tags.push_back(trimmed_tag);
            }
        }
    }
}

template <typename T>
void t_doc_generator::generate_stat_schema_struct(string prefix, t_field* tfield, t_struct* cstruct, T* tstruct, bool is_top_level) {
    map<string, vector<string> > suffixes;
    vector<string> top_level_tags;
    vector<string> member_tags;
    bool is_empty_tag = false;
    if(is_top_level) {
        extract_tags(tfield, suffixes, top_level_tags, member_tags);
    } else {
        map<string, string>::iterator jt = cstruct->annotations_.find("tags");
        if (jt == tfield->annotations_.end()) {
            is_empty_tag = true;
        } else {
            return;
        }
    }
    if(is_empty_tag || is_top_level)
        generate_stat_schema_struct_members(prefix, tfield, cstruct, member_tags, suffixes);
    generate_stat_schema_toplevel_tags(tstruct, top_level_tags);
    generate_stat_schema_suffixes(prefix, tfield, cstruct, tstruct, suffixes);
}

template <typename T>
void t_doc_generator::generate_stat_schema_list(string name, t_type* ttype, t_field* tfield, T* tstruct, bool is_top_level) {
    if (ttype->is_struct()) {
        string sname = ttype->get_name();
        t_struct *cstruct = find_struct_with_name(sname);
        if (cstruct) {
            generate_stat_schema_struct(name, tfield, cstruct, tstruct, is_top_level);
        }
    } else if (ttype->is_base_type()) {
        map<string, vector<string> > suffixes;
        vector<string> top_level_tags;
        vector<string> member_tags;
        bool is_empty_tag = false;
        map<string, string>::iterator jt;
        string tname = tfield->get_name();
        jt = tfield->annotations_.find("tags");
        if (jt == tfield->annotations_.end())
            is_empty_tag = true;
        if (!is_top_level && !is_empty_tag) {
	    return;
        }
        if(!name.empty()) {
            tname = name + "." + tname;
        }
        if (is_empty_tag == false) {
            extract_tags(tfield, suffixes, top_level_tags, member_tags);
        }
        string datatype = get_datatype_from_tfield(ttype);
        string index = "false";
        bool is_suffixed_field = false;
        is_indexed_or_suffixed_field(tname, member_tags, suffixes, index, is_suffixed_field);
        if (!is_suffixed_field) {
            if (first_member_) {
                first_member_ = false;
                f_stats_tables_ << "\t{\"name\":\"" << tname << "\",\"datatype\":\"" << datatype << "\",\"index\":" << index << "}";
            } else {
                f_stats_tables_ << ",\n\t{\"name\":\"" << tname << "\",\"datatype\":\"" << datatype << "\",\"index\":" << index << "}";
            }
        }
    }
}

t_struct* t_doc_generator::find_struct_with_name(string sname) {
    t_struct *cstruct = program_->get_struct(sname.c_str());
    if (cstruct) {
        return cstruct;
    }
    // Check in includes
    const vector<t_program*>& includes = program_->get_includes();
    vector<t_program*>::const_iterator iter;
    for (iter = includes.begin(); iter != includes.end(); ++iter) {
        cstruct = (*iter)->get_struct(sname.c_str());
        if (cstruct) {
            return cstruct;
        }
    }
    return NULL;
}

template <typename T>
void t_doc_generator::generate_stat_table_schema(string oname, t_field* tfield, T* tsandesh, string prefix) {
    if (tfield->get_type()->is_list()) {
        t_type* ltype= ((t_list*)((tfield)->get_type()))->get_elem_type();
        generate_stat_schema_list(prefix, ltype, tfield, tsandesh, true);
    } else if (tfield->get_type()->is_map()) { 
        t_type* valtype= ((t_map*)(tfield->get_type()))->get_val_type();
        t_type* keytype= ((t_map*)(tfield->get_type()))->get_key_type();
        generate_stat_schema_map(prefix, keytype, valtype, tfield, tsandesh, true);
    } else if (tfield->get_type()->is_struct()) {
        string sname = (tfield->get_type())->get_name();
        t_struct *cstruct = find_struct_with_name(sname);
        if (cstruct) {
            generate_stat_schema_struct(prefix, tfield, cstruct, tsandesh, true);
        }
    }
}

string t_doc_generator::find_obj_table_name(const vector<t_field*> members) {
    string obj_table_name;
    vector<t_field*>::const_iterator m_iter;
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        map<string, string>::iterator jt;
        if((*m_iter)->get_name() == "name") {
            jt = (*m_iter)->annotations_.find("key");
            if (jt != (*m_iter)->annotations_.end()) {
                return (*m_iter)->annotations_["key"];
            }
        }
    }
    return obj_table_name;
}

template <typename T>
void t_doc_generator::find_recursive_tags_map(T* tsandesh, t_field* tfield, string table, t_type* keytype, t_type* valtype, string prefix) {
    if (valtype->is_struct()) {
        string sname = valtype->get_name();
        find_recursive_tags_struct(tsandesh, tfield, table, sname, prefix);
    }
}

template <typename T>
void t_doc_generator::find_recursive_tags_list(T* tsandesh, t_field* tfield, string table, t_type* ttype, string prefix) {
    if (ttype->is_struct()) {
        string sname = ttype->get_name();
        find_recursive_tags_struct(tsandesh, tfield, table, sname, prefix);
    }
}

template <typename T>
void t_doc_generator::find_recursive_tags_struct(T* tsandesh, t_field* tfield, string table, string sname, string prefix) {
    t_struct *cstruct = find_struct_with_name(sname);
    if(cstruct) {
        const vector<t_field*> members = cstruct->get_members();
        generate_stat_tables_schema(tsandesh, members, table, prefix);
    }
}

template <typename T>
void t_doc_generator::find_recursive_tags(T* tsandesh, t_field* tfield, string table, string fname) {
    if (tfield->get_type()->is_list()) {
        t_type* ltype= ((t_list*)((tfield)->get_type()))->get_elem_type();
        find_recursive_tags_list(tsandesh, tfield, table, ltype, fname);
    } else if (tfield->get_type()->is_map()) { 
        t_type* keytype= ((t_map*)(tfield->get_type()))->get_key_type();
        t_type* valtype= ((t_map*)(tfield->get_type()))->get_val_type();
        find_recursive_tags_map(tsandesh, tfield, table, keytype, valtype, fname);
    } else if (tfield->get_type()->is_struct()) {
        string sname = (tfield->get_type())->get_name();
        find_recursive_tags_struct(tsandesh, tfield, table, sname, fname);
    }

}

string t_doc_generator::get_display_name_from_comments(t_field* tfield){
    string doc = tfield->get_doc();
    size_t index;
    string display_name;
    if ((index = doc.find_first_of("@")) != string::npos) {
        boost::char_separator<char> fsep("@");
        tokenizer ftokens(doc, fsep);
        BOOST_FOREACH(const string &f, ftokens) {
            size_t lindex;
            if ((lindex = f.find_first_of(": \t")) != string::npos) {
                string attr_name = f.substr(0, lindex);
                boost::trim(attr_name);
                if(attr_name == "display_name") {
                    display_name = f.substr(lindex+1);
                    boost::trim(display_name);
                    return display_name;
                }
            }
        }
    }
    return display_name;
}

template <typename T>
void t_doc_generator::generate_stat_tables_schema(T* tsandesh, const vector<t_field*> members, string table, string prefix) {
    vector<t_field*>::const_iterator m_iter;
    string oname = tsandesh->get_name();
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        map<string, string>::iterator jt;
        jt = (*m_iter)->annotations_.find("tags");
        if (jt != (*m_iter)->annotations_.end()) {
            string fname = (*m_iter)->get_name();
            if (!prefix.empty())
                fname = prefix + "." + fname;
            string display_name = get_display_name_from_comments(*m_iter);
            generate_stat_table_schema_header(oname, fname, table, display_name);
            generate_stat_table_schema(oname, *m_iter, tsandesh, prefix);
            f_stats_tables_ << "\n]\n}";
            find_recursive_tags(tsandesh, *m_iter, table, fname);
        }
    }
}

void t_doc_generator::generate_stat_tables_schema(t_sandesh* tsandesh) {
    bool is_uve = ((t_base_type *)tsandesh->get_type())->is_sandesh_uve();
    const vector<t_field*> members = tsandesh->get_members();
    string empty_prefix;
    if (is_uve) {
        t_struct* data = program_->get_struct(members.front()->get_type()->get_name().c_str());
        const vector<t_field*> tmembers = data->get_members();
        string obj_table_name = find_obj_table_name(tmembers);
        generate_stat_tables_schema(data, tmembers, obj_table_name, empty_prefix);
    } else {
        string obj_table_name = "NONE";
        generate_stat_tables_schema(tsandesh, members, obj_table_name, empty_prefix);
    }
}

/**
 * Generates a struct definition for a thrift data type.
 *
 * @param tstruct The struct definition
 */
void t_doc_generator::generate_struct(t_struct* tstruct, ofstream &f_out) {
  string name = tstruct->get_name();
  f_out << "<div class=\"definition\">";
  f_out << "<h3 id=\"Struct_" << name << "\">";
  f_out << "Struct: ";
  f_out << name << "</h3>" << endl;
  vector<t_field*> members = tstruct->get_members();
  vector<t_field*>::iterator mem_iter = members.begin();
  f_out << "<table>";
  f_out << "<tr><th>Key</th><th>Field</th><th>Type</th><th>Description</th><th>Requiredness</th><th>Default value</th></tr>"
    << endl;
  for ( ; mem_iter != members.end(); mem_iter++) {
    f_out << "<tr><td>" << (*mem_iter)->get_key() << "</td><td>";
    f_out << (*mem_iter)->get_name();
    f_out << "</td><td>";
    print_type((*mem_iter)->get_type(), f_out);
    f_out << "</td><td>";
    f_out << (*mem_iter)->get_doc();
    f_out << "</td><td>";
    if ((*mem_iter)->get_req() == t_field::T_OPTIONAL) {
      f_out << "optional";
    } else if ((*mem_iter)->get_req() == t_field::T_REQUIRED) {
      f_out << "required";
    } else {
      f_out << "default";
    }
    f_out << "</td><td>";
    t_const_value* default_val = (*mem_iter)->get_value();
    if (default_val != NULL) {
      print_const_value(default_val, f_out);
    }
    f_out << "</td></tr>" << endl;
  }
  f_out << "</table><br/>";
  print_doc(tstruct, f_out);
  f_out << "</div>";
}

/**
 * Generates the HTML block for a sandesh.
 *
 * @param tsandesh The sandesh definition
 */
void t_doc_generator::generate_sandesh(t_sandesh* tsandesh, ofstream &f_out) {
  f_out << "<h3 id=\"Snh_" << sandesh_name_ << "\"> "
    << sandesh_name_ << "</h3>" << endl;
  print_sandesh(tsandesh, f_out);
  vector<t_field*> members = tsandesh->get_members();
  f_out << "<table>";
  f_out << "<tr><th>Field</th><th>Type</th><th>Description</th></tr>"
    << endl;
  BOOST_FOREACH(t_field *tfield, members) {
    if (tfield->get_auto_generated()) {
      continue;
    }
    t_type *type = tfield->get_type();
    if (type->is_static_const_string()) {
      continue;
    }
    f_out << "<tr><td>" << tfield->get_name() << "</td><td>";
    print_type(tfield->get_type(), f_out);
    f_out << "</td><td>";
    f_out << tfield->get_doc();
    f_out << "</td></tr>" << endl;
  }
  f_out << "</table><br/>";
}

THRIFT_REGISTER_GENERATOR(doc, "Documentation", "")

