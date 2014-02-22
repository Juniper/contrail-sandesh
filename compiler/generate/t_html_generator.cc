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
         std::vector<t_program*>& finished);
  void generate_index();
  void generate_css();
  void generate_xsl();
  void generate_xml();

  /**
   * Program-level generation functions
   */
  void generate_field_xml(t_field * tfl);
  void generate_struct_xml(const string& stname);
  void generate_sandesh_xml(t_sandesh* tsnd);
  void generate_field_xsl(t_field * tfl);
  void generate_struct_xsl(t_struct* tstruct);
  void generate_sandesh_xsl(t_sandesh* tsnd);
  void generate_typedef (t_typedef*  ttypedef) {}
  void generate_enum    (t_enum*     tenum) {}
  void generate_const   (t_const*    tconst) {}
  void generate_struct  (t_struct*   tstruct) {}
#ifdef SANDESH
  void generate_sandesh (t_sandesh*  tsandesh);
#endif
  void generate_service (t_service*  tservice);
  void generate_xception(t_struct*   txception) {}

  void print_doc        (t_doc* tdoc);
  int  print_type       (t_type* ttype);
  void print_const_value(t_const_value* tvalue);

  std::ofstream f_out_;
};

/**
 * Emits the Table of Contents links at the top of the module's page
 */
void t_html_generator::generate_program_toc() {

#ifdef SANDESH
  f_out_ << "<table><tr><th>Module</th><th>Sandeshs</th></tr>" << endl;
#else
  f_out_ << "<table><tr><th>Module</th><th>Services</th></tr>" << endl;
#endif
  generate_program_toc_row(program_);
  f_out_ << "</table>" << endl;
}


/**
 * Recurses through from the provided program and generates a ToC row
 * for each discovered program exactly once by maintaining the list of
 * completed rows in 'finished'
 */
void t_html_generator::generate_program_toc_rows(t_program* tprog,
         std::vector<t_program*>& finished) {
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
void t_html_generator::generate_program_toc_row(t_program* tprog) {
  string fname = tprog->get_name() + ".html";
  f_out_ << "<tr>" << endl << "<td>" << tprog->get_name() << "</td><td>";
#if SANDESH
  map<string,string> sn_html;
  if (!tprog->get_sandeshs().empty()) {
    vector<t_sandesh*> sandeshs = tprog->get_sandeshs();
    vector<t_sandesh*>::iterator sn_iter;
    for (sn_iter = sandeshs.begin(); sn_iter != sandeshs.end(); ++sn_iter) {
      if (static_cast<const t_base_type*>((*sn_iter)->get_type())->get_base() !=
              t_base_type::TYPE_SANDESH_REQUEST) 
          continue;
      string name = (*sn_iter)->get_name();
      // f_out_ << "<a href=\"" << fname << "#Enum_" << name << "\">" << name
      // <<  "</a><br/>" << endl;
      string html = "<a href=\"" + fname + "#Snh_" + name + "\">" + name +
        "</a>";
      sn_html.insert(pair<string,string>(name, html));
    }
    for (map<string,string>::iterator sn_iter = sn_html.begin();
         sn_iter != sn_html.end(); sn_iter++) {
      f_out_ << sn_iter->second << "<br/>" << endl;
    }
  }
#else
  if (!tprog->get_services().empty()) {
    vector<t_service*> services = tprog->get_services();
    vector<t_service*>::iterator sv_iter;
    for (sv_iter = services.begin(); sv_iter != services.end(); ++sv_iter) {
      string name = get_service_name(*sv_iter);
      f_out_ << "<a href=\"" << fname << "#Svc_" << name << "\">" << name
        << "</a><br/>" << endl;
      f_out_ << "<ul>" << endl;
      map<string,string> fn_html;
      vector<t_function*> functions = (*sv_iter)->get_functions();
      vector<t_function*>::iterator fn_iter;
      for (fn_iter = functions.begin(); fn_iter != functions.end(); ++fn_iter) {
        string fn_name = (*fn_iter)->get_name();
        string html = "<li><a href=\"" + fname + "#Fn_" + name + "_" +
          fn_name + "\">" + fn_name + "</a></li>";
        fn_html.insert(pair<string,string>(fn_name, html));
      }
      for (map<string,string>::iterator html_iter = fn_html.begin();
        html_iter != fn_html.end(); html_iter++) {
        f_out_ << html_iter->second << endl;
      }
      f_out_ << "</ul>" << endl;
    }
  }
#endif
  f_out_ << "</td></tr>" << endl;
}

void t_html_generator::generate_field_xsl(t_field * tfl) {
    string fdname = tfl->get_name();
    t_type * fdtype = tfl->get_type();
    string fdtname = fdtype->get_name();

    //TODO: add support for more types
    if (fdtype->is_base_type()) {
      f_out_ << "<td style=\"color:blue\"><xsl:value-of select=\"" <<
          fdname << "\"/></td>" << endl;
    } else if (fdtype->is_list()) {
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
    } else {
        f_out_ << "<td><table border=\"1\">" << endl;
        f_out_ << "<xsl:apply-templates select=\"" << fdname << 
            "/" << fdtname << "\"/></table></td>" << endl;
    }
}

void t_html_generator::generate_sandesh_xsl(t_sandesh* tsnd) {
  string name = tsnd->get_name();

  f_out_ << "<xsl:template match=\"" << name << "\">" << endl;
  f_out_ << "<xsl:choose><xsl:when test=\"../@type = 'slist'\"><tr>" << endl;
  {
      vector<t_field*> members = tsnd->get_members();
      vector<t_field*>::iterator mem_iter = members.begin();
      for ( ; mem_iter != members.end(); mem_iter++) {
          if ((*mem_iter)->get_auto_generated()) continue;
          generate_field_xsl(*mem_iter);
      }
  }
  f_out_ << "</tr></xsl:when><xsl:otherwise>" << endl;
  f_out_ << "<h2>" << name << "</h2>" << endl;
  f_out_ << "<table border=\"1\">" << endl;

  {
      vector<t_field*> members = tsnd->get_members();
      vector<t_field*>::iterator mem_iter = members.begin();
      for ( ; mem_iter != members.end(); mem_iter++) {
          if ((*mem_iter)->get_auto_generated()) continue;
          f_out_ << "<tr><td valign=\"top\">" << (*mem_iter)->get_name() <<
              "</td>" << endl;
          generate_field_xsl(*mem_iter);
          f_out_ << "</tr>" << endl;
      }
  }
  f_out_ << "</table></xsl:otherwise></xsl:choose></xsl:template>" <<
      endl << endl;

  f_out_ << "<xsl:template match=\"__" << name << "_list\">" << endl;
  f_out_ << "<h2>" << name << "</h2>" << endl;
  f_out_ << "<table border=\"1\"><tr>" << endl;
  {
      vector<t_field*> members = tsnd->get_members();
      vector<t_field*>::iterator mem_iter = members.begin();
      for ( ; mem_iter != members.end(); mem_iter++) {
          if ((*mem_iter)->get_auto_generated()) continue;
          f_out_ << "<th>" << (*mem_iter)->get_name() << "</th>" << endl;
      }
  }
  f_out_ << "</tr><xsl:apply-templates select=\"" <<
      name << "\"/>" << endl;
  f_out_ << "</table></xsl:template>" << endl << endl;

}

void t_html_generator::generate_struct_xsl(t_struct* tstruct) {
  string name = tstruct->get_name();
  f_out_ << "<xsl:template match=\"" << name << "\">" << endl;
  f_out_ << "<xsl:choose>" << endl;
  f_out_ << "<xsl:when test=\"name(..) = 'list'\"><tr>" << endl;
  vector<t_field*> members = tstruct->get_members();
  vector<t_field*>::iterator mem_iter = members.begin();
  for ( ; mem_iter != members.end(); mem_iter++) {
      generate_field_xsl(*mem_iter);
  }
  f_out_ << "</tr></xsl:when><xsl:otherwise>" << endl;

  mem_iter = members.begin();
  for ( ; mem_iter != members.end(); mem_iter++) {
      f_out_ << "<tr><td valign=\"top\">" << (*mem_iter)->get_name() <<
          "</td>" << endl;
      generate_field_xsl(*mem_iter);
      f_out_ << "</tr>" << endl;
  }
  f_out_ << "</xsl:otherwise></xsl:choose></xsl:template>" << endl << endl;
}

void t_html_generator::generate_xsl() {
  // Make output directory
  MKDIR(get_out_dir().c_str());
  string fname = get_out_dir() + program_->get_name() + ".xsl";
  f_out_.open(fname.c_str());
  f_out_ << "<xsl:stylesheet version=\"1.0\" " <<
      "xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">" << endl;
  f_out_ << "<xsl:output method=\"html\" indent=\"yes\" " <<
      "doctype-public=\"-//W3C//DTD HTML 4.0 Transitional//EN\" />" << endl;

  if (!program_->get_objects().empty()) {
    vector<t_struct*> objects = program_->get_objects();
    vector<t_struct*>::iterator o_iter;
    for (o_iter = objects.begin(); o_iter != objects.end(); ++o_iter) {
      if (!(*o_iter)->is_xception()) {
          generate_struct_xsl(*o_iter);
      }
    }
  }
#ifdef SANDESH
  if (!program_->get_sandeshs().empty()) {
    f_out_ << "<xsl:template match=\"/\"><html><head>" << endl;
    f_out_ << "<title>HTTP Introspect : " << program_->get_name() <<
        "</title>" << endl;
    f_out_ << "</head><body>" << endl;
    f_out_ << endl;
    vector<t_sandesh*> sandeshs = program_->get_sandeshs();
    vector<t_sandesh*>::iterator s_iter;
    for (s_iter = sandeshs.begin(); s_iter != sandeshs.end(); ++s_iter) {
      if (static_cast<const t_base_type*>((*s_iter)->get_type())->get_base() !=
              t_base_type::TYPE_SANDESH_REQUEST) {
          string name = (*s_iter)->get_name();
          f_out_ << "<xsl:apply-templates select=\"" <<
              name << "\"/>" << endl;
          f_out_ << "<xsl:apply-templates select=\"__" << name <<
              "_list\"/>" << endl;
      }
    }
    f_out_ << "</body></html></xsl:template>" << endl << endl;
  }
  if (!program_->get_sandeshs().empty()) {
    vector<t_sandesh*> sandeshs = program_->get_sandeshs();
    vector<t_sandesh*>::iterator s_iter;
    for (s_iter = sandeshs.begin(); s_iter != sandeshs.end(); ++s_iter) {
      if (static_cast<const t_base_type*>((*s_iter)->get_type())->get_base() !=
              t_base_type::TYPE_SANDESH_REQUEST) 
          generate_sandesh_xsl(*s_iter);
    }
  }
#endif
  f_out_ << "</xsl:stylesheet>" << endl;

  f_out_.close();
}

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
  // Make output directory
  MKDIR(get_out_dir().c_str());
  string fname = get_out_dir() + program_->get_name() + ".html";
  f_out_.open(fname.c_str());
  f_out_ << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"" << endl;
  f_out_ << "    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" << endl;
  f_out_ << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl;
  f_out_ << "<head>" << endl;
  f_out_ << "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />" << endl;
  f_out_ << "<link href=\"style.css\" rel=\"stylesheet\" type=\"text/css\"/>"
   << endl;
  f_out_ << "<title>Module: " << program_->get_name()
   << "</title></head><body>" << endl << "<h1>Module: "
   << program_->get_name() << "</h1>" << endl;

  print_doc(program_);

  generate_program_toc();

#ifdef SANDESH
  if (!program_->get_sandeshs().empty()) {
    f_out_ << "<hr/><h2 id=\"Sandeshs\"></h2>" << endl;
    // Generate Sandeshs in declared order
    vector<t_sandesh*> sandeshs = program_->get_sandeshs();
    vector<t_sandesh*>::iterator s_iter;
    for (s_iter = sandeshs.begin(); s_iter != sandeshs.end(); ++s_iter) {
      if (static_cast<const t_base_type*>((*s_iter)->get_type())->get_base() ==
              t_base_type::TYPE_SANDESH_REQUEST) 
          generate_sandesh(*s_iter);
    }
  }
#endif

  f_out_ << "</body></html>\r\n";
  f_out_.close();

  generate_index();
  generate_css();
  generate_xsl();
  generate_xml();
}

/**
 * Emits the index.html file for the recursive set of Thrift programs
 */
void t_html_generator::generate_index() {
  string index_fname = get_out_dir() + "index.html";
  f_out_.open(index_fname.c_str());
  f_out_ << "<html><head>" << endl;
  f_out_ << "<link href=\"style.css\" rel=\"stylesheet\" type=\"text/css\"/>"
   << endl;
  f_out_ << "<title>All Thrift declarations</title></head><body>"
   << endl << "<h1>All Thrift declarations</h1>" << endl;
#ifdef SANDESH
  f_out_ << "<table><tr><th>Module</th><th>Services</th><th>Sandeshs</th>"
   << "<th>Data types</th><th>Constants</th></tr>" << endl;
#else
  f_out_ << "<table><tr><th>Module</th><th>Services</th><th>Data types</th>"
   << "<th>Constants</th></tr>" << endl;
#endif
  vector<t_program*> programs;
  generate_program_toc_rows(program_, programs);
  f_out_ << "</table>" << endl;
  f_out_ << "</body></html>" << endl;
  f_out_.close();
}

void t_html_generator::generate_css() {
  string css_fname = get_out_dir() + "style.css";
  f_out_.open(css_fname.c_str());
  f_out_ << "/* Auto-generated CSS for generated Thrift docs */" << endl;
  f_out_ <<
    "body { font-family: Tahoma, sans-serif; }" << endl;
  f_out_ <<
    "pre { background-color: #dddddd; padding: 6px; }" << endl;
  f_out_ <<
    "h3,h4 { padding-top: 0px; margin-top: 0px; }" << endl;
  f_out_ <<
    "div.definition { border: 1px solid gray; margin: 10px; padding: 10px; }" << endl;
  f_out_ <<
    "div.extends { margin: -0.5em 0 1em 5em }" << endl;
  f_out_ <<
    "table { border: 1px solid grey; border-collapse: collapse; }" << endl;
  f_out_ <<
    "td { border: 1px solid grey; padding: 1px 6px; vertical-align: top; }" << endl;
  f_out_ <<
    "th { border: 1px solid black; background-color: #bbbbbb;" << endl <<
    "     text-align: left; padding: 1px 6px; }" << endl;
  f_out_.close();
}

/**
 * If the provided documentable object has documentation attached, this
 * will emit it to the output stream in HTML format.
 */
void t_html_generator::print_doc(t_doc* tdoc) {
  if (tdoc->has_doc()) {
    string doc = tdoc->get_doc();
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
}

/**
 * Prints out the provided type in HTML
 */
int t_html_generator::print_type(t_type* ttype) {
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
#ifdef SANDESH
    } else if (ttype->is_sandesh()) {
      f_out_ << "Snh_";
#endif
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
void t_html_generator::print_const_value(t_const_value* tvalue) {
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


#ifdef SANDESH
/**
 * Generates a Sandesh definition for a thrift data type.
 *
 * @param tsandesh The Sandesh definition
 */
void t_html_generator::generate_sandesh(t_sandesh* tsandesh) {
  string name = tsandesh->get_name();
  f_out_ << "<div class=\"definition\">";
  f_out_ << "<h3 id=\"Snh_" << name << "\">";
  f_out_ << name << "</h3>" << endl;
  vector<t_field*> members = tsandesh->get_members();
  vector<t_field*>::iterator mem_iter = members.begin();
  f_out_ << "<form action=\"Snh_" << name << "\" method=\"get\">" << endl;
  f_out_ << "<table>";
  f_out_ << "<tr><th>Key</th><th>Field</th><th>Type</th><th>Description</th><th>Requiredness</th><th>Value</th></tr>"
    << endl;
  for ( ; mem_iter != members.end(); mem_iter++) {
    if ((*mem_iter)->get_auto_generated()) continue;

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
    f_out_ << "<input type=\"text\" name=\"";
    f_out_ << (*mem_iter)->get_name();
    f_out_ << "\" size=\"40\"></td></tr>" << endl;
  }
  f_out_ << "</table><button type=\"submit\">Send</button></form><br/>";
  print_doc(tsandesh);
  f_out_ << "</div>";
}
#endif

/**
 * Generates the HTML block for a Thrift service.
 *
 * @param tservice The service definition
 */
void t_html_generator::generate_service(t_service* tservice) {
  f_out_ << "<h3 id=\"Svc_" << service_name_ << "\">Service: "
    << service_name_ << "</h3>" << endl;

  if (tservice->get_extends()) {
    f_out_ << "<div class=\"extends\"><em>extends</em> ";
    print_type(tservice->get_extends());
    f_out_ << "</div>\n";
  }
  print_doc(tservice);
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator fn_iter = functions.begin();
  for ( ; fn_iter != functions.end(); fn_iter++) {
    string fn_name = (*fn_iter)->get_name();
    f_out_ << "<div class=\"definition\">";
    f_out_ << "<h4 id=\"Fn_" << service_name_ << "_" << fn_name
      << "\">Function: " << service_name_ << "." << fn_name
      << "</h4>" << endl;
    f_out_ << "<pre>";
    int offset = print_type((*fn_iter)->get_returntype());
    bool first = true;
    f_out_ << " " << fn_name << "(";
    offset += fn_name.size() + 2;
    vector<t_field*> args = (*fn_iter)->get_arglist()->get_members();
    vector<t_field*>::iterator arg_iter = args.begin();
    if (arg_iter != args.end()) {
      for ( ; arg_iter != args.end(); arg_iter++) {
        if (!first) {
          f_out_ << "," << endl;
          for (int i = 0; i < offset; ++i) {
            f_out_ << " ";
          }
        }
        first = false;
        print_type((*arg_iter)->get_type());
        f_out_ << " " << (*arg_iter)->get_name();
        if ((*arg_iter)->get_value() != NULL) {
          f_out_ << " = ";
          print_const_value((*arg_iter)->get_value());
        }
      }
    }
    f_out_ << ")" << endl;
    first = true;
    vector<t_field*> excepts = (*fn_iter)->get_xceptions()->get_members();
    vector<t_field*>::iterator ex_iter = excepts.begin();
    if (ex_iter != excepts.end()) {
      f_out_ << "    throws ";
      for ( ; ex_iter != excepts.end(); ex_iter++) {
        if (!first) {
          f_out_ << ", ";
        }
        first = false;
        print_type((*ex_iter)->get_type());
      }
      f_out_ << endl;
    }
    f_out_ << "</pre>";
    print_doc(*fn_iter);
    f_out_ << "</div>";
  }
}

THRIFT_REGISTER_GENERATOR(html, "HTML", "")

