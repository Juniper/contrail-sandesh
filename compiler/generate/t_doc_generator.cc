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

  void generate_program();
  void generate_program_toc();
  void generate_program_toc_row(t_program* tprog);
  void generate_program_toc_rows(t_program* tprog,
         vector<t_program*>& finished);
  void generate_index();

  /**
   * Program-level generation functions
   */

  template <typename T>
  void generate_stat_table_schema(string, t_field*, T*, string);
  template <typename T>
  void generate_stat_schema_map(string, t_type*, t_type*, t_field*, T*);
  template <typename T>
  void generate_stat_schema_list(string, t_type*, t_field*, T*, bool);
  template <typename T>
  void generate_stat_schema_struct(string, string, t_field*, T*, bool, string);
  t_struct* find_struct_with_name(string sname);
  template <typename T>
  void generate_stat_schema_struct_members(string, t_field*, t_struct*, T*);
  void extract_tags(t_field*, map<string, vector<string> >&, vector<string>&, vector<string>&);
  void generate_stat_schema_struct_base_member(string, t_field*, string);
  void generate_stat_table_schema_header(string, string, string, string);
  string get_datatype_from_tfield(t_type* tfield);
  template <typename T>
  string get_type_of_member(string, t_field*, t_struct*, T*);
  template <typename T>
  void populate_stat_schema_map_key_value(t_field* tfield, t_type*, t_type*, T*);
  void generate_stat_schema_struct_members(string name, t_field*, t_struct*, vector<string>);
  void is_indexed_or_suffixed_field(string, vector<string>, string&, bool&);
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
  void find_recursive_tags_map(T*, t_field*, string, t_type*, string);
  string find_obj_table_name(const vector<t_field*>);
  void generate_typedef (t_typedef*  ttypedef);
  void generate_enum    (t_enum*     tenum);
  void generate_const   (t_const*    tconst);
  void generate_struct  (t_struct*   tstruct);
  void generate_service (t_service*  tservice) {}
  void generate_sandesh (t_sandesh*  tsandesh);

  void print_doc        (t_doc* tdoc);
  int  print_type       (t_type* ttype);
  void print_const_value(t_const_value* tvalue);
  void print_sandesh    (t_sandesh* tdoc);
  void print_sandesh_message(t_sandesh* tdoc);
  void print_sandesh_message_table(t_sandesh* tdoc);
  void print_doc_string (string doc);

  ofstream f_out_;
  ofstream f_stats_tables_;
 private:
  bool stat_table_created;
  bool first_member;
};

/**
 * Emits the Table of Contents links at the top of the module's page
 */
void t_doc_generator::generate_program_toc() {
  f_out_ << "<table><tr><th>Module</th><th>Messages</th></tr>" << endl;
  generate_program_toc_row(program_);
  f_out_ << "</table>" << endl;
}


/**
 * Recurses through from the provided program and generates a ToC row
 * for each discovered program exactly once by maintaining the list of
 * completed rows in 'finished'
 */
void t_doc_generator::generate_program_toc_rows(t_program* tprog,
         vector<t_program*>& finished) {
  for (vector<t_program*>::iterator iter = finished.begin();
       iter != finished.end(); iter++) {
    if (tprog->get_path() == (*iter)->get_path()) {
      return;
    }
  }
  finished.push_back(tprog);
  generate_program_toc_row(tprog);
  vector<t_program*> includes = tprog->get_includes();
  for (vector<t_program*>::iterator iter = includes.begin();
       iter != includes.end(); iter++) {
    generate_program_toc_rows(*iter, finished);
  }
}

/**
 * Emits the Table of Contents links at the top of the module's page
 */
void t_doc_generator::generate_program_toc_row(t_program* tprog) {
  string fname = tprog->get_name() + ".html";
  f_out_ << "<tr>" << endl << "<td>" << tprog->get_name() << "</td><td>";
  if (!tprog->get_sandeshs().empty()) {
    vector<t_sandesh*> sandeshs = tprog->get_sandeshs();
    vector<t_sandesh*>::iterator snh_iter;
    for (snh_iter = sandeshs.begin(); snh_iter != sandeshs.end(); ++snh_iter) {
      string name = get_sandesh_name(*snh_iter);
      f_out_ << "<a href=\"" << fname << "#Snh_" << name << "\">" << name
        << "</a><br/>" << endl;
    }
  }
  f_out_ << "</td>" << endl;
  f_out_ << "</tr>";
}

/**
 * Prepares for file generation by opening up the necessary file output
 * stream.
 */
void t_doc_generator::generate_program() {
  // Make output directory
  MKDIR(get_out_dir().c_str());
  string fname = get_out_dir() + program_->get_name() + ".html";
  f_out_.open(fname.c_str());
  f_out_ << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"" << endl;
  f_out_ << "    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" << endl;
  f_out_ << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl;
  f_out_ << "<head>" << endl;
  f_out_ << "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />" << endl;
  f_out_ << "<link href=\"/doc-style.css\" rel=\"stylesheet\" type=\"text/css\"/>"
   << endl;
  f_out_ << "<title>Documentation for module: " << program_->get_name()
   << "</title></head><body>" << endl << "<h1>Documentation for module: "
   << program_->get_name() << "</h1>" << endl;

  fname = get_out_dir() + program_->get_name() + "_stats_tables.json";
  f_stats_tables_.open(fname.c_str());
  stat_table_created = false;
  first_member = true;
  print_doc(program_);

  generate_program_toc();

  f_stats_tables_ << "{\"_STAT_TABLES\":[" << endl;
  if (!program_->get_sandeshs().empty()) {
    f_out_ << "<hr/><h2 id=\"Messages\">Messages</h2>" << endl;
    // Generate sandeshs
    vector<t_sandesh*> sandeshs = program_->get_sandeshs();
    vector<t_sandesh*>::iterator snh_iter;
    for (snh_iter = sandeshs.begin(); snh_iter != sandeshs.end(); ++snh_iter) {
      sandesh_name_ = get_sandesh_name(*snh_iter);
      f_out_ << "<div class=\"definition\">";
      generate_sandesh(*snh_iter);
      f_out_ << "</div>";
      generate_stat_tables_schema(*snh_iter);
    }
  }

  if (!program_->get_consts().empty()) {
    f_out_ << "<hr/><h2 id=\"Constants\">Constants</h2>" << endl;
    vector<t_const*> consts = program_->get_consts();
    f_out_ << "<table>";
    f_out_ << "<tr><th>Constant</th><th>Type</th><th>Value</th></tr>" << endl;
    generate_consts(consts);
    f_out_ << "</table>";
  }

  if (!program_->get_enums().empty()) {
    f_out_ << "<hr/><h2 id=\"Enumerations\">Enumerations</h2>" << endl;
    // Generate enums
    vector<t_enum*> enums = program_->get_enums();
    vector<t_enum*>::iterator en_iter;
    for (en_iter = enums.begin(); en_iter != enums.end(); ++en_iter) {
      generate_enum(*en_iter);
    }
  }

  if (!program_->get_typedefs().empty()) {
    f_out_ << "<hr/><h2 id=\"Typedefs\">Type declarations</h2>" << endl;
    // Generate typedefs
    vector<t_typedef*> typedefs = program_->get_typedefs();
    vector<t_typedef*>::iterator td_iter;
    for (td_iter = typedefs.begin(); td_iter != typedefs.end(); ++td_iter) {
      generate_typedef(*td_iter);
    }
  }

  if (!program_->get_objects().empty()) {
    f_out_ << "<hr/><h2 id=\"Structs\">Data structures</h2>" << endl;
    // Generate structs and exceptions in declared order
    vector<t_struct*> objects = program_->get_objects();
    vector<t_struct*>::iterator o_iter;
    for (o_iter = objects.begin(); o_iter != objects.end(); ++o_iter) {
      generate_struct(*o_iter);
    }
  }

  f_stats_tables_ << "]" << endl << "}" << endl;
  f_out_ << "</body></html>" << endl;
  f_out_.close();
}

/**
 * Emits the index.html file for the recursive set of Thrift programs
 */
void t_doc_generator::generate_index() {
  string index_fname = get_out_dir() + "index.html";
  f_out_.open(index_fname.c_str());
  f_out_ << "<html><head>" << endl;
  f_out_ << "<link href=\"style.css\" rel=\"stylesheet\" type=\"text/css\"/>"
   << endl;
  f_out_ << "<table><tr><th>Module</th><th>Messages</th></tr>" << endl;
  vector<t_program*> programs;
  generate_program_toc_rows(program_, programs);
  f_out_ << "</table>" << endl;
  f_out_ << "</body></html>" << endl;
  f_out_.close();
}

void t_doc_generator::print_doc_string(string doc) {
  size_t index;
  while ((index = doc.find_first_of("\r\n")) != string::npos) {
    if (index == 0) {
      f_out_ << "<p/>" << endl;
    } else {
      f_out_ << doc.substr(0, index) << endl;
    }
    if (index + 1 < doc.size() && doc.at(index) != doc.at(index + 1) &&
        (doc.at(index + 1) == '\r' || doc.at(index + 1) == '\n')) {
      index++;
    }
    doc = doc.substr(index + 1);
  }
  f_out_ << doc << "<br/>";
}

/**
 * If the provided documentable object has documentation attached, this
 * will emit it to the output stream in HTML format.
 */
void t_doc_generator::print_doc(t_doc* tdoc) {
  if (tdoc->has_doc()) {
    string doc = tdoc->get_doc();
    print_doc_string(doc);
  }
}

void t_doc_generator::print_sandesh_message(t_sandesh* tsandesh) {
  bool first = true;
  // Print the message
  vector<t_field*> members = tsandesh->get_members();
  BOOST_FOREACH(t_field *tfield, members) {
    if (tfield->get_auto_generated()) {
      continue;
    }
    if (first) {
      f_out_ << "<tr><th>Message</th><td>";
    }
    t_type *type = tfield->get_type();
    if (type->is_static_const_string()) {
      t_const_value* default_val = tfield->get_value();
      if (!first) {
        f_out_ << " ";
      }
      f_out_ << get_escaped_string(default_val);
    } else {
      f_out_ << "<code>";
      if (!first) {
        f_out_ << " ";
      }
      f_out_ << tfield->get_name() << "</code>";
    }
    first = false;
  }
  if (!first) {
    f_out_ << "</td></tr>" << endl;
  }
}

void t_doc_generator::print_sandesh_message_table(t_sandesh* tsandesh) {
  f_out_ << "<table>";
  print_sandesh_message(tsandesh);
  f_out_ << "</table><br/>" << endl;
}

/**
 * If the provided documentable sandesh has documentation attached, this
 * will emit it to the output stream in HTML format and print the @variables
 * in a table format.
 */
typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
void t_doc_generator::print_sandesh(t_sandesh* tsandesh) {
  if (tsandesh->has_doc()) {
    bool table = false;
    string doc = tsandesh->get_doc();
    size_t index;
    if ((index = doc.find_first_of("@")) != string::npos) {
      // Print leading documentation
      f_out_ << doc.substr(0, index) << "<p/>" << endl;
      string fdoc(doc.substr(index + 1));
      if (!table) {
        f_out_ << "<table>" << endl;
        print_sandesh_message(tsandesh);
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
          f_out_ << "<tr><th>" << type << "</th>";
          if (lindex + 1 < f.size() && f.at(lindex) != f.at(lindex + 1) &&
              (f.at(lindex + 1) == ':' || f.at(lindex + 1) == '\t' ||
               f.at(lindex + 1) == ' ')) {
            lindex++;
          }
          string content(f.substr(lindex + 1));
          f_out_ << "<td>" << content << "</td></tr>" << endl;
        } else {
          f_out_ << "<td>" << f << "</td>" << endl;
        }
      }
    } else {
      f_out_ << doc << "<p/>" << endl;
      print_sandesh_message_table(tsandesh);
    }
    if (table) {
      f_out_ << "</table><br/>" << endl;
    }
  } else {
    print_sandesh_message_table(tsandesh);
  }
}

/**
 * Prints out the provided type in HTML
 */
int t_doc_generator::print_type(t_type* ttype) {
  int len = 0;
  f_out_ << "<code>";
  if (ttype->is_container()) {
    if (ttype->is_list()) {
      f_out_ << "list&lt;";
      len = 6 + print_type(((t_list*)ttype)->get_elem_type());
      f_out_ << "&gt;";
    } else if (ttype->is_set()) {
      f_out_ << "set&lt;";
      len = 5 + print_type(((t_set*)ttype)->get_elem_type());
      f_out_ << "&gt;";
    } else if (ttype->is_map()) {
      f_out_ << "map&lt;";
      len = 5 + print_type(((t_map*)ttype)->get_key_type());
      f_out_ << ", ";
      len += print_type(((t_map*)ttype)->get_val_type());
      f_out_ << "&gt;";
    }
  } else if (ttype->is_base_type()) {
    f_out_ << (((t_base_type*)ttype)->is_binary() ? "binary" : ttype->get_name());
    len = ttype->get_name().size();
  } else {
    string prog_name = ttype->get_program()->get_name();
    string type_name = ttype->get_name();
    f_out_ << "<a href=\"" << prog_name << ".html#";
    if (ttype->is_typedef()) {
      f_out_ << "Typedef_";
    } else if (ttype->is_struct() || ttype->is_xception()) {
      f_out_ << "Struct_";
    } else if (ttype->is_enum()) {
      f_out_ << "Enum_";
    } else if (ttype->is_service()) {
      f_out_ << "Svc_";
    }
    f_out_ << type_name << "\">";
    len = type_name.size();
    if (ttype->get_program() != program_) {
      f_out_ << prog_name << ".";
      len += prog_name.size() + 1;
    }
    f_out_ << type_name << "</a>";
  }
  f_out_ << "</code>";
  return len;
}

/**
 * Prints out an HTML representation of the provided constant value
 */
void t_doc_generator::print_const_value(t_const_value* tvalue) {
  bool first = true;
  switch (tvalue->get_type()) {
  case t_const_value::CV_INTEGER:
    f_out_ << tvalue->get_integer();
    break;
  case t_const_value::CV_DOUBLE:
    f_out_ << tvalue->get_double();
    break;
  case t_const_value::CV_STRING:
    f_out_ << '"' << get_escaped_string(tvalue) << '"';
    break;
  case t_const_value::CV_MAP:
    {
      f_out_ << "{ ";
      map<t_const_value*, t_const_value*> map_elems = tvalue->get_map();
      map<t_const_value*, t_const_value*>::iterator map_iter;
      for (map_iter = map_elems.begin(); map_iter != map_elems.end(); map_iter++) {
        if (!first) {
          f_out_ << ", ";
        }
        first = false;
        print_const_value(map_iter->first);
        f_out_ << " = ";
        print_const_value(map_iter->second);
      }
      f_out_ << " }";
    }
    break;
  case t_const_value::CV_LIST:
    {
      f_out_ << "{ ";
      vector<t_const_value*> list_elems = tvalue->get_list();;
      vector<t_const_value*>::iterator list_iter;
      for (list_iter = list_elems.begin(); list_iter != list_elems.end(); list_iter++) {
        if (!first) {
          f_out_ << ", ";
        }
        first = false;
        print_const_value(*list_iter);
      }
      f_out_ << " }";
    }
    break;
  default:
    f_out_ << "UNKNOWN";
    break;
  }
}

/**
 * Generates a typedef.
 *
 * @param ttypedef The type definition
 */
void t_doc_generator::generate_typedef(t_typedef* ttypedef) {
  string name = ttypedef->get_name();
  f_out_ << "<div class=\"definition\">";
  f_out_ << "<h3 id=\"Typedef_" << name << "\">Typedef: " << name
   << "</h3>" << endl;
  f_out_ << "<p><strong>Base type:</strong>&nbsp;";
  print_type(ttypedef->get_type());
  f_out_ << "</p>" << endl;
  print_doc(ttypedef);
  f_out_ << "</div>" << endl;
}

/**
 * Generates code for an enumerated type.
 *
 * @param tenum The enumeration
 */
void t_doc_generator::generate_enum(t_enum* tenum) {
  string name = tenum->get_name();
  f_out_ << "<div class=\"definition\">";
  f_out_ << "<h3 id=\"Enum_" << name << "\">Enumeration: " << name
   << "</h3>" << endl;
  print_doc(tenum);
  vector<t_enum_value*> values = tenum->get_constants();
  vector<t_enum_value*>::iterator val_iter;
  f_out_ << "<br/><table>" << endl;
  for (val_iter = values.begin(); val_iter != values.end(); ++val_iter) {
    f_out_ << "<tr><td><code>";
    f_out_ << (*val_iter)->get_name();
    f_out_ << "</code></td><td><code>";
    f_out_ << (*val_iter)->get_value();
    f_out_ << "</code></td></tr>" << endl;
  }
  f_out_ << "</table></div>" << endl;
}

/**
 * Generates a constant value
 */
void t_doc_generator::generate_const(t_const* tconst) {
  string name = tconst->get_name();
  f_out_ << "<tr id=\"Const_" << name << "\"><td><code>" << name
   << "</code></td><td><code>";
  print_type(tconst->get_type());
  f_out_ << "</code></td><td><code>";
  print_const_value(tconst->get_value());
  f_out_ << "</code></td></tr>";
  if (tconst->has_doc()) {
    f_out_ << "<tr><td colspan=\"3\"><blockquote>";
    print_doc(tconst);
    f_out_ << "</blockquote></td></tr>";
  }
}

void t_doc_generator::generate_stat_table_schema_header(string type, string attr, string table_name, string display_name) {
    if (stat_table_created == false) {
        stat_table_created = true;
        f_stats_tables_ << "{\n\"stat_type\":\"" << type << "\"," << endl;
    } else {
        f_stats_tables_ << ",\n{\n\"stat_type\":\"" << type << "\"," << endl;
    }
    f_stats_tables_ << "\"stat_attr\":\"" << attr << "\"," << endl;
    f_stats_tables_ << "\"display_name\":\"" << display_name << "\"," << endl;
    f_stats_tables_ << "\"obj_table\": \"" << table_name << "\",\n";
    f_stats_tables_ << "\"attributes\":[" << endl;
    first_member = true;
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
            if (name == (*m_iter)->get_name().c_str()) {
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
            if (name.substr(1) == (*m_iter)->get_name().c_str()) {
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
            if (tag == (*m_iter)->get_name().c_str()) {
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
            fname = tfield->get_name().c_str() + fname;
        }
        if (!prefix.empty()) {
            fname = prefix + "." + fname;
        }
        string index = "\"true\"";
        string suffix = "[";
        vector<string>::iterator name_it;
        for (name_it = it->second.begin(); name_it!=it->second.end(); ++name_it) {
            string suffix_name = tfield->get_name().c_str() + *name_it;
            if (boost::starts_with(suffix_name, ".")) {
                suffix_name = tfield->get_name().c_str() + suffix_name;
            }
            if (!prefix.empty()) {
                suffix_name = prefix + "." + suffix_name;
            }
            if(name_it == it->second.begin()) {
                suffix = suffix + "'" + suffix_name + "\'";
            } else {
                suffix = suffix + ", '" + suffix_name + "\'";
            }
        }
        suffix += "]";
        f_stats_tables_ << ",\n\t{\"name\":\"" << fname << "\",\"datatype\":\"" << datatype << "\",\"index\": " << index << ",\"suffixes\":\"" << suffix << "\"}";
    }
}

void t_doc_generator::is_indexed_or_suffixed_field(string fname, vector<string> tags, string& index, bool& is_suffixed_field) {
    BOOST_FOREACH(const string &tag, tags) {
        string field_name = string(".") + fname;
        string trimmed_tag = tag;
        boost::trim(trimmed_tag);
        if (trimmed_tag == field_name) {
            index = "\"true\"";
            break;
        } else if (boost::starts_with(trimmed_tag, field_name)) {
            is_suffixed_field = true;
            break;
        }
    }
}

void t_doc_generator::generate_stat_schema_struct_members(string prefix, t_field* tfield, t_struct* cstruct, vector<string> tags) {
    const vector<t_field*> members = cstruct->get_members();
    vector<t_field*>::const_iterator m_iter;
    string name = tfield->get_name().c_str();
    if(!prefix.empty()) {
        name = prefix + "." + name;
    }
    bool is_empty_tag = tags.empty();
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        string index = "\"false\"";
        bool is_suffixed_field = false;
        is_indexed_or_suffixed_field((*m_iter)->get_name(), tags, index, is_suffixed_field);
        if (is_suffixed_field) {
            continue;
        }
        t_type* mtype = (*m_iter)->get_type();
        if(mtype->is_base_type() && (!is_empty_tag || prefix.empty())) {
            generate_stat_schema_struct_base_member(name, *m_iter, index);
        } else if (mtype->is_struct()) {
            string mtype_name = mtype->get_name();
            string empty_prefix;
            generate_stat_schema_struct(name, mtype_name, (*m_iter), cstruct, true, empty_prefix);
        } else if (mtype->is_list()) {
            t_type* etype= ((t_list*)((*m_iter)->get_type()))->get_elem_type();
            generate_stat_schema_list(name, etype, (*m_iter), cstruct, true);
        } else if (mtype->is_map()) {
            t_type* valtype= ((t_map*)((*m_iter)->get_type()))->get_val_type();
            t_type* keytype= ((t_map*)((*m_iter)->get_type()))->get_key_type();
            generate_stat_schema_map(name, keytype, valtype, tfield, cstruct);
        }
    }
}

string t_doc_generator::get_datatype_from_tfield(t_type* ttype) {
    string datatype = ttype->get_name().c_str();
    if ((datatype == "u32") || (datatype == "u16") || (datatype == "u64"))
        datatype = "int";
    return datatype;
}

template <typename T>
void t_doc_generator::generate_stat_schema_map(string sname, t_type* keytype, t_type* valtype, t_field* tfield, T* tstruct) {
    map<string, vector<string> > suffixes;
    vector<string> top_level_tags;
    vector<string> member_tags;
    extract_tags(tfield, suffixes, top_level_tags, member_tags);
    string name = tfield->get_name().c_str() + string(".__key");
    string datatype = get_datatype_from_tfield(keytype);
    string index = "\"false\"";
    bool is_suffixed_field = false;
    is_indexed_or_suffixed_field(string("__key"), member_tags, index, is_suffixed_field);
    if (!is_suffixed_field) {
        f_stats_tables_ << "\t{\"name\":\"" << name << "\",\"datatype\":\"" << datatype << "\",\"index\": " << index << "},\n";
    }
    if(valtype->is_base_type()) {
        name = tfield->get_name().c_str() + string(".__value");
        datatype = get_datatype_from_tfield(valtype);
        index = "\"false\"";
        BOOST_FOREACH(const string &f, member_tags) {
            if (f == string(".__value")) {
                index = "\"true\"";
            }
        }
        f_stats_tables_ << "\t{\"name\":\"" << name << "\",\"datatype\":\"" << datatype << "\",\"index\": " << index << "}";
    }

    string empty_prefix;
    if (valtype->is_struct()) {
        sname = valtype->get_name();
        t_struct *cstruct = find_struct_with_name(sname);
        if (cstruct) {
            generate_stat_schema_struct_members(empty_prefix, tfield, cstruct, member_tags);
            generate_stat_schema_toplevel_tags(tstruct, top_level_tags);
            generate_stat_schema_suffixes(empty_prefix, tfield, cstruct, tstruct, suffixes);
        }
    } else {
        generate_stat_schema_toplevel_tags(tstruct, top_level_tags);
        generate_stat_schema_suffixes(empty_prefix, tfield, NULL, tstruct, suffixes);
    }
}

void t_doc_generator::generate_stat_schema_struct_base_member(string name, t_field* tfield, string index) {
    string fname = tfield->get_name().c_str();
    if (!name.empty()) {
        fname = name + string(".") + fname;
    }
    string datatype = get_datatype_from_tfield(tfield->get_type());
    if(!first_member) {
        f_stats_tables_ << ",\n\t{\"name\":\"" << fname << "\",\"datatype\":\"" << datatype << "\",\"index\": " << index << "}";
    } else {
        first_member = false;
        f_stats_tables_ << "\t{\"name\":\"" << fname << "\",\"datatype\":\"" << datatype << "\",\"index\": " << index << "}";
    }
}

void t_doc_generator::extract_tags(t_field* tfield, map<string, vector<string> > &suffixes, vector<string> &top_level_tags, vector<string> &member_tags) {
    boost::char_separator<char> fsep(",");
    tokenizer tags(tfield->annotations_["tags"], fsep);
    BOOST_FOREACH(const string &tag, tags) {
        size_t index = tag.find(':');
        if(index != string::npos) {
            string fname = tag.substr(0, index);
            boost::trim(fname);
            std::string suffix_name = tag.substr(index+1);
            if(suffixes.find(fname) == suffixes.end()) {
                vector<string> new_suffix;
                new_suffix.push_back(suffix_name);
                suffixes.insert(make_pair(fname, new_suffix));
            } else {
                vector<string> old_suffixes = suffixes[fname];
                old_suffixes.push_back(suffix_name);
                suffixes[fname] = old_suffixes;
            }
        } else if (!boost::starts_with(tag, ".")) {
            top_level_tags.push_back(tag);
        } else {
            member_tags.push_back(tag);
        }
    }
}

template <typename T>
void t_doc_generator::generate_stat_schema_struct_members(string prefix, t_field* tfield, t_struct* cstruct, T* tstruct) {
    map<string, vector<string> > suffixes;
    vector<string> top_level_tags;
    vector<string> member_tags;
    extract_tags(tfield, suffixes, top_level_tags, member_tags);
    generate_stat_schema_struct_members(prefix, tfield, cstruct, member_tags);
    generate_stat_schema_toplevel_tags(tstruct, top_level_tags);
    generate_stat_schema_suffixes(prefix, tfield, cstruct, tstruct, suffixes);
}

template <typename T>
void t_doc_generator::generate_stat_schema_list(string name, t_type* ttype, t_field* tfield, T* tstruct, bool is_recursive) {
    if (ttype->is_struct()) {
        string sname = ttype->get_name();
        string empty_prefix;
        generate_stat_schema_struct(name, sname, tfield, tstruct, is_recursive, empty_prefix);
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
void t_doc_generator::generate_stat_schema_struct(string oname, string sname, t_field* tfield, T* tstruct, bool is_recursive, string prefix) {
    t_struct *cstruct = find_struct_with_name(sname);
    if (cstruct) {
        if(is_recursive) {
            generate_stat_schema_struct_members(oname+"."+prefix, tfield, cstruct, tstruct);
        } else {
            generate_stat_schema_struct_members(prefix, tfield, cstruct, tstruct);
        }
    }
}

template <typename T>
void t_doc_generator::generate_stat_table_schema(string oname, t_field* tfield, T* tsandesh, string prefix) {
    if (tfield->get_type()->is_list()) {
        t_type* ltype= ((t_list*)((tfield)->get_type()))->get_elem_type();
        generate_stat_schema_list(oname, ltype, tfield, tsandesh, false);
    } else if (tfield->get_type()->is_map()) { 
        t_type* valtype= ((t_map*)(tfield->get_type()))->get_val_type();
        t_type* keytype= ((t_map*)(tfield->get_type()))->get_key_type();
        generate_stat_schema_map(oname, keytype, valtype, tfield, tsandesh);
    } else if (tfield->get_type()->is_struct()) {
        string sname = (tfield->get_type())->get_name();
        generate_stat_schema_struct(oname, sname, tfield, tsandesh, false, prefix);
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
void t_doc_generator::find_recursive_tags_map(T* tsandesh, t_field* tfield, string table, t_type* valtype, string prefix) {
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
        t_type* valtype= ((t_map*)(tfield->get_type()))->get_val_type();
        find_recursive_tags_map(tsandesh, tfield, table, valtype, fname);
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
    string oname = tsandesh->get_name().c_str();
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
void t_doc_generator::generate_struct(t_struct* tstruct) {
  string name = tstruct->get_name();
  f_out_ << "<div class=\"definition\">";
  f_out_ << "<h3 id=\"Struct_" << name << "\">";
  f_out_ << "Struct: ";
  f_out_ << name << "</h3>" << endl;
  vector<t_field*> members = tstruct->get_members();
  vector<t_field*>::iterator mem_iter = members.begin();
  f_out_ << "<table>";
  f_out_ << "<tr><th>Key</th><th>Field</th><th>Type</th><th>Description</th><th>Requiredness</th><th>Default value</th></tr>"
    << endl;
  for ( ; mem_iter != members.end(); mem_iter++) {
    f_out_ << "<tr><td>" << (*mem_iter)->get_key() << "</td><td>";
    f_out_ << (*mem_iter)->get_name();
    f_out_ << "</td><td>";
    print_type((*mem_iter)->get_type());
    f_out_ << "</td><td>";
    f_out_ << (*mem_iter)->get_doc();
    f_out_ << "</td><td>";
    if ((*mem_iter)->get_req() == t_field::T_OPTIONAL) {
      f_out_ << "optional";
    } else if ((*mem_iter)->get_req() == t_field::T_REQUIRED) {
      f_out_ << "required";
    } else {
      f_out_ << "default";
    }
    f_out_ << "</td><td>";
    t_const_value* default_val = (*mem_iter)->get_value();
    if (default_val != NULL) {
      print_const_value(default_val);
    }
    f_out_ << "</td></tr>" << endl;
  }
  f_out_ << "</table><br/>";
  print_doc(tstruct);
  f_out_ << "</div>";
}

/**
 * Generates the HTML block for a sandesh.
 *
 * @param tsandesh The sandesh definition
 */
void t_doc_generator::generate_sandesh(t_sandesh* tsandesh) {
  f_out_ << "<h3 id=\"Snh_" << sandesh_name_ << "\"> "
    << sandesh_name_ << "</h3>" << endl;
  print_sandesh(tsandesh);
  vector<t_field*> members = tsandesh->get_members();
  f_out_ << "<table>";
  f_out_ << "<tr><th>Field</th><th>Type</th><th>Description</th></tr>"
    << endl;
  BOOST_FOREACH(t_field *tfield, members) {
    if (tfield->get_auto_generated()) {
      continue;
    }
    t_type *type = tfield->get_type();
    if (type->is_static_const_string()) {
      continue;
    }
    f_out_ << "<tr><td>" << tfield->get_name() << "</td><td>";
    print_type(tfield->get_type());
    f_out_ << "</td><td>";
    f_out_ << tfield->get_doc();
    f_out_ << "</td></tr>" << endl;
  }
  f_out_ << "</table><br/>";
}

THRIFT_REGISTER_GENERATOR(doc, "Documentation", "")

