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
 * Contains some contributions under the Thrift Software License.
 * Please see doc/old-thrift-license.txt in the Thrift distribution for
 * details.
 */

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <ctype.h>

#include "platform.h"
#include "t_oop_generator.h"

using namespace std;

/* forward declarations */
string initial_caps_to_underscores(string name);
string to_upper_case(string name);
string to_lower_case(string name);

/**
 * C code generator
 */
class t_c_generator : public t_oop_generator {
 public:

  /* constructor */
  t_c_generator(t_program *program,
                const map<string, string> &parsed_options,
                const string &option_string) : t_oop_generator(program)
  {
    (void) parsed_options;
    (void) option_string;
    /* set the output directory */
    this->out_dir_base_ = "gen-c";

    /* set the namespace */
    this->nspace = program_->get_namespace("c");

    if (this->nspace.empty()) {
      this->nspace = "";
      this->nspace_u = "";
      this->nspace_uc = "";
      this->nspace_lc = "";
    } else {
      /* replace dots with underscores */
      char *tmp = strdup(this->nspace.c_str());
      for (unsigned int i = 0; i < strlen(tmp); i++) {
        if (tmp[i] == '.') {
          tmp[i] = '_';
        }
      }
      this->nspace = string(tmp, strlen(tmp));
      free(tmp);

      /* clean up the namespace for C.
       * An input of 'namespace foo' should result in:
       *  - nspace = foo       - for thrift objects and typedefs
       *  - nspace_u = Foo     - for internal GObject prefixes
       *  - nspace_uc = FOO_   - for macro prefixes
       *  - nspace_lc = foo_   - for filename and method prefixes
       * The underscores are there since uc and lc strings are used as file and
       * variable prefixes.
       */
      this->nspace_u = initial_caps_to_underscores(this->nspace);
      this->nspace_uc = to_upper_case(this->nspace_u) + "_";
      this->nspace_lc = to_lower_case(this->nspace_u) + "_";
    }
  }

  /* initialization and destruction */
  void init_generator();
  void close_generator();

  /* generation functions */
  void generate_typedef(t_typedef *ttypedef);
  void generate_enum(t_enum *tenum);
  void generate_consts(vector<t_const *> consts);
  void generate_struct(t_struct *tstruct);
  void generate_service(t_service *tservice);
  void generate_xception(t_struct *tstruct);
  void generate_sandesh(t_sandesh *tsandesh);
  void generate_sandesh_info();

 private:

  /* file streams */
  ofstream f_types_;
  ofstream f_types_impl_;
  ofstream f_header_;
  ofstream f_service_;

  /* namespace variables */
  string nspace;
  string nspace_u;
  string nspace_uc;
  string nspace_lc;

  /* helper functions */
  bool is_complex_type(t_type *ttype);
  string type_name(t_type* ttype, bool in_typedef=false, bool is_const=false);
  string base_type_name(t_base_type *type);
  string type_to_enum(t_type *type);
  string constant_value(string name, t_type *type, t_const_value *value);
  string function_signature(t_function *tfunction);
  string argument_list(t_struct *tstruct);
  string xception_list(t_struct *tstruct);
  string declare_field(t_field *tfield, bool init=false, bool pointer=false, bool constant=false, bool reference=false);

  /* generation functions */
  void generate_const_initializer(string name, t_type *type, t_const_value *value);
  void generate_service_client(t_service *tservice);
  void generate_service_server(t_service *tservice);
  void generate_object(t_struct *tstruct);
  void generate_object(t_sandesh *tsandesh);
  void generate_object_internal(string name, const vector<t_field *> &members, bool is_sandesh);
  void generate_struct_writer(ofstream &out, string name, const vector<t_field *> &fields, bool is_sandesh, bool is_function=true);
  void generate_struct_reader(ofstream &out, string name, const vector<t_field *> &fields, bool is_sandesh, bool is_function=true);
  void generate_struct_deleter(ofstream &out, string name, const vector<t_field *> &fields, bool is_sandesh, bool is_function=true);

  void generate_serialize_field(ofstream &out, t_field *tfield, string prefix, string suffix, int error_ret);
  void generate_serialize_struct(ofstream &out, t_struct *tstruct, string prefix, int error_ret);
  void generate_serialize_container(ofstream &out, t_type *ttype, string prefix, int error_ret);
  void generate_serialize_map_element(ofstream &out, t_map *tmap, string key, string value, int error_ret);
  void generate_serialize_set_element(ofstream &out, t_set *tset, string element, int error_ret);
  void generate_serialize_list_element(ofstream &out, t_list *tlist, string list, string index, int error_ret);

  void generate_deserialize_field(ofstream &out, t_field *tfield, string prefix, string suffix, int error_ret);
  void generate_deserialize_struct(ofstream &out, t_struct *tstruct, string prefix, int error_ret);
  void generate_deserialize_container(ofstream &out, t_type *ttype, string prefix, int error_ret);
  void generate_deserialize_map_element(ofstream &out, t_map *tmap, string prefix, int error_ret);
  void generate_deserialize_set_element(ofstream &out, t_set *tset, string prefix, int error_ret);
  void generate_deserialize_list_element(ofstream &out, t_list *tlist, string prefix, string index, int error_ret);

  string generate_new_hash_from_type(t_type * ttype);
  string generate_new_array_from_type(t_type * ttype); 
};

/**
 * Prepare for file generation by opening up the necessary file
 * output streams.
 */
void t_c_generator::init_generator() {
  /* create output directory */
  MKDIR(get_out_dir().c_str());

  string program_name_u = initial_caps_to_underscores(program_name_);
  string program_name_uc = to_upper_case(program_name_u);
  string program_name_lc = to_lower_case(program_name_u);

  /* create output files */
  string f_types_name = get_out_dir() + this->nspace_lc
                        + program_name_lc + "_types.h";
  f_types_.open(f_types_name.c_str());
  string f_types_impl_name = get_out_dir() + this->nspace_lc
                             + program_name_lc + "_types.c";
  f_types_impl_.open(f_types_impl_name.c_str());

  /* add thrift boilerplate headers */
  f_types_ << autogen_comment();
  f_types_impl_ << autogen_comment();

  /* include inclusion guard */
  f_types_ << 
    "#ifndef " << this->nspace_uc << program_name_uc << "_TYPES_H" << endl <<
    "#define " << this->nspace_uc << program_name_uc << "_TYPES_H" << endl <<
    endl;

  /* include C++ compatibility */
  f_types_ <<
    "#ifdef __cplusplus" << endl <<
    "extern \"C\" {" << endl <<
    "#endif" << endl <<
    endl; 

  /* include base types */
  f_types_ <<
    "/* base includes */" << endl <<
    "#include \"sandesh/library/c/sandesh.h\"" << endl;

  /* include other thrift includes */
  const vector<t_program *> &includes = program_->get_includes();
  for (size_t i = 0; i < includes.size(); ++i) {
    f_types_ <<
      "/* other thrift includes */" << endl <<
      "#include \"" << this->nspace_lc << includes[i]->get_name() <<
          "_types.h\"" << endl;
  }
  f_types_ << endl;

  /* include custom headers */
  const vector<string> &c_includes = program_->get_c_includes();
  f_types_ << "/* custom thrift includes */" << endl;
  for (size_t i = 0; i < c_includes.size(); ++i) {
    if (c_includes[i][0] == '<') {
      f_types_ <<
        "#include " << c_includes[i] << endl;
    } else {
      f_types_ <<
        "#include \"" << c_includes[i] << "\"" << endl;
    }
  }
  f_types_ << endl;

  // include the types file
  f_types_impl_ <<
    endl <<
    "#include \"" << this->nspace_lc << program_name_u << 
        "_types.h\"" << endl <<
    endl;

  f_types_ <<
    "/* begin types */" << endl << endl;
}

/**
 *  Finish up generation and close all file streams.
 */
void t_c_generator::close_generator() {
  string program_name_uc = to_upper_case 
    (initial_caps_to_underscores(program_name_));

  /* end the C++ compatibility */
  f_types_ <<
    "#ifdef __cplusplus" << endl <<
    "}" << endl <<
    "#endif" << endl <<
    endl;

  /* end the header inclusion guard */
  f_types_ <<
    "#endif /* " << this->nspace_uc << program_name_uc << "_TYPES_H */" << endl;

  /* close output file */
  f_types_.close();
  f_types_impl_.close();
}

/**
 * Generates a Thrift typedef in C code.  For example:
 * 
 * Thrift: 
 * typedef map<i32,i32> SomeMap
 * 
 * C: 
 * typedef GHashTable * ThriftSomeMap;
 */
void t_c_generator::generate_typedef(t_typedef* ttypedef) {
  f_types_ <<
    indent() << "typedef " << type_name(ttypedef->get_type(), true) <<
        " " << this->nspace << ttypedef->get_symbolic() << ";" << endl <<
    endl;
} 

/**
 * Generates a C enumeration.  For example:
 *
 * Thrift:
 * enum MyEnum {
 *   ONE = 1,
 *   TWO
 * }
 *
 * C:
 * enum _ThriftMyEnum {
 *   THRIFT_MY_ENUM_ONE = 1,
 *   THRIFT_MY_ENUM_TWO
 * };
 * typedef enum _ThriftMyEnum ThriftMyEnum;
 */
void t_c_generator::generate_enum(t_enum *tenum) {
  string name = tenum->get_name();
  string name_uc = to_upper_case(initial_caps_to_underscores(name));

  f_types_ <<
    indent() << "enum _" << this->nspace << name << " {" << endl;

  indent_up();

  vector<t_enum_value *> constants = tenum->get_constants();
  vector<t_enum_value *>::iterator c_iter;
  bool first = true;

  /* output each of the enumeration elements */
  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    if (first) {
      first = false;
    } else {
      f_types_ << "," << endl;
    }

    f_types_ <<
      indent() << this->nspace_uc << name_uc << "_" << (*c_iter)->get_name();
    if ((*c_iter)->has_value()) {
      f_types_ <<
        " = " << (*c_iter)->get_value();
    }
  }

  indent_down();
  f_types_ <<
    endl <<
    "};" << endl <<
    "typedef enum _" << this->nspace << name << " " << this->nspace << name << ";" << endl <<
    endl;
}

/**
 * Generates Thrift constants in C code.
 */
void t_c_generator::generate_consts (vector<t_const *> consts) {
  f_types_ << "/* constants */" << endl;
  f_types_impl_ << "/* constants */" << endl;

  vector<t_const *>::iterator c_iter;
  for (c_iter = consts.begin(); c_iter != consts.end(); ++c_iter) {
    string name = (*c_iter)->get_name();
    string name_uc = to_upper_case(name);
    string name_lc = to_lower_case(name);
    t_type *type = (*c_iter)->get_type();
    t_const_value *value = (*c_iter)->get_value();

    f_types_ <<
      indent() << "#define " << this->nspace_uc << name_uc << " " <<
          constant_value (name_lc, type, value) << endl;

    generate_const_initializer (name_lc, type, value);
  }

  f_types_ << endl;
  f_types_impl_ << endl;
}

/**
 * Generate Thrift structs in C code.  Example:
 *
 * Thrift:
 * struct Bonk
 * {
 *   1: string message,
 *   2: i32 type
 * }
 *
 * C instance header:
 * struct _Bonk
 * {
 *
 *   char * message;
 *   int32 type;
 * };
 * typedef struct _Bonk
 */
void t_c_generator::generate_struct (t_struct *tstruct) {
  f_types_ << "/* struct " << tstruct->get_name() << " */" << endl;
  generate_object(tstruct);
}

/**
 * Generate C code to represent Sandesh.
 */
void t_c_generator::generate_sandesh (t_sandesh *tsandesh) {
  f_types_ << "/* sandesh " << tsandesh->get_name() <<  " */" << endl;
  generate_object(tsandesh);
}

/**
 * Generate C code to represent Sandesh information for all Sandeshs
 */
void t_c_generator::generate_sandesh_info () {
  string program_name_u = initial_caps_to_underscores(program_name_);
  string program_name_lc = to_lower_case(program_name_u);
  ofstream &out = f_types_impl_;
  out << endl <<
      "static sandesh_info_t " << program_name_lc << "_infos[] = {" << endl;
  vector<t_sandesh*> sandeshs = program_->get_sandeshs();
  vector<t_sandesh*>::iterator s_iter;
  indent_up();
  for (s_iter = sandeshs.begin(); s_iter != sandeshs.end(); ++s_iter) {
    string sandesh_name = get_sandesh_name(*s_iter);
    string sandesh_name_u = initial_caps_to_underscores(sandesh_name);
    out << indent() << "{" << endl;
    indent_up();
    out << indent() << ".name = \"" << sandesh_name << "\"," << endl;
    out << indent() << ".size = sizeof(" << sandesh_name << ")," << endl;
    out << indent() << ".read = " << sandesh_name_u << "_read," << endl;
    out << indent() << ".write = " << sandesh_name_u << "_write," << endl;
    out << indent() << ".process = " << sandesh_name_u << "_process," << endl;
    out << indent() << ".free = " << sandesh_name_u << "_free," << endl;
    indent_down();
    out << indent() << "}," << endl;
  }
  // Add end of array marker
  out << indent() << "{" << endl;
  indent_up();
  out << indent() << ".name = NULL," << endl;
  out << indent() << ".size = 0," << endl;
  out << indent() << ".read = NULL," << endl;
  out << indent() << ".write = NULL," << endl;
  out << indent() << ".process = NULL," << endl;
  out << indent() << ".free = NULL," << endl;
  indent_down();
  out << indent() << "}, " << endl;
  indent_down();
  out << "};" << endl << endl;

  // Generate the extract function
  out << "sandesh_info_t *" << endl
      << program_name_lc <<
      "_find_sandesh_info (const char *sname)" << endl;
  out << "{" << endl;
  indent_up();
  out << indent() << "return sandesh_find_info(" << program_name_lc <<
      "_infos, sname);" << endl;
  indent_down();
  out << "}" << endl << endl;

  // Generate the extract function declaration
    f_types_ << endl <<
      "sandesh_info_t * " << program_name_lc <<
      "_find_sandesh_info(const char *sname);" << endl;
}

/**
 * Generate C code to represent Thrift services.  Creates a new GObject
 * which can be used to access the service.
 */
void t_c_generator::generate_service (t_service *tservice) {
  // NOT IMPLEMENTED
}

/**
 *
 */
void t_c_generator::generate_xception (t_struct *tstruct) {
  // NOT IMPLEMENTED
}

/********************
 * HELPER FUNCTIONS *
 ********************/

/**
 * Returns true if ttype is not a primitive.
 */
bool t_c_generator::is_complex_type(t_type *ttype) {
  ttype = get_true_type (ttype);

  return ttype->is_container()
         || ttype->is_struct()
         || ttype->is_xception()
         || (ttype->is_base_type()
             && (((t_base_type *) ttype)->get_base()
                  == t_base_type::TYPE_STRING ||
                 ((t_base_type *) ttype)->get_base()
                  == t_base_type::TYPE_XML));
}


/**
 * Maps a Thrift t_type to a C type.
 */
string t_c_generator::type_name (t_type* ttype, bool in_typedef, bool is_const) {
  (void) in_typedef;
  if (ttype->is_base_type()) {
    string bname = base_type_name ((t_base_type *) ttype);

    if (is_const) {
      return "const " + bname;
    } else {
      return bname;
    }
  }

  if (ttype->is_container()) {
    string cname;

    t_container *tcontainer = (t_container *) ttype;
    if (tcontainer->has_cpp_name()) {
      cname = tcontainer->get_cpp_name();
    } else if (ttype->is_map()) {
      throw "compiler error: no C support for map";
    } else if (ttype->is_set()) {
      throw "compiler error: no C support for set";
    } else if (ttype->is_list()) {
      t_type *etype = ((t_list *) ttype)->get_elem_type();
      if (etype->is_base_type()) {
        cname = base_type_name((t_base_type *) etype) + " *";
      } else {
        cname = type_name(etype, in_typedef, is_const);
      }
    }

    if (is_const) {
      return "const " + cname;
    } else {
      return cname;
    }
  }

  // check for a namespace
  string pname = this->nspace + ttype->get_name();

  if (is_complex_type (ttype)) {
    pname += " *";
  }

  if (is_const) {
    return "const " + pname;
  } else {
    return pname;
  }
}

/**
 * Maps a Thrift primitive to a C primitive.
 */
string t_c_generator::base_type_name(t_base_type *type) {
  t_base_type::t_base tbase = type->get_base();

  switch (tbase) {
    case t_base_type::TYPE_VOID:
      return "void";
    case t_base_type::TYPE_STRING:
      if (type->is_binary()) {
        return "u_int8_t *";
      } else {
        return "char *";
      }
    case t_base_type::TYPE_XML:
      return "char *";
    case t_base_type::TYPE_BOOL:
      return "u_int8_t";
    case t_base_type::TYPE_BYTE:
      return "int8_t";
    case t_base_type::TYPE_I16:
      return "int16_t";
    case t_base_type::TYPE_I32:
      return "int32_t";
    case t_base_type::TYPE_I64:
      return "int64_t";
    case t_base_type::TYPE_U16:
      return "uint16_t";
    case t_base_type::TYPE_U32:
      return "uint32_t";
    case t_base_type::TYPE_U64:
      return "uint64_t";
    case t_base_type::TYPE_DOUBLE:
      return "double";
    default:
      throw "compiler error: no C base type name for base type "
            + t_base_type::t_base_name (tbase);
  }
}

/**
 * Returns a member of the ThriftType C enumeration in thrift_protocol.h
 * for a Thrift type.
 */
string t_c_generator::type_to_enum (t_type *type) {
  type = get_true_type (type);

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type *) type)->get_base();

    switch (tbase) {
      case t_base_type::TYPE_VOID:
        throw "NO T_VOID CONSTRUCT";
      case t_base_type::TYPE_STRING:
      case t_base_type::TYPE_STATIC_CONST_STRING:
      case t_base_type::TYPE_SANDESH_SYSTEM:
      case t_base_type::TYPE_SANDESH_REQUEST:
      case t_base_type::TYPE_SANDESH_RESPONSE:
      case t_base_type::TYPE_SANDESH_TRACE:
      case t_base_type::TYPE_SANDESH_TRACE_OBJECT:
      case t_base_type::TYPE_SANDESH_BUFFER:
      case t_base_type::TYPE_SANDESH_UVE:
      case t_base_type::TYPE_SANDESH_OBJECT:
      case t_base_type::TYPE_SANDESH_FLOW:
        return "T_STRING";
      case t_base_type::TYPE_BOOL:
        return "T_BOOL";
      case t_base_type::TYPE_BYTE:
        return "T_BYTE";
      case t_base_type::TYPE_I16:
        return "T_I16";
      case t_base_type::TYPE_I32:
        return "T_I32";
      case t_base_type::TYPE_I64:
        return "T_I64";
      case t_base_type::TYPE_U16:
        return "T_U16";
      case t_base_type::TYPE_U32:
        return "T_U32";
      case t_base_type::TYPE_U64:
        return "T_U64";
      case t_base_type::TYPE_XML:
        return "T_XML";
      case t_base_type::TYPE_DOUBLE:
        return "T_DOUBLE";
    }
  } else if (type->is_enum()) {
    return "T_I32";
  } else if (type->is_struct()) {
    return "T_STRUCT";
  } else if (type->is_xception()) {
    return "T_STRUCT";
  } else if (type->is_map()) {
    return "T_MAP";
  } else if (type->is_set()) {
    return "T_SET";
  } else if (type->is_list()) {
    return "T_LIST";
  }

  throw "INVALID TYPE IN type_to_enum: " + type->get_name();
}


/**
 * Returns C code that represents a Thrift constant.
 */
string t_c_generator::constant_value(string name, t_type *type, t_const_value *value) {
  ostringstream render;

  if (type->is_base_type()) {
    /* primitives */
    t_base_type::t_base tbase = ((t_base_type *) type)->get_base();
    switch (tbase) {
      case t_base_type::TYPE_STRING:
      case t_base_type::TYPE_XML:
        render << "\"" + value->get_string() + "\"";
        break;
      case t_base_type::TYPE_BOOL:
        render << ((value->get_integer() != 0) ? 1 : 0);
        break;
      case t_base_type::TYPE_BYTE:
      case t_base_type::TYPE_I16:
      case t_base_type::TYPE_I32:
      case t_base_type::TYPE_I64:
      case t_base_type::TYPE_U16:
      case t_base_type::TYPE_U32:
      case t_base_type::TYPE_U64:
        render << value->get_integer();
        break;
      case t_base_type::TYPE_DOUBLE:
        if (value->get_type() == t_const_value::CV_INTEGER) {
          render << value->get_integer();
        } else {
          render << value->get_double();
        }
        break;
      default:
        throw "compiler error: no const of base type "
              + t_base_type::t_base_name (tbase);
    }
  } else if (type->is_enum()) {
    render << "(" << type_name (type) << ")" << value->get_integer();
  } else if (type->is_struct() || type->is_xception() || type->is_list()
             || type->is_set() || type->is_map()) {
    render << "(" << this->nspace_lc <<
      to_lower_case(name) << "_constant())";
  } else {
    render << "NULL /* not supported */";
  }

  return render.str();
}

/**
 * Renders a function signature of the form 'type name(args)'
 *
 * @param tfunction Function definition
 * @return String of rendered function definition
 */
string t_c_generator::function_signature(t_function* tfunction) {
  t_type* ttype = tfunction->get_returntype();
  t_struct* arglist = tfunction->get_arglist();
  t_struct* xlist = tfunction->get_xceptions();
  string fname = initial_caps_to_underscores(tfunction->get_name());

  bool has_return = !ttype->is_void();
  bool has_args = arglist->get_members().size() == 0;
  bool has_xceptions = xlist->get_members().size() == 0;
  return
    "gboolean " + this->nspace_lc + fname + " (" + this->nspace
    + service_name_ + "If * iface"
    + (has_return ? ", " + type_name(ttype) + "* _return" : "")
    + (has_args ? "" : (", " + argument_list (arglist))) 
    + (has_xceptions ? "" : (", " + xception_list (xlist)))
    + ", GError ** error)";
}

/**
 * Renders a field list
 *
 * @param tstruct The struct definition
 * @return Comma sepearated list of all field names in that struct
 */
string t_c_generator::argument_list (t_struct* tstruct) {
  string result = "";

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  bool first = true;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
    } else {
      result += ", ";
    }
    result += type_name((*f_iter)->get_type(), false, true) + " " +
              (*f_iter)->get_name();
  }
  return result;
}

/**
 * Renders mutable exception lists
 *
 * @param tstruct The struct definition
 * @return Comma sepearated list of all field names in that struct
 */
string t_c_generator::xception_list (t_struct* tstruct) {
  string result = "";

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  bool first = true;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
    } else {
      result += ", ";
    }
    result += type_name((*f_iter)->get_type(), false, false) + "* " +
              (*f_iter)->get_name();
  }
  return result;
}


/**
 * Declares a field, including any necessary initialization.
 */
string t_c_generator::declare_field(t_field *tfield,
                                         bool init,
                                         bool pointer,
                                         bool constant,
                                         bool reference) {
  string result = "";
  if (constant) {
    result += "const ";
  }
  result += type_name(tfield->get_type());
  if (pointer) {
    result += "*";
  }
  if (reference) {
    result += "*";
  }
  result += " " + tfield->get_name();
  if (init) {
    t_type* type = get_true_type(tfield->get_type());

    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type *) type)->get_base();
      switch (tbase) {
        case t_base_type::TYPE_VOID:
          break;
        case t_base_type::TYPE_BOOL:
        case t_base_type::TYPE_BYTE:
        case t_base_type::TYPE_I16:
        case t_base_type::TYPE_I32:
        case t_base_type::TYPE_I64:
        case t_base_type::TYPE_U16:
        case t_base_type::TYPE_U32:
        case t_base_type::TYPE_U64:
          result += " = 0";
          break;
        case t_base_type::TYPE_DOUBLE:
          result += " = (gdouble) 0";
          break;
        case t_base_type::TYPE_STRING:
        case t_base_type::TYPE_XML:
          result += " = NULL";
          break;
        default:
          throw "compiler error: no C intializer for base type "
                + t_base_type::t_base_name (tbase);
      }
    } else if (type->is_enum()) {
      result += " = (" + type_name (type) + ") 0";
    }
  }

  if (!reference) {
    result += ";";
  }

  return result;
}

/**
 * Generates C code that initializes complex constants.
 */
void t_c_generator::generate_const_initializer(string name, t_type *type, t_const_value *value) {
  string name_u = initial_caps_to_underscores(name);
  string name_lc = to_lower_case(name_u);
  string type_u = initial_caps_to_underscores(type->get_name());
  string type_uc = to_upper_case(type_u);

  if (type->is_struct() || type->is_xception()) {
    throw "compiler error: no support for const struct or exception";
  } else if (type->is_list()) {
    throw "compiler error: no support for const array";
  } else if (type->is_set()) {
      throw "compiler error: no support for const set";
  } else if (type->is_map()) {
      throw "compiler error: no support for const map";
  }
}

/**
 * Generates C code that represents a Thrift service client.
 */
void t_c_generator::generate_service_client(t_service *tservice) {
  // NOT IMPLEMENTED
}

/**
 * Generates C code that represents a Thrift service server.
 */
void t_c_generator::generate_service_server(t_service *tservice) {
  // NOT IMPLEMENTED
}

/**
 * Generates C code to represent a sandesh.
 */
void t_c_generator::generate_object(t_sandesh *tsandesh) {
  string name = tsandesh->get_name();
  generate_object_internal(name, tsandesh->get_members(), true);
}

/**
 * Generates C code to represent a struct.
 */
void t_c_generator::generate_object(t_struct *tstruct) {
  string name = tstruct->get_name();
  generate_object_internal(name, tstruct->get_members(), false);
}

/**
 * Generates C code to represent a structure.
 */
void t_c_generator::generate_object_internal(string name,
                                             const vector<t_field *> &members,
                                             bool is_sandesh) {
  string name_u = initial_caps_to_underscores(name);
  string name_uc = to_upper_case(name_u);
  string dtname = is_sandesh ? "void" : name;
  string dvname = is_sandesh ? "sandesh" : name_u;

  // write the instance definition
  f_types_ <<
    "struct _" << this->nspace << name << endl <<
    "{ " << endl <<
    "  /* public */" << endl;

  // for each field, add a member variable
  vector<t_field *>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    // Ignore auto generated fields
    if ((*m_iter)->get_auto_generated()) {
      continue;
    }
    t_type *t = get_true_type ((*m_iter)->get_type());
    f_types_ <<
      "  " << type_name (t) << " " << (*m_iter)->get_name() << ";" << endl;
    if (t->is_list()) {
      // Auto-generate size for the list
      f_types_ << "  " << "u_int32_t " << (*m_iter)->get_name() << "_size;"
        << endl;
    }
    if ((*m_iter)->get_req() != t_field::T_REQUIRED) {
      f_types_ <<
        "  u_int8_t __isset_" << (*m_iter)->get_name() << ";" << endl;
    }
  }

  // close the structure definition and create a typedef
  f_types_ <<
    "};" << endl <<
    "typedef struct _" << this->nspace << name << " " << 
        this->nspace << name << ";" << endl <<
      endl;

  // prototypes for struct I/O methods
  f_types_ <<
    "int32_t " <<
    this->nspace_lc << name_u <<
    "_write (" << dtname <<
    "* w" << dvname << ", ThriftProtocol *protocol, int *error);" << endl;
  f_types_ <<
    "int32_t " <<
    this->nspace_lc << name_u <<
    "_read (" << dtname <<
    "* r" << dvname << ", ThriftProtocol *protocol, int *error);" << endl;
  f_types_ <<
    "void " <<
    this->nspace_lc << name_u <<
    "_free(" << dtname <<
    "* f" << dvname << ");" << endl;
  if (is_sandesh) {
    f_types_ <<
      "void " <<
      this->nspace_lc << name_u <<
      "_process (void *p" << name_u << ");" << endl;
  }

  // start writing the object implementation .c file
  // generate struct I/O methods
  generate_struct_reader (f_types_impl_, name, members, is_sandesh);
  generate_struct_writer (f_types_impl_, name, members, is_sandesh);
  generate_struct_deleter (f_types_impl_, name, members, is_sandesh);
}

/**
 * Generates functions to free Thrift structures and sandeshs to a stream.
 */
void t_c_generator::generate_struct_deleter (ofstream &out,
                                             string name,
                                             const vector<t_field *> &fields,
                                             bool is_sandesh,
                                             bool is_function) {
  string name_u = initial_caps_to_underscores(name);
  string fname = "f" + name_u;
  string sname = is_sandesh ? "sandesh" : "struct";
  string dname = is_sandesh ? "void" : name;
  string dfname = is_sandesh ? "fsandesh" : fname;

  vector <t_field *>::const_iterator f_iter;

  if (is_function) {
    indent(out) <<
      "void" << endl <<
      this->nspace_lc << name_u <<
      "_free (" << dname <<
      "* " << dfname << ")" << endl;
  }
  indent(out) << "{" << endl;
  indent_up();

  if (is_sandesh) {
    out << indent() << name << "* " << fname << " = " <<
      "(" << name << "* ) " << dfname << ";" <<
      endl << endl;
  }

  // satisfy -Wall in case we don't use some variables
  out <<
    indent() << "/* satisfy -Wall in case these aren't used */" << endl <<
    indent() << "THRIFT_UNUSED_VAR (" << fname << ");" << endl;

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    // Ignore auto generated fields
    if ((*f_iter)->get_auto_generated()) {
      continue;
    }
    t_type *type = get_true_type ((*f_iter)->get_type());
    string fvname = fname + "->" + (*f_iter)->get_name();
    if (type->is_struct() || type->is_xception()) {
      indent(out) << "if (" << fvname << ") {" << endl;
      indent_up();
      indent(out) << ((t_struct *)type)->get_name() << "_free(" <<
        fvname << ");" << endl;
      indent(out) << "os_free(" << fvname << ");" << endl;
      indent(out) << fvname << " = NULL;" << endl;
      scope_down(out);
    } else if (type->is_container()) {
      indent(out) << "if (" << fvname << ") {" << endl;
      indent_up();
      indent(out) << "os_free(" << fvname << ");" << endl;
      indent(out) << fvname << "_size = 0;" << endl;
      indent(out) << fvname << " = NULL;" << endl;
      scope_down(out);
    } else if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type *) type)->get_base();
      switch (tbase) {
      case t_base_type::TYPE_STRING:
      case t_base_type::TYPE_XML:
        indent(out) << "if (" << fvname << ") {" << endl;
        indent_up();
        indent(out) << "os_free(" << fvname << ");" << endl;
        indent(out) << fvname << " = NULL;" << endl;
        scope_down(out);
        break;

      default:
        break;
      }
    }
  }
  indent_down();
  indent(out) <<
    "}" << endl <<
    endl;
}

/**
 * Generates functions to write Thrift structures and sandeshs to a stream.
 */
void t_c_generator::generate_struct_writer (ofstream &out,
                                            string name,
                                            const vector<t_field *> &fields,
                                            bool is_sandesh,
                                            bool is_function) {
  string name_u = initial_caps_to_underscores(name);
  string wname = "w" + name_u;
  string sname = is_sandesh ? "sandesh" : "struct";
  string dname = is_sandesh ? "void" : name;
  string dwname = is_sandesh ? "wsandesh" : wname;

  vector <t_field *>::const_iterator f_iter;
  int error_ret = 0;

  if (is_function) {
    error_ret = -1;
    indent(out) <<
      "int32_t" << endl <<
      this->nspace_lc << name_u <<
      "_write (" << dname <<
      "* " << dwname << ", ThriftProtocol *protocol, int *error)" << endl;
  }
  indent(out) << "{" << endl;
  indent_up();

  out <<
    indent() << "int32_t ret;" << endl <<
    indent() << "int32_t xfer = 0;" << endl <<
    indent() << "u_int32_t i;" << endl <<
    endl;

  if (is_sandesh) {
    out << indent() << name << "* " << wname << " = " <<
      "(" << name << "* ) " << dwname << ";" <<
      endl << endl;
  }

  // satisfy -Wall in case we don't use some variables
  out <<
    indent() << "/* satisfy -Wall in case these aren't used */" << endl <<
    indent() << "THRIFT_UNUSED_VAR (i);" << endl <<
    indent() << "THRIFT_UNUSED_VAR (" << wname << ");" << endl;

  out << endl;

  out <<
    indent() << "if ((ret = thrift_protocol_write_" << sname << "_begin (protocol, \"" << name << "\", error)) < 0)" << endl <<
    indent() << "  return " << error_ret << ";" << endl <<
    indent() << "xfer += ret;" << endl;

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    // Ignore auto generated fields
    if ((*f_iter)->get_auto_generated()) {
      continue;
    }
    if ((*f_iter)->get_req() == t_field::T_OPTIONAL) {
      indent(out) << "if (" << wname << "->__isset_" << (*f_iter)->get_name() << " == 1) {" << endl;
      indent_up();
    } 

    out <<
     indent() << "if ((ret = thrift_protocol_write_field_begin (protocol, " <<
     "\"" << (*f_iter)->get_name() << "\", " <<
     type_to_enum ((*f_iter)->get_type()) << ", " <<
     (*f_iter)->get_key() << ", error)) < 0)" << endl <<
     indent() << "  return " << error_ret << ";" << endl <<
     indent() << "xfer += ret;" << endl;
    generate_serialize_field (out, *f_iter, wname + "->", "", error_ret);
    out <<
      indent() << "xfer += ret;" << endl;
    out <<
      indent() << "if ((ret = thrift_protocol_write_field_end (protocol, error)) < 0)" << endl <<
      indent() << "  return " << error_ret << ";" << endl <<
      indent() << "xfer += ret;" << endl;

    if ((*f_iter)->get_req() == t_field::T_OPTIONAL) {
      indent_down();
      indent(out) << "}" << endl;
    }
  }

  // write the struct map
  out <<
    indent() << "if ((ret = thrift_protocol_write_field_stop (protocol, error)) < 0)" << endl <<
    indent() << "  return " << error_ret << ";" << endl <<
    indent() << "xfer += ret;" << endl <<
    indent() << "if ((ret = thrift_protocol_write_" << sname << "_end (protocol, error)) < 0)" << endl <<
    indent() << "  return " << error_ret << ";" << endl <<
    indent() << "xfer += ret;" << endl <<
    endl;

  if (is_function) {
    indent(out) << "return xfer;" << endl;
  }

  indent_down();
  indent(out) <<
    "}" << endl <<
    endl;
}

/**
 * Generates code to read Thrift structures from a stream.
 */
void t_c_generator::generate_struct_reader(ofstream &out,
                                           string name,
                                           const vector <t_field *> &fields,
                                           bool is_sandesh,
                                           bool is_function) {
  string name_u = initial_caps_to_underscores(name);
  string rname = "r" + name_u;
  int error_ret = 0;
  vector <t_field *>::const_iterator f_iter;
  string sname = is_sandesh ? "sandesh" : "struct";
  string dname = is_sandesh ? "void" : name;
  string drname = is_sandesh ? "rsandesh" : rname;

  if (is_function) {
    error_ret = -1;
    indent(out) <<
      "/* reads a " << name_u << " " << sname << " */" << endl <<
      "int32_t" << endl <<
      this->nspace_lc << name_u <<
          "_read (" << dname <<
          " *" << drname << ", ThriftProtocol *protocol, int *error)" << endl;
  }

  indent(out) << "{" << endl;
  indent_up();

  // declare stack temp variables
  out <<
    indent() << "int32_t ret;" << endl <<
    indent() << "int32_t xfer = 0;" << endl <<
    indent() << "char *name;" << endl <<
    indent() << "ThriftType ftype;" << endl <<
    indent() << "int16_t fid;" << endl <<
    indent() << "u_int32_t len = 0, i;" << endl <<
    indent() << "void *data = NULL;" << endl;

  if (is_sandesh) {
    out << indent() << name << "* " << rname << " = " <<
      "(" << name << "* ) " << drname << ";" << endl;
  }

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    // Ignore auto generated fields
    if ((*f_iter)->get_auto_generated()) {
      continue;
    }
    if ((*f_iter)->get_req() == t_field::T_REQUIRED) {
      indent(out) << "u_int8_t isset_" << (*f_iter)->get_name() << " = 0;" << endl;
    }
  }

  // satisfy -Wall in case we don't use some variables
  out <<
    indent() << "/* satisfy -Wall in case these aren't used */" << endl <<
    indent() << "THRIFT_UNUSED_VAR (len);" << endl <<
    indent() << "THRIFT_UNUSED_VAR (data);" << endl <<
    indent() << "THRIFT_UNUSED_VAR (i);" << endl <<
    indent() << "THRIFT_UNUSED_VAR (" << rname << ");" << endl;

  out << endl;

  // read the beginning of the structure marker
  out <<
    indent() << "/* read the " << sname << " begin marker */" << endl <<
    indent() << "if ((ret = thrift_protocol_read_" << sname << "_begin (protocol, &name, error)) < 0)" << endl <<
    indent() << "{" << endl <<
    indent() << "  if (name) os_free (name);" << endl <<
    indent() << "  return " << error_ret << ";" << endl <<
    indent() << "}" << endl <<
    indent() << "xfer += ret;" << endl <<
    indent() << "if (name) { os_free (name); name = NULL; }" << endl <<
    endl;

  // read the struct fields
  out <<
    indent() << "/* read the struct fields */" << endl <<
    indent() << "while (1)" << endl;
  scope_up(out);

  // read beginning field marker
  out <<
    indent() << "/* read the beginning of a field */" << endl <<
    indent() << "if ((ret = thrift_protocol_read_field_begin (protocol, &name, &ftype, &fid, error)) < 0)" << endl <<
    indent() << "{" << endl <<
    indent() << "  if (name) os_free (name);" << endl <<
    indent() << "  return " << error_ret << ";" << endl <<
    indent() << "}" << endl <<
    indent() << "xfer += ret;" << endl <<
    indent() << "if (name) { os_free (name); name = NULL; }" << endl <<
    endl;

  // check for field STOP marker
  out <<
    indent() << "/* break if we get a STOP field */" << endl <<
    indent() << "if (ftype == T_STOP)" << endl <<
    indent() << "{" << endl <<
    indent() << "  break;" << endl <<
    indent() << "}" << endl <<
    endl;

  // switch depending on the field type
  indent(out) <<
    "switch (fid)" << endl;

  // start switch
  scope_up(out);

  // generate deserialization code for known types
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    // Ignore auto generated fields
    if ((*f_iter)->get_auto_generated()) {
      continue;
    }
    indent(out) <<
      "case " << (*f_iter)->get_key() << ":" << endl;
    indent_up();
    indent(out) <<
      "if (ftype == " << type_to_enum ((*f_iter)->get_type()) << ")" << endl;
    indent(out) <<
      "{" << endl;


    indent_up();
    // generate deserialize field
    generate_deserialize_field (out, *f_iter, rname + "->", "", error_ret);
    indent_down();

    out <<
      indent() << "} else {" << endl <<
      indent() << "  if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)" << endl <<
      indent() << "    return " << error_ret << ";" << endl <<
      indent() << "  xfer += ret;" << endl <<
      indent() << "}" << endl <<
      indent() << "break;" << endl;
    indent_down();
  }

  // create the default case
  out <<
    indent() << "default:" << endl <<
    indent() << "  if ((ret = thrift_protocol_skip (protocol, ftype, error)) < 0)" << endl <<
    indent() << "    return " << error_ret << ";" << endl <<
    indent() << "  xfer += ret;" << endl <<
    indent() << "  break;" << endl;

  // end switch
  scope_down(out);

  // read field end marker
  out <<
    indent() << "if ((ret = thrift_protocol_read_field_end (protocol, error)) < 0)" << endl <<
    indent() << "  return " << error_ret << ";" << endl <<
    indent() << "xfer += ret;" << endl;

  // end while loop
  scope_down(out);
  out << endl;

  // read the end of the structure
  out <<
    indent() << "if ((ret = thrift_protocol_read_" << sname << "_end (protocol, error)) < 0)" << endl <<
    indent() << "  return " << error_ret << ";" << endl <<
    indent() << "xfer += ret;" << endl <<
    endl;

  // if a required field is missing, throw an error
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    // Ignore auto generated fields
    if ((*f_iter)->get_auto_generated()) {
      continue;
    }
    if ((*f_iter)->get_req() == t_field::T_REQUIRED) {
      out <<
        indent() << "if (!isset_" << (*f_iter)->get_name() << ")" << endl <<
        indent() << "{" << endl <<
        indent() << "  *error = THRIFT_PROTOCOL_ERROR_INVALID_DATA;" << endl <<
        indent() << "  syslog(LOG_ERROR, \"THRIFT_PROTOCOL_ERROR: missing field\");" << endl <<
        indent() << "  return -1;" << endl <<
        indent() << "}" << endl <<
        endl;
    }
  }

  if (is_function) {
    indent(out) <<
      "return xfer;" << endl;
  }

  // end the function/structure
  indent_down();
  indent(out) <<
    "}" << endl <<
    endl;
}

void t_c_generator::generate_serialize_field(ofstream &out,
                                                  t_field *tfield,
                                                  string prefix,
                                                  string suffix,
                                                  int error_ret) {
  t_type *type = get_true_type (tfield->get_type());
  string name = prefix + tfield->get_name() + suffix;

  if (type->is_void()) {
    throw "CANNOT GENERATE SERIALIZE CODE FOR void TYPE: " + name;
  }

  if (type->is_struct() || type->is_xception()) {
    generate_serialize_struct (out, (t_struct *) type, name, error_ret);
  } else if (type->is_container()) {
    generate_serialize_container (out, type, name, error_ret);
  } else if (type->is_base_type() || type->is_enum()) {
    indent(out) <<
      "if ((ret = thrift_protocol_write_";

    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type *) type)->get_base();
      switch (tbase) {
        case t_base_type::TYPE_VOID:
          throw "compiler error: cannot serialize void field in a struct: "
                + name;
          break;
        case t_base_type::TYPE_BOOL:
          out << "bool (protocol, " << name;
          break;
        case t_base_type::TYPE_BYTE:
          out << "byte (protocol, " << name;
          break;
        case t_base_type::TYPE_I16:
          out << "i16 (protocol, " << name;
          break;
        case t_base_type::TYPE_I32:
          out << "i32 (protocol, " << name;
          break;
        case t_base_type::TYPE_I64:
          out << "i64 (protocol, " << name;
          break;
        case t_base_type::TYPE_U16:
          out << "u16 (protocol, " << name;
          break;
        case t_base_type::TYPE_U32:
          out << "u32 (protocol, " << name;
          break;
        case t_base_type::TYPE_U64:
          out << "u64 (protocol, " << name;
          break;
        case t_base_type::TYPE_DOUBLE:
          out << "double (protocol, " << name;
          break;
        case t_base_type::TYPE_STRING:
          if (((t_base_type *) type)->is_binary()) {
            out << "binary (protocol, ((GByteArray *) " << name <<
                   ")->data, ((GByteArray *) " << name <<
                   ")->len";
          } else {
            out << "string (protocol, " << name;
          }
          break;
        case t_base_type::TYPE_XML:
          out << "xml (protocol, " << name;
          break;
        default:
          throw "compiler error: no C writer for base type "
                + t_base_type::t_base_name (tbase) + name;
      }
    } else if (type->is_enum()) {
      out << "i32 (protocol, " << name;
    }
    out << ", error)) < 0)" << endl <<
      indent() << "  return " << error_ret << ";" << endl;
  } else {
    printf ("DO NOT KNOW HOW TO SERIALIZE FIELD '%s' TYPE '%s'\n",
            name.c_str(), type_name (type).c_str());
  }
}

void t_c_generator::generate_serialize_struct(ofstream &out,
                                                   t_struct *tstruct,
                                                   string prefix,
                                                   int error_ret) {
  out <<
    indent() << "if ((ret = " << tstruct->get_name() << "_write (" << prefix << " , protocol, error)) < 0)" << endl <<
    indent() << "  return " << error_ret << ";" << endl <<
    endl;
}

void t_c_generator::generate_serialize_container(ofstream &out,
                                                      t_type *ttype,
                                                      string prefix,
                                                      int error_ret) {
  scope_up(out);

  if (ttype->is_map()) {
    throw "compiler error: serialize of map not supported in C";
  } else if (ttype->is_set()) {
    throw "compiler error: serialize of set not supported in C";
  } else if (ttype->is_list()) {
    string length = prefix + "_size";
    out <<
      indent() << "if ((ret = thrift_protocol_write_list_begin (protocol, " <<
                   type_to_enum (((t_list *) ttype)->get_elem_type()) <<
                   ", " << length << ", error)) < 0)" << endl <<
      indent() << "  return " << error_ret << ";" << endl <<
      indent() << "xfer += ret;" << endl <<
      indent() << "for (i = 0; i < " << length << "; i++)" << endl;

    scope_up(out);
    generate_serialize_list_element (out, (t_list *) ttype, prefix, "i", error_ret);
    scope_down(out);

    out <<
      indent() << "if ((ret = thrift_protocol_write_list_end (protocol, error)) < 0)" << endl <<
      indent() << "  return " << error_ret << ";" << endl;
  }

  scope_down(out);
}

void t_c_generator::generate_serialize_map_element(ofstream &out,
                                                        t_map *tmap,
                                                        string key,
                                                        string value,
                                                        int error_ret) {
  t_field kfield (tmap->get_key_type(), key);
  generate_serialize_field (out, &kfield, "", "", error_ret);

  t_field vfield (tmap->get_val_type(), value);
  generate_serialize_field (out, &vfield, "", "", error_ret);
}

void t_c_generator::generate_serialize_set_element(ofstream &out,
                                                        t_set *tset,
                                                        string element,
                                                        int error_ret) {
  t_field efield (tset->get_elem_type(), element);
  generate_serialize_field (out, &efield, "", "", error_ret);
}

void t_c_generator::generate_serialize_list_element(ofstream &out,
                                                         t_list *tlist,
                                                         string list,
                                                         string index,
                                                         int error_ret) {
  t_type *ttype = tlist->get_elem_type();
  string name = list + "[" + index + "]";
  t_field efield (ttype, name);
  generate_serialize_field (out, &efield, "", "", error_ret);
  out << indent() << "xfer += ret;" << endl;
}

/* deserializes a field of any type. */
void t_c_generator::generate_deserialize_field(ofstream &out,
                                               t_field *tfield,
                                               string prefix,
                                               string suffix,
                                               int error_ret) {
  t_type *type = get_true_type (tfield->get_type());

  if (type->is_void()) {
    throw "CANNOT GENERATE DESERIALIZE CODE FOR void TYPE: " +
      prefix + tfield->get_name();
  }

  string name = prefix + tfield->get_name() + suffix;

  if (type->is_struct() || type->is_xception()) {
    generate_deserialize_struct (out, (t_struct *) type, name, error_ret);
  } else if (type->is_container()) {
    generate_deserialize_container (out, type, name, error_ret);
  } else if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type *) type)->get_base();

    indent(out) << "if ((ret = thrift_protocol_read_";

    switch (tbase) {
      case t_base_type::TYPE_VOID:
        throw "compiler error: cannot serialize void field in a struct: " + name;
        break;
      case t_base_type::TYPE_STRING:
        if (((t_base_type *) type)->is_binary()) {
          out << "binary (protocol, &data, &len";
        } else {
          out << "string (protocol, &" << name;
        }
        break;
      case t_base_type::TYPE_XML:
        out << "xml (protocol, &" << name;
        break;
      case t_base_type::TYPE_BOOL:
        out << "bool (protocol, &" << name;
        break;
      case t_base_type::TYPE_BYTE:
        out << "byte (protocol, &" << name;
        break;
      case t_base_type::TYPE_I16:
        out << "i16 (protocol, &" << name;
        break;
      case t_base_type::TYPE_I32:
        out << "i32 (protocol, &" << name;
        break;
      case t_base_type::TYPE_I64:
        out << "i64 (protocol, &" << name;
        break;
      case t_base_type::TYPE_U16:
        out << "u16 (protocol, &" << name;
        break;
      case t_base_type::TYPE_U32:
        out << "u32 (protocol, &" << name;
        break;
      case t_base_type::TYPE_U64:
        out << "u64 (protocol, &" << name;
        break;
      case t_base_type::TYPE_DOUBLE:
        out << "double (protocol, &" << name;
        break;
      default:
        throw "compiler error: no C reader for base type "
          + t_base_type::t_base_name (tbase) + name;
    }
    out << ", error)) < 0)" << endl;
    out << indent() << "  return " << error_ret << ";" << endl <<
           indent() << "xfer += ret;" << endl;

  } else if (type->is_enum()) {
    string t = tmp ("ecast");
    out <<
      indent() << "int32_t " << t << ";" << endl <<
      indent() << "if ((ret = thrift_protocol_read_i32 (protocol, &" << t << ", error)) < 0)" << endl <<
      indent() << "  return " << error_ret << ";" << endl <<
      indent() << "xfer += ret;" << endl <<
      indent() << name << " = (" << type_name (type) << ")" << t << ";" << endl;
  } else {
    printf ("DO NOT KNOW HOW TO DESERIALIZE FIELD '%s' TYPE '%s'\n",
            tfield->get_name().c_str(), type_name (type).c_str());
  }

  // if the type is not required and this is a thrift struct (no prefix),
  // set the isset variable.  if the type is required, then set the
  // local variable indicating the value was set, so that we can do    // validation later.
  if (tfield->get_req() != t_field::T_REQUIRED && prefix != "") {
    indent(out) << prefix << "__isset_" << tfield->get_name() << suffix << " = 1;" << endl;
  } else if (tfield->get_req() == t_field::T_REQUIRED && prefix != "") {
    indent(out) << "isset_" << tfield->get_name() << " = 1;" << endl;
  }
}

void t_c_generator::generate_deserialize_struct(ofstream &out,
                                                t_struct *tstruct,
                                                string prefix,
                                                int error_ret) {
  out <<
    indent() << prefix << " = (" << tstruct->get_name() << "* ) os_zalloc (sizeof(*" << prefix << "));" << endl <<
    indent() << "if ((ret = " << tstruct->get_name() << "_read (" << prefix << ", protocol, error)) < 0)" << endl <<
    indent() << "  return " << error_ret << ";" << endl <<
    indent() << "xfer += ret;" << endl;
}

void t_c_generator::generate_deserialize_container (ofstream &out, t_type *ttype,
                                               string prefix, int error_ret) {
  scope_up(out);

  if (ttype->is_map()) {
    throw "compiler error: deserialize of map not supported in C";
  } else if (ttype->is_set()) {
    throw "compiler error: deserialize of set not supported in C";
  } else if (ttype->is_list()) {
    out <<
      indent() << "ThriftType element_type;" << endl <<
      indent() << "if ((ret = thrift_protocol_read_list_begin (protocol, &element_type, &" << prefix << "_size, error)) < 0)" << endl <<
      indent() << "  return " << error_ret << ";" << endl <<
      indent() << "xfer += ret;" << endl <<
      endl;

    out <<
      indent() << prefix << " = (" << type_name(ttype, false, false) <<
        ") os_zalloc (sizeof(*" << prefix << ") * " << prefix << "_size);" << endl;
    out <<
      indent() << "/* iterate through list elements */" << endl <<
      indent() << "for (i = 0; i < " << prefix << "_size; i++)" << endl;

    scope_up(out);
    generate_deserialize_list_element (out, (t_list *) ttype, prefix, "i",
                                       error_ret);
    scope_down(out);

    out <<
      indent() << "if ((ret = thrift_protocol_read_list_end (protocol, error)) < 0)" << endl << 
      indent() << "  return " << error_ret << ";" << endl <<
      indent() << "xfer += ret;" << endl <<
      endl;
  }

  scope_down(out);
}

void t_c_generator::generate_deserialize_map_element(ofstream &out,
                                                          t_map *tmap,
                                                          string prefix,
                                                          int error_ret) {
  // NOT IMPLEMENTED
}

void t_c_generator::generate_deserialize_set_element(ofstream &out,
                                                          t_set *tset,
                                                          string prefix,
                                                          int error_ret) {
  // NOT IMPLEMENTED
}

void t_c_generator::generate_deserialize_list_element(ofstream &out,
                                                           t_list *tlist,
                                                           string prefix,
                                                           string index,
                                                           int error_ret) {
  string elem = prefix + "[" + index + "]";
  t_field felem (tlist->get_elem_type(), elem);
  generate_deserialize_field (out, &felem, "", "", error_ret);
}

/***************************************
 * UTILITY FUNCTIONS                   *
 ***************************************/

/**
 * Upper case a string.  Wraps boost's string utility.
 */
string to_upper_case(string name) {
  string s (name);
  std::transform (s.begin(), s.end(), s.begin(), ::toupper);
  return s;
//  return boost::to_upper_copy (name);
}

/**
 * Lower case a string.  Wraps boost's string utility.
 */
string to_lower_case(string name) {
  string s (name);
  std::transform (s.begin(), s.end(), s.begin(), ::tolower);
  return s;
//  return boost::to_lower_copy (name);
}

/**
 * Makes a string friendly to C code standards by lowercasing and adding
 * underscores, with the exception of the first character.  For example:
 *
 * Input: "ZomgCamelCase"
 * Output: "zomg_camel_case"
 */
string initial_caps_to_underscores(string name) {
  string ret;
  const char *tmp = name.c_str();
  int pos = 0;

  /* the first character isn't underscored if uppercase, just lowercased */
  ret += tolower (tmp[pos]);
  pos++;
  for (unsigned int i = pos; i < name.length(); i++) {
    char lc = tolower (tmp[i]); 
    if (lc != tmp[i]) {
      ret += '_';
    }
    ret += lc;
  }

  return ret;
}

/* register this generator with the main program */
THRIFT_REGISTER_GENERATOR(c, "C", "")
