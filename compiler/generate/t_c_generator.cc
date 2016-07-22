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
  string argument_list(t_struct *tstruct);
  string declare_field(t_field *tfield, bool init=false, bool pointer=false, bool constant=false, bool reference=false);

  /* generation functions */
  void generate_const_initializer(string name, t_type *type, t_const_value *value);
  void generate_service_client(t_service *tservice);
  void generate_service_server(t_service *tservice);
  void generate_object(t_struct *tstruct);
  void generate_object(t_sandesh *tsandesh);
  void generate_object_internal(string name, const vector<t_field *> &members, bool is_sandesh);
  void generate_struct_writer(ofstream &out, string name, const vector<t_field *> &fields, bool is_sandesh, bool is_function=true);
  void generate_struct_writer_to_buffer(ofstream &out, string name, const vector<t_field *> &fields, bool is_sandesh, bool is_function=true);
  void generate_struct_reader(ofstream &out, string name, const vector<t_field *> &fields, bool is_sandesh, bool is_function=true);
  void generate_struct_reader_from_buffer(ofstream &out, string name, const vector<t_field *> &fields, bool is_sandesh, bool is_function=true);
  void generate_struct_deleter(ofstream &out, string name, const vector<t_field *> &fields, bool is_sandesh, bool is_function=true);

  void generate_buffer_bounds_chk(ofstream &out, string length, int error_ret);
  void generate_buffer_incr_offset(ofstream &out, string length);

  void generate_write_buffer_memcpy(ofstream &out, string source, string length, bool ref);
  void generate_write_buffer_memcpy_incr_offset(ofstream &out, string source, string length, bool ref);
  void generate_write_buffer_chk_memcpy_incr_offset(ofstream &out, string source, string length, int error_ret, bool ref);
  void generate_write_buffer_binary(ofstream &out, string buf, string buf_len, int error_ret);

  void generate_struct_begin_writer_to_buffer(ofstream &out, string name, bool is_sandesh, int error_ret);
  void generate_struct_end_writer_to_buffer(ofstream &out, bool is_sandesh);
  void generate_field_begin_writer_to_buffer(ofstream &out, string key, string field_type, int error_ret);
  void generate_field_end_writer_to_buffer(ofstream &out);
  void generate_field_stop_writer_to_buffer(ofstream &out, int error_ret);
  void generate_list_begin_writer_to_buffer(ofstream &out, string element_type, string length, int error_ret);
  void generate_list_end_writer_to_buffer(ofstream &out);

  void generate_serialize_bool_to_buffer(ofstream &out, string name, int error_ret);
  void generate_serialize_byte_to_buffer(ofstream &out, string name, int error_ret);
  void generate_serialize_i16_to_buffer(ofstream &out, string name, int error_ret);
  void generate_serialize_i32_to_buffer(ofstream &out, string name, int error_ret);
  void generate_serialize_i64_to_buffer(ofstream &out, string name, int error_ret);
  void generate_serialize_u16_to_buffer(ofstream &out, string name, int error_ret);
  void generate_serialize_u32_to_buffer(ofstream &out, string name, int error_ret);
  void generate_serialize_u64_to_buffer(ofstream &out, string name, int error_ret);
  void generate_serialize_string_to_buffer(ofstream &out, string name, int error_ret);
  void generate_serialize_xml_to_buffer(ofstream &out, string name, int error_ret);
  void generate_serialize_ipaddr_to_buffer(ofstream &out, string name, int error_ret);
  void generate_serialize_uuid_t_to_buffer(ofstream &out, string name, int error_ret);
  void generate_serialize_double_to_buffer(ofstream &out, string name, int error_ret);

  void generate_read_buffer_memcpy(ofstream &out, string dest, string length, bool ref);
  void generate_read_buffer_memcpy_incr_offset(ofstream &out, string dest, string length, bool ref);
  void generate_read_buffer_chk_memcpy_incr_offset(ofstream &out, string dest, string length, int error_ret, bool ref);

  void generate_struct_begin_reader_from_buffer(ofstream &out, string name, bool is_sandesh, int error_ret);
  void generate_struct_end_reader_from_buffer(ofstream &out, bool is_sandesh);
  void generate_field_begin_reader_from_buffer(ofstream &out, string field_type, string field_id, int error_ret);
  void generate_field_end_reader_from_buffer(ofstream &out);
  void generate_list_begin_reader_from_buffer(ofstream &out, string element_size, int error_ret);
  void generate_list_end_reader_from_buffer(ofstream &out);

  void generate_deserialize_bool_from_buffer(ofstream &out, string name, int error_ret);
  void generate_deserialize_byte_from_buffer(ofstream &out, string name, int error_ret);
  void generate_deserialize_i16_from_buffer(ofstream &out, string name, int error_ret);
  void generate_deserialize_i32_from_buffer(ofstream &out, string name, int error_ret);
  void generate_deserialize_i64_from_buffer(ofstream &out, string name, int error_ret);
  void generate_deserialize_u16_from_buffer(ofstream &out, string name, int error_ret);
  void generate_deserialize_u32_from_buffer(ofstream &out, string name, int error_ret);
  void generate_deserialize_u64_from_buffer(ofstream &out, string name, int error_ret);
  void generate_deserialize_string_from_buffer(ofstream &out, string name, int error_ret);
  void generate_deserialize_xml_from_buffer(ofstream &out, string name, int error_ret);
  void generate_deserialize_ipaddr_from_buffer(ofstream &out, string name, int error_ret);
  void generate_deserialize_uuid_t_from_buffer(ofstream &out, string name, int error_ret);
  void generate_deserialize_double_from_buffer(ofstream &out, string name, int error_ret);

  void generate_serialize_field(ofstream &out, t_field *tfield, string prefix, string suffix, int error_ret);
  void generate_serialize_field_to_buffer(ofstream &out, t_field *tfield, string prefix, string suffix, int error_ret);
  void generate_serialize_struct(ofstream &out, t_struct *tstruct, string prefix, int error_ret);
  void generate_serialize_struct_to_buffer(ofstream &out, t_struct *tstruct, string prefix, int error_ret);
  void generate_serialize_container(ofstream &out, t_type *ttype, string prefix, int error_ret);
  void generate_serialize_container_to_buffer(ofstream &out, t_type *ttype, string prefix, int error_ret);
  void generate_serialize_list_element(ofstream &out, t_list *tlist, string list, string index, int error_ret);
  void generate_serialize_list_element_to_buffer(ofstream &out, t_list *tlist, string list, string index, int error_ret);

  void generate_deserialize_field(ofstream &out, t_field *tfield, string prefix, string suffix, int error_ret);
  void generate_deserialize_field_from_buffer(ofstream &out, t_field *tfield, string prefix, string suffix, int error_ret);
  void generate_deserialize_struct(ofstream &out, t_struct *tstruct, string prefix, int error_ret);
  void generate_deserialize_struct_from_buffer(ofstream &out, t_struct *tstruct, string prefix, int error_ret);
  void generate_deserialize_container(ofstream &out, t_type *ttype, string prefix, int error_ret);
  void generate_deserialize_container_from_buffer(ofstream &out, t_type *ttype, string prefix, int error_ret);
  void generate_deserialize_list_element(ofstream &out, t_list *tlist, string prefix, string index, int error_ret);
  void generate_deserialize_list_element_from_buffer(ofstream &out, t_list *tlist, string prefix, string index, int error_ret);
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
    out <<
      indent() << ".name = \"" << sandesh_name << "\"," << endl <<
      indent() << ".size = sizeof(" << sandesh_name << ")," << endl <<
      indent() << ".read = " << sandesh_name_u << "_read," << endl <<
      indent() << ".read_binary_from_buffer = " << sandesh_name_u << "_read_binary_from_buffer," << endl <<
      indent() << ".write = " << sandesh_name_u << "_write," << endl <<
      indent() << ".write_binary_to_buffer = " << sandesh_name_u << "_write_binary_to_buffer," << endl <<
      indent() << ".process = " << sandesh_name_u << "_process," << endl <<
      indent() << ".free = " << sandesh_name_u << "_free," << endl;
    indent_down();
    out << indent() << "}," << endl;
  }
  // Add end of array marker
  out << indent() << "{" << endl;
  indent_up();
  out <<
    indent() << ".name = NULL," << endl <<
    indent() << ".size = 0," << endl <<
    indent() << ".read = NULL," << endl <<
    indent() << ".read_binary_from_buffer = NULL," << endl <<
    indent() << ".write = NULL," << endl <<
    indent() << ".write_binary_to_buffer = NULL," << endl <<
    indent() << ".process = NULL," << endl <<
    indent() << ".free = NULL," << endl;
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
    case t_base_type::TYPE_IPV4:
      return "uint32_t";
    case t_base_type::TYPE_IPADDR:
      return "ipaddr_t";
    case t_base_type::TYPE_UUID:
      return "uuid_t";
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
      case t_base_type::TYPE_IPV4:
        return "T_IPV4";
      case t_base_type::TYPE_IPADDR:
        return "T_IPADDR";
      case t_base_type::TYPE_UUID:
        return "T_UUID";
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
      case t_base_type::TYPE_IPV4:
        render << value->get_integer();
        break;
      case t_base_type::TYPE_UUID:
        render << value->get_uuid(); // Need to fix later, c structure is not getting initialize.
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
        case t_base_type::TYPE_IPV4:
          result += " = 0";
          break;
        case t_base_type::TYPE_DOUBLE:
          result += " = (gdouble) 0";
          break;
        case t_base_type::TYPE_STRING:
        case t_base_type::TYPE_XML:
          result += " = NULL";
          break;
        case t_base_type::TYPE_UUID:
          result += "{0}";
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
    "_write_binary_to_buffer (" << dtname <<
    "* w" << dvname << ", uint8_t *buf, const size_t buf_len, int *error);" << endl;
  f_types_ <<
    "int32_t " <<
    this->nspace_lc << name_u <<
    "_read (" << dtname <<
    "* r" << dvname << ", ThriftProtocol *protocol, int *error);" << endl;
  f_types_ <<
    "int32_t " <<
    this->nspace_lc << name_u <<
    "_read_binary_from_buffer (" << dtname <<
    "* r" << dvname << ", uint8_t *buf, const size_t buf_len, int *error);" << endl;
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
  generate_struct_reader_from_buffer (f_types_impl_, name, members, is_sandesh);
  generate_struct_writer (f_types_impl_, name, members, is_sandesh);
  generate_struct_writer_to_buffer (f_types_impl_, name, members, is_sandesh);
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

void t_c_generator::generate_buffer_bounds_chk(ofstream &out,
                                               string length,
                                               int error_ret) {
  out <<
    indent() << "if (Xbuflen < Xoffset + " << length << ") {" << endl <<
    indent() << "  return " << error_ret << ";" << endl <<
    indent() << "}" << endl;
}

void t_c_generator::generate_buffer_incr_offset(ofstream &out,
                                                string length) {
  out <<
    indent() << "Xoffset += " << length << ";" << endl;
}

void t_c_generator::generate_write_buffer_memcpy(ofstream &out,
                                                 string source,
                                                 string length,
                                                 bool ref) {
  if (ref) {
    out <<
      indent() << "memcpy (Xbuf + Xoffset, &" << source << ", " <<
        length << ");" << endl;
  } else {
    out <<
      indent() << "memcpy (Xbuf + Xoffset, " << source << ", " <<
        length << ");" << endl;
  }
}

void t_c_generator::generate_read_buffer_memcpy(ofstream &out,
                                                string dest,
                                                string length,
                                                bool ref) {
  if (ref) {
    out <<
      indent() << "memcpy (&" << dest << ", Xbuf + Xoffset, " <<
        length << ");" << endl;
  } else {
    out <<
      indent() << "memcpy (" << dest << ", Xbuf + Xoffset, " <<
        length << ");" << endl;
  }
}

void t_c_generator::generate_write_buffer_memcpy_incr_offset(ofstream &out,
                                                             string source,
                                                             string length,
                                                             bool ref) {
  generate_write_buffer_memcpy (out, source, length, ref);
  generate_buffer_incr_offset (out, length);
}

void t_c_generator::generate_read_buffer_memcpy_incr_offset(ofstream &out,
                                                            string dest,
                                                            string length,
                                                            bool ref) {
  generate_read_buffer_memcpy (out, dest, length, ref);
  generate_buffer_incr_offset (out, length);
}

void t_c_generator::generate_write_buffer_chk_memcpy_incr_offset(ofstream &out,
                                                                 string source,
                                                                 string length,
                                                                 int error_ret,
                                                                 bool ref) {
  generate_buffer_bounds_chk (out, length, error_ret);
  generate_write_buffer_memcpy_incr_offset (out, source, length, ref);
}

void t_c_generator::generate_read_buffer_chk_memcpy_incr_offset(ofstream &out,
                                                                string dest,
                                                                string length,
                                                                int error_ret,
                                                                bool ref) {
  generate_buffer_bounds_chk (out, length, error_ret);
  generate_read_buffer_memcpy_incr_offset (out, dest, length, ref);
}

void t_c_generator::generate_write_buffer_binary(ofstream &out, string buf,
                                             string buf_len, int error_ret) {
  string nbuf_len(tmp("Xnbuflen"));
  generate_buffer_bounds_chk (out, "4", error_ret);
  scope_up(out);
  out <<
    indent() << "int32_t " << nbuf_len << " = htonl (" <<
      buf_len << ");" << endl;
  generate_write_buffer_memcpy_incr_offset (out, nbuf_len, "4", true);
  scope_down(out);
  generate_write_buffer_chk_memcpy_incr_offset (out, buf, buf_len, error_ret,
    false);
}

void t_c_generator::generate_struct_begin_writer_to_buffer(ofstream &out,
                                                    string name,
                                                    bool is_sandesh,
                                                    int error_ret) {
  if (is_sandesh) {
    ostringstream os;
    os << name.length();
    string name_len(os.str());
    out <<
      indent() << "/* thrift_protocol_write_sandesh_begin */" << endl;
    generate_write_buffer_binary (out, "\"" + name + "\"", name_len,
                                     error_ret);
  } else {
    out << indent() << "/* thrift_protocol_write_struct_begin */" << endl;
  }
}

void t_c_generator::generate_struct_begin_reader_from_buffer(ofstream &out,
                                                    string name,
                                                    bool is_sandesh,
                                                    int error_ret) {
  if (is_sandesh) {
    out <<
      indent() << "/* thrift_protocol_read_sandesh_begin */" << endl;
    generate_deserialize_string_from_buffer (out, name, error_ret);
    out <<
      indent() << "if (" << name << ") { os_free (" << name << "); " <<
        name << " = NULL; }" << endl << endl;
  } else {
    out << indent() << "/* thrift_protocol_read_struct_begin */" << endl;
  }
}

void t_c_generator::generate_struct_end_writer_to_buffer(ofstream &out,
                                                         bool is_sandesh) {
  if (is_sandesh) {
    out << indent() << "/* thrift_protocol_write_sandesh_send */" << endl;
  } else {
    out << indent() << "/* thrift_protocol_write_struct_send */" << endl;
  }
}

void t_c_generator::generate_struct_end_reader_from_buffer(ofstream &out,
                                                           bool is_sandesh) {
  if (is_sandesh) {
    out << indent() << "/* thrift_protocol_read_sandesh_send */" << endl;
  } else {
    out << indent() << "/* thrift_protocol_read_struct_send */" << endl;
  }
}

void t_c_generator::generate_field_stop_writer_to_buffer(ofstream &out,
                                                         int error_ret) {
  string field_stop(tmp("Xfield_stop"));
  out << endl <<
    indent() << "/* thrift_protocol_field_stop */" << endl;
  generate_buffer_bounds_chk (out, "1", error_ret);
  scope_up(out);
  out <<
    indent() << "int8_t " << field_stop << " = (int8_t) T_STOP;" << endl;
  generate_write_buffer_memcpy_incr_offset (out, field_stop, "1", true);
  scope_down(out);
}

void t_c_generator::generate_struct_writer_to_buffer(ofstream &out,
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
      this->nspace_lc << name_u << "_write_binary_to_buffer (" <<
      dname << "* " << dwname <<
      ", uint8_t *Xbuf, const size_t Xbuflen, int *Xerror)" << endl;
  }
  indent(out) << "{" << endl;
  indent_up();

  out <<
    indent() << "int32_t Xret;" << endl <<
    indent() << "int32_t Xoffset = 0;" << endl <<
    indent() << "u_int32_t Xi;" << endl <<
    endl;

  if (is_sandesh) {
    out << indent() << name << "* " << wname << " = " <<
      "(" << name << "* ) " << dwname << ";" <<
      endl << endl;
  }

  // satisfy -Wall in case we don't use some variables
  out <<
    indent() << "/* satisfy -Wall in case these aren't used */" << endl <<
    indent() << "THRIFT_UNUSED_VAR (Xret);" << endl <<
    indent() << "THRIFT_UNUSED_VAR (Xi);" << endl <<
    indent() << "THRIFT_UNUSED_VAR (" << wname << ");" << endl;

  out << endl;

  generate_struct_begin_writer_to_buffer(out, name, is_sandesh, error_ret);

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    // Ignore auto generated fields
    if ((*f_iter)->get_auto_generated()) {
      continue;
    }
    if ((*f_iter)->get_req() == t_field::T_OPTIONAL) {
      indent(out) << "if (" << wname << "->__isset_" << (*f_iter)->get_name() << " == 1) {" << endl;
      indent_up();
    }
    ostringstream key_os;
    key_os << (*f_iter)->get_key();
    string key(key_os.str());
    string tenum(type_to_enum((*f_iter)->get_type()));
    generate_field_begin_writer_to_buffer (out, key, tenum, error_ret);
    generate_serialize_field_to_buffer (out, *f_iter, wname + "->", "", error_ret);
    generate_field_end_writer_to_buffer (out);
    if ((*f_iter)->get_req() == t_field::T_OPTIONAL) {
      indent(out) << "if (" << wname << "->__isset_" << (*f_iter)->get_name() << " == 1) {" << endl;
      indent_up();
    }
  }
  generate_field_stop_writer_to_buffer(out, error_ret);
  generate_struct_end_writer_to_buffer(out, is_sandesh);
  if (is_function) {
    indent(out) << "return Xoffset;" << endl;
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

/**
 * Generates code to read Thrift structures from a buffer.
 */
void t_c_generator::generate_struct_reader_from_buffer(ofstream &out,
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
          "_read_binary_from_buffer (" << dname <<
          " *" << drname << ", uint8_t *Xbuf, const size_t Xbuflen, int *Xerror)" << endl;
  }

  indent(out) << "{" << endl;
  indent_up();

  // declare stack temp variables
  out <<
    indent() << "int32_t Xret;" << endl <<
    indent() << "int32_t Xoffset = 0;" << endl <<
    indent() << "char *Xname;" << endl <<
    indent() << "ThriftType Xftype;" << endl <<
    indent() << "int16_t Xfid;" << endl <<
    indent() << "u_int32_t Xi;" << endl;

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
    indent() << "THRIFT_UNUSED_VAR (Xret);" << endl <<
    indent() << "THRIFT_UNUSED_VAR (Xname);" << endl <<
    indent() << "THRIFT_UNUSED_VAR (Xi);" << endl <<
    indent() << "THRIFT_UNUSED_VAR (" << rname << ");" << endl;

  out << endl;

  // read the beginning of the structure marker
  out <<
    indent() << "/* read the " << sname << " begin marker */" << endl;
  generate_struct_begin_reader_from_buffer (out, "Xname", is_sandesh, error_ret);

  // read the struct fields
  out <<
    indent() << "/* read the struct fields */" << endl <<
    indent() << "while (1)" << endl;
  scope_up(out);

  // read beginning field marker
  out <<
    indent() << "/* read the beginning of a field */" << endl;
  generate_field_begin_reader_from_buffer (out, "Xftype", "Xfid", error_ret);
  out << endl;

  // switch depending on the field type
  indent(out) <<
    "switch (Xfid)" << endl;

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
      "if (Xftype == " << type_to_enum ((*f_iter)->get_type()) << ")" << endl;
    indent(out) <<
      "{" << endl;


    indent_up();
    // generate deserialize field
    generate_deserialize_field_from_buffer (out, *f_iter, rname + "->", "", error_ret);
    indent_down();

    out <<
      indent() << "} else {" << endl <<
      indent() << "  if ((Xret = thrift_binary_protocol_skip_from_buffer (Xbuf + Xoffset, Xbuflen - Xoffset, Xftype, Xerror)) < 0)" << endl <<
      indent() << "    return " << error_ret << ";" << endl <<
      indent() << "  Xoffset += Xret;" << endl <<
      indent() << "}" << endl <<
      indent() << "break;" << endl;
    indent_down();
  }

  // create the default case
  out <<
    indent() << "default:" << endl <<
    indent() << "  if ((Xret = thrift_binary_protocol_skip_from_buffer (Xbuf + Xoffset, Xbuflen - Xoffset, Xftype, Xerror)) < 0)" << endl <<
    indent() << "    return " << error_ret << ";" << endl <<
    indent() << "  Xoffset += Xret;" << endl <<
    indent() << "  break;" << endl;

  // end switch
  scope_down(out);

  // read field end marker
  generate_field_end_reader_from_buffer (out);

  // end while loop
  scope_down(out);
  out << endl;

  // read the end of the structure
  generate_struct_end_reader_from_buffer (out, is_sandesh);

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
      "return Xoffset;" << endl;
  }

  // end the function/structure
  indent_down();
  indent(out) <<
    "}" << endl <<
    endl;
}

void t_c_generator::generate_field_begin_writer_to_buffer(ofstream &out,
                                              string key,
                                              string field_type,
                                              int error_ret) {
  out <<
    indent() << "/* thrift_protocol_write_field_begin */" << endl;
  generate_buffer_bounds_chk (out, "1", error_ret);
  string field_type8(tmp("Xfield_type8"));
  scope_up(out);
  out <<
    indent() << "int8_t " << field_type8 << " = (int8_t) " <<
    field_type << ";" << endl;
  generate_write_buffer_memcpy_incr_offset (out, field_type8,
                                            "1", true);
  scope_down(out);
  generate_buffer_bounds_chk (out, "2", error_ret);
  string nkey(tmp("Xnfield_id"));
  scope_up(out);
  out <<
    indent() << "int16_t " << nkey << " = htons (" << key << ");" << endl;
  generate_write_buffer_memcpy_incr_offset (out, nkey, "2", true);
  scope_down(out);
  out << endl;
}

void t_c_generator::generate_field_begin_reader_from_buffer(ofstream &out,
                                              string field_type,
                                              string field_id,
                                              int error_ret) {
  out <<
    indent() << "/* thrift_protocol_read_field_begin */" << endl;
  generate_deserialize_byte_from_buffer (out, field_type, error_ret);
  // check for field STOP marker
  out <<
    indent() << "/* break if we get a STOP field */" << endl <<
    indent() << "if (" << field_type << " == T_STOP)" << endl <<
    indent() << "{" << endl <<
    indent() << "  break;" << endl <<
    indent() << "}" << endl;
  out <<
    indent() << " else" << endl;
  scope_up(out);
  generate_deserialize_i16_from_buffer (out, field_id, error_ret);
  scope_down(out);
}

void t_c_generator::generate_field_end_writer_to_buffer(ofstream &out) {
  out <<
    indent() << "/* thrift_protocol_write_field_end */" << endl;
}

void t_c_generator::generate_field_end_reader_from_buffer(ofstream &out) {
  out <<
    indent() << "/* thrift_protocol_read_field_end */" << endl;
}

void t_c_generator::generate_serialize_bool_to_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  string nbool(tmp("Xbool"));
  out <<
    indent() << "/* thrift_protocol_write_bool */" << endl;
  scope_up(out);
  out <<
    indent() << "uint8_t " << nbool << " = " << name << " ? 1 : 0;" << endl;
  generate_serialize_byte_to_buffer (out, nbool, error_ret);
  scope_down(out);
}

void t_c_generator::generate_deserialize_bool_from_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  out <<
    indent() << "/* thrift_protocol_read_bool */" << endl;
  scope_up(out);
  out << indent() << "void * b[1];" << endl;
  generate_read_buffer_chk_memcpy_incr_offset (out, "b", "1", error_ret,
                                               false);
  out <<
    indent() << name << " = *(int8_t *) b != 0;" << endl;
  scope_down(out);
}

void t_c_generator::generate_serialize_byte_to_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  out <<
    indent() << "/* thrift_protocol_write_byte */" << endl;
  generate_write_buffer_chk_memcpy_incr_offset (out, name, "1", error_ret,
                                                true);
}

void t_c_generator::generate_deserialize_byte_from_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  out <<
    indent() << "/* thrift_protocol_read_byte */" << endl;
  scope_up(out);
  out << indent() << "void * b[1];" << endl;
  generate_read_buffer_chk_memcpy_incr_offset (out, "b", "1", error_ret,
                                                false);
  out <<
    indent() << name << " = *(int8_t *) b;" << endl;
  scope_down(out);;
}

void t_c_generator::generate_serialize_i16_to_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  string ni16(tmp("Xni16"));
  out << indent() << "/* thrift_protocol_write_i16 */" << endl;
  scope_up(out);
  out <<
    indent() << "int16_t " << ni16 << " = htons (" << name << ");" << endl;
  generate_write_buffer_chk_memcpy_incr_offset (out, ni16, "2", error_ret,
                                                true);
  scope_down(out);
}

void t_c_generator::generate_deserialize_i16_from_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  out << indent() << "/* thrift_protocol_read_i16 */" << endl;
  scope_up(out);
  out <<
    indent() << "union {"<< endl <<
    indent() << "  void * b[2];" << endl <<
    indent() << "  int16_t all;" << endl <<
    indent() << "} bytes;" << endl;
  generate_read_buffer_chk_memcpy_incr_offset (out, "bytes.b", "2", error_ret,
                                               false);
  out <<
    indent() << name << " = ntohs (bytes.all);" << endl;
  scope_down(out);
}

void t_c_generator::generate_serialize_u16_to_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  string nu16(tmp("Xnu16"));
  out << indent() << "/* thrift_protocol_write_u16 */" << endl;
  scope_up(out);
  out <<
    indent() << "uint16_t " << nu16 << " = htons (" << name << ");" << endl;
  generate_write_buffer_chk_memcpy_incr_offset (out, nu16, "2", error_ret,
                                                true);
  scope_down(out);
}

void t_c_generator::generate_deserialize_u16_from_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  out << indent() << "/* thrift_protocol_read_u16 */" << endl;
  scope_up(out);
  out <<
    indent() << "union {"<< endl <<
    indent() << "  void * b[2];" << endl <<
    indent() << "  uint16_t all;" << endl <<
    indent() << "} bytes;" << endl;
  generate_read_buffer_chk_memcpy_incr_offset (out, "bytes.b", "2", error_ret,
                                               false);
  out <<
    indent() << name << " = ntohs (bytes.all);" << endl;
  scope_down(out);
}

void t_c_generator::generate_serialize_i32_to_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  string ni32(tmp("Xni32"));
  out << indent() << "/* thrift_protocol_write_i32 */" << endl;
  scope_up(out);
  out <<
    indent() << "int32_t " << ni32 << " = htonl (" << name << ");" << endl;
  generate_write_buffer_chk_memcpy_incr_offset (out, ni32, "4", error_ret,
                                                true);
  scope_down(out);
}

void t_c_generator::generate_deserialize_i32_from_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  out << indent() << "/* thrift_protocol_read_i32 */" << endl;
  scope_up(out);
  out <<
    indent() << "union {"<< endl <<
    indent() << "  void * b[4];" << endl <<
    indent() << "  int32_t all;" << endl <<
    indent() << "} bytes;" << endl;
  generate_read_buffer_chk_memcpy_incr_offset (out, "bytes.b", "4", error_ret,
                                               false);
  out <<
    indent() << name << " = ntohl (bytes.all);" << endl;
  scope_down(out);
}

void t_c_generator::generate_serialize_u32_to_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  string nu32(tmp("Xnu32"));
  out << indent() << "/* thrift_protocol_write_u32 */" << endl;
  scope_up(out);
  out <<
    indent() << "uint32_t " << nu32 << " = htonl (" << name << ");" << endl;
  generate_write_buffer_chk_memcpy_incr_offset (out, nu32, "4", error_ret,
                                                true);
  scope_down(out);
}

void t_c_generator::generate_deserialize_u32_from_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  out << indent() << "/* thrift_protocol_read_u32 */" << endl;
  scope_up(out);
  out <<
    indent() << "union {"<< endl <<
    indent() << "  void * b[4];" << endl <<
    indent() << "  uint32_t all;" << endl <<
    indent() << "} bytes;" << endl;
  generate_read_buffer_chk_memcpy_incr_offset (out, "bytes.b", "4", error_ret,
                                               false);
  out <<
    indent() << name << " = ntohl (bytes.all);" << endl;
  scope_down(out);
}

void t_c_generator::generate_serialize_i64_to_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  string ni64(tmp("Xni64"));
  out << indent() << "/* thrift_protocol_write_i64 */" << endl;
  scope_up(out);
  out <<
    indent() << "int64_t " << ni64 << ";" << endl <<
    indent() << "os_put_value64((uint8_t *)&" << ni64 << ", " << name <<
      ");" << endl;
  generate_write_buffer_chk_memcpy_incr_offset (out, ni64, "8", error_ret,
                                                true);
  scope_down(out);
}

void t_c_generator::generate_deserialize_i64_from_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  out << indent() << "/* thrift_protocol_read_i64 */" << endl;
  scope_up(out);
  out <<
    indent() << "void * b[8];" << endl;
  generate_read_buffer_chk_memcpy_incr_offset (out, "b", "8", error_ret,
                                               false);
  out <<
    indent() << name << " = os_get_value64((uint8_t *)b);" << endl;
  scope_down(out);
}

void t_c_generator::generate_serialize_u64_to_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  string nu64(tmp("Xnu64"));
  out << indent() << "/* thrift_protocol_write_u64 */" << endl;
  scope_up(out);
  out <<
    indent() << "uint64_t " << nu64 << ";" << endl <<
    indent() << "os_put_value64((uint8_t *)&" << nu64 << ", " << name <<
      ");" << endl;
  generate_write_buffer_chk_memcpy_incr_offset (out, nu64, "8", error_ret,
                                                true);
  scope_down(out);
}

void t_c_generator::generate_deserialize_u64_from_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  out << indent() << "/* thrift_protocol_read_u64 */" << endl;
  scope_up(out);
  out <<
    indent() << "void * b[8];" << endl;
  generate_read_buffer_chk_memcpy_incr_offset (out, "b", "8", error_ret,
                                               false);
  out <<
    indent() << name << " = os_get_value64((uint8_t *)b);" << endl;
  scope_down(out);
}

void t_c_generator::generate_serialize_xml_to_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  generate_serialize_string_to_buffer(out, name, error_ret);
}

void t_c_generator::generate_deserialize_xml_from_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  generate_deserialize_string_from_buffer(out, name, error_ret);
}

void t_c_generator::generate_serialize_string_to_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  string s_strlen(tmp("Xsstrlen"));
  out << indent() << "/* thrift_protocol_write_string */" << endl;
  scope_up(out);
  out <<
    indent() << "int32_t " << s_strlen << " = " << name <<
      " != NULL ? strlen (" << name << ") : 0;" << endl;
  generate_write_buffer_binary (out, name, s_strlen, error_ret);
  scope_down(out);
}

void t_c_generator::generate_deserialize_string_from_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  string s_strlen(tmp("Xread_len"));
  out << indent() << "/* thrift_protocol_read_string */" << endl;
  scope_up(out);
  out <<
    indent() << "int32_t " << s_strlen << " = 0;" << endl;
  generate_deserialize_i32_from_buffer (out, s_strlen, error_ret);
  out <<
    indent() << "if (" << s_strlen << " > 0)" << endl;
  scope_up(out);
  out <<
    indent() << "if (Xbuflen < Xoffset + " << s_strlen << ") {" << endl <<
    indent() << "  return " << error_ret << ";" << endl <<
    indent() << "}" << endl <<
    indent() << name << " = os_malloc (" << s_strlen << " + 1);" << endl;
  generate_read_buffer_memcpy_incr_offset (out, name, s_strlen, false);
  out <<
    indent() << name << "[" << s_strlen << "] = 0;" << endl;
  scope_down(out);
  out <<
    indent() << " else" << endl;
  scope_up(out);
  out <<
    indent() << name << " = NULL;" << endl;
  scope_down(out);
  scope_down(out);
}

void t_c_generator::generate_serialize_ipaddr_to_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  out <<
    indent() << "/* thrift_protocol_write_ipaddr */" << endl;
  string iptype(name + ".iptype");
  out <<
    indent() << "if (" << iptype << " == AF_INET)" << endl;
  scope_up(out);
  generate_write_buffer_chk_memcpy_incr_offset (out, iptype, "1", error_ret,
                                                true);
  string ipv4(name + ".ipv4");
  generate_write_buffer_chk_memcpy_incr_offset (out, ipv4, "4", error_ret,
                                                true);
  scope_down(out);
  out <<
    indent() << " else if (" << iptype << " == AF_INET6)" << endl;
  scope_up(out);
  generate_write_buffer_chk_memcpy_incr_offset (out, iptype, "1", error_ret,
                                                true);
  string ipv6(name + ".ipv6");
  generate_write_buffer_chk_memcpy_incr_offset (out, ipv6, "16", error_ret,
                                                true);
  scope_down(out);
  out <<
    indent() << " else" << endl;
  scope_up(out);
  out <<
    indent() << "return -1;" << endl;
  scope_down(out);
}

void t_c_generator::generate_deserialize_ipaddr_from_buffer(ofstream &out,
                                                string name,
                                                int error_ret) {
  out <<
    indent() << "/* thrift_protocol_read_ipaddr */" << endl;
  string iptype(name + ".iptype");
  generate_deserialize_byte_from_buffer (out, iptype, error_ret);
  out <<
    indent() << "if (" << iptype << " == AF_INET)" << endl;
  scope_up(out);
  string ipv4(name + ".ipv4");
  generate_read_buffer_chk_memcpy_incr_offset (out, ipv4, "4", error_ret,
                                               true);
  scope_down(out);
  out <<
    indent() << "else if (" << iptype << " == AF_INET6)" << endl;
  scope_up(out);
  string ipv6(name + ".ipv6");
  generate_read_buffer_chk_memcpy_incr_offset (out, ipv6, "16", error_ret,
                                               true);
  scope_down(out);
  out <<
    indent() << " else" << endl;
  scope_up(out);
  out <<
    indent() << "return -1;" << endl;
  scope_down(out);
}

void t_c_generator::generate_serialize_uuid_t_to_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  out <<
    indent() << "/* thrift_protocol_write_uuid_t */" << endl;
  generate_write_buffer_chk_memcpy_incr_offset (out, name, "16", error_ret,
                                                true);
}

void t_c_generator::generate_deserialize_uuid_t_from_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  out <<
    indent() << "/* thrift_protocol_read_uuid_t */" << endl;
  generate_read_buffer_chk_memcpy_incr_offset (out, name, "16", error_ret,
                                                true);
}

void t_c_generator::generate_serialize_double_to_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  out <<
    indent() << "/* thrift_protocol_write_double */" << endl <<
    indent() << "/* NOT SUPPORTED */" << endl <<
    indent() << "return " << error_ret << ";" << endl;
}

void t_c_generator::generate_deserialize_double_from_buffer(ofstream &out,
                                              string name,
                                              int error_ret) {
  out <<
    indent() << "/* thrift_protocol_read_double */" << endl <<
    indent() << "/* NOT SUPPORTED */" << endl <<
    indent() << "return " << error_ret << ";" << endl;
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
        case t_base_type::TYPE_IPV4:
          out << "ipv4 (protocol, " << name;
          break;
        case t_base_type::TYPE_IPADDR:
          out << "ipaddr (protocol, &" << name;
          break;
        case t_base_type::TYPE_STRING:
          if (((t_base_type *) type)->is_binary()) {
            throw "CANNOT GENERATE SERIALIZE CODE FOR binary TYPE: " + name;
          } else {
            out << "string (protocol, " << name;
          }
          break;
        case t_base_type::TYPE_XML:
          out << "xml (protocol, " << name;
          break;
        case t_base_type::TYPE_UUID:
          out << "uuid_t (protocol, " << name;
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

void t_c_generator::generate_serialize_field_to_buffer(ofstream &out,
                                                  t_field *tfield,
                                                  string prefix,
                                                  string suffix,
                                                  int error_ret) {
  t_type *type = get_true_type (tfield->get_type());
  string name = prefix + tfield->get_name() + suffix;
  ostringstream key_os;
  key_os << tfield->get_key();
  string key(key_os.str());
  string tenum(type_to_enum(tfield->get_type()));

  if (type->is_void()) {
    throw "CANNOT GENERATE SERIALIZE CODE FOR void TYPE: " + name;
  }

  if (type->is_struct() || type->is_xception()) {
    generate_serialize_struct_to_buffer (out, (t_struct *) type, name, error_ret);
  } else if (type->is_container()) {
    generate_serialize_container_to_buffer (out, type, name, error_ret);
  } else if (type->is_base_type() || type->is_enum()) {
    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type *) type)->get_base();
      switch (tbase) {
        case t_base_type::TYPE_VOID:
          throw "compiler error: cannot serialize void field in a struct: "
                + name;
          break;
        case t_base_type::TYPE_BOOL:
          generate_serialize_bool_to_buffer (out, name, error_ret);
          break;
        case t_base_type::TYPE_BYTE:
          generate_serialize_byte_to_buffer (out, name, error_ret);
          break;
        case t_base_type::TYPE_I16:
          generate_serialize_i16_to_buffer (out, name, error_ret);
          break;
        case t_base_type::TYPE_I32:
          generate_serialize_i32_to_buffer (out, name, error_ret);
          break;
        case t_base_type::TYPE_I64:
          generate_serialize_i64_to_buffer (out, name, error_ret);
          break;
        case t_base_type::TYPE_U16:
          generate_serialize_u16_to_buffer (out, name, error_ret);
          break;
        case t_base_type::TYPE_U32:
          generate_serialize_u32_to_buffer (out, name, error_ret);
          break;
        case t_base_type::TYPE_U64:
          generate_serialize_u64_to_buffer (out, name, error_ret);
          break;
        case t_base_type::TYPE_DOUBLE:
          generate_serialize_double_to_buffer (out, name, error_ret);
          break;
        case t_base_type::TYPE_IPV4:
          generate_serialize_i32_to_buffer (out, name, error_ret);
          break;
        case t_base_type::TYPE_IPADDR:
          generate_serialize_ipaddr_to_buffer (out, name, error_ret);
          break;
        case t_base_type::TYPE_STRING:
          if (((t_base_type *) type)->is_binary()) {
            throw "CANNOT GENERATE SERIALIZE CODE FOR binary TYPE: " + name;
          } else {
            generate_serialize_string_to_buffer (out, name, error_ret);
          }
          break;
        case t_base_type::TYPE_XML:
          generate_serialize_xml_to_buffer (out, name, error_ret);
          break;
        case t_base_type::TYPE_UUID:
          generate_serialize_uuid_t_to_buffer (out, name, error_ret);
          break;
        default:
          throw "compiler error: no C writer for base type "
                + t_base_type::t_base_name (tbase) + name;
      }
    } else if (type->is_enum()) {
      generate_serialize_i32_to_buffer (out, name, error_ret);
    }
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

void t_c_generator::generate_serialize_struct_to_buffer(ofstream &out,
                                                   t_struct *tstruct,
                                                   string prefix,
                                                   int error_ret) {
  out <<
    indent() << "if ((Xret = " << tstruct->get_name() <<
      "_write_binary_to_buffer (" << prefix <<
      ", Xbuf + Xoffset, Xbuflen - Xoffset, Xerror)) < 0)" << endl;
  out <<
    indent() << "  return " << error_ret << ";" << endl <<
    indent() << "Xoffset += Xret;" << endl;
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

void t_c_generator::generate_serialize_container_to_buffer(ofstream &out,
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
    generate_list_begin_writer_to_buffer (out,
      type_to_enum (((t_list *) ttype)->get_elem_type()), length,
      error_ret);
    out <<
      indent() << "for (Xi = 0; Xi < " << length << "; Xi++)" << endl;
    scope_up(out);
    generate_serialize_list_element_to_buffer (out, (t_list *) ttype, prefix, "Xi", error_ret);
    scope_down(out);
    generate_list_end_writer_to_buffer (out);
    out << endl;
  }
  scope_down(out);
}

void t_c_generator::generate_list_begin_writer_to_buffer(ofstream &out,
                                                         string element_type,
                                                         string length,
                                                         int error_ret) {
  string element_type8(tmp("Xelement_type8"));
  out <<
    indent() << "/* thrift_protocol_write_list_begin */" << endl;
  generate_buffer_bounds_chk (out, "1", error_ret);
  scope_up(out);
  out <<
    indent() << "int8_t " <<  element_type8 << " = (int8_t) " <<
    element_type << ";" << endl;
  generate_write_buffer_memcpy_incr_offset (out, element_type8, "1",
                                            true);
  scope_down(out);
  string nelement_size(tmp("Xnelement_size"));
  generate_buffer_bounds_chk (out, "4", error_ret);
  scope_up(out);
  out <<
    indent() << "uint32_t " << nelement_size << " = htonl (" <<
    length << ");" << endl;
  generate_write_buffer_memcpy_incr_offset (out, nelement_size, "4",
                                            true);
  scope_down(out);
  out << endl;
}

void t_c_generator::generate_list_end_writer_to_buffer(ofstream &out) {
  out <<
    indent() << "/* thrift_protocol_write_list_end */" << endl;
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

void t_c_generator::generate_serialize_list_element_to_buffer(ofstream &out,
                                                         t_list *tlist,
                                                         string list,
                                                         string index,
                                                         int error_ret) {
  t_type *ttype = tlist->get_elem_type();
  string name = list + "[" + index + "]";
  t_field efield (ttype, name);
  generate_serialize_field_to_buffer (out, &efield, "", "", error_ret);
  out << endl;
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
      case t_base_type::TYPE_IPV4:
        out << "ipv4 (protocol, &" << name;
        break;
      case t_base_type::TYPE_IPADDR:
        out << "ipaddr (protocol, &" << name;
        break;
      case t_base_type::TYPE_UUID:
        out << "uuid_t (protocol, &" << name;
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

/* deserializes a field of any type. */
void t_c_generator::generate_deserialize_field_from_buffer(ofstream &out,
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
    generate_deserialize_struct_from_buffer (out, (t_struct *) type, name, error_ret);
  } else if (type->is_container()) {
    generate_deserialize_container_from_buffer (out, type, name, error_ret);
  } else if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type *) type)->get_base();

    switch (tbase) {
      case t_base_type::TYPE_VOID:
        throw "compiler error: cannot serialize void field in a struct: " + name;
        break;
      case t_base_type::TYPE_STRING:
        if (((t_base_type *) type)->is_binary()) {
          throw "CANNOT GENERATE DESERIALIZE CODE FOR binary TYPE: " + name;
        } else {
          generate_deserialize_string_from_buffer (out, name, error_ret);
        }
        break;
      case t_base_type::TYPE_XML:
        generate_deserialize_xml_from_buffer (out, name, error_ret);
        break;
      case t_base_type::TYPE_BOOL:
        generate_deserialize_bool_from_buffer (out, name, error_ret);
        break;
      case t_base_type::TYPE_BYTE:
        generate_deserialize_byte_from_buffer (out, name, error_ret);
        break;
      case t_base_type::TYPE_I16:
        generate_deserialize_i16_from_buffer (out, name, error_ret);
        break;
      case t_base_type::TYPE_I32:
        generate_deserialize_i32_from_buffer (out, name, error_ret);
        break;
      case t_base_type::TYPE_I64:
        generate_deserialize_i64_from_buffer (out, name, error_ret);
        break;
      case t_base_type::TYPE_U16:
        generate_deserialize_u16_from_buffer (out, name, error_ret);
        break;
      case t_base_type::TYPE_U32:
        generate_deserialize_u32_from_buffer (out, name, error_ret);
        break;
      case t_base_type::TYPE_U64:
        generate_deserialize_u64_from_buffer (out, name, error_ret);
        break;
      case t_base_type::TYPE_DOUBLE:
        generate_deserialize_double_from_buffer (out, name, error_ret);
        break;
      case t_base_type::TYPE_IPV4:
        generate_deserialize_i32_from_buffer (out, name, error_ret);
        break;
      case t_base_type::TYPE_IPADDR:
        generate_deserialize_ipaddr_from_buffer (out, name, error_ret);
        break;
      case t_base_type::TYPE_UUID:
        generate_deserialize_uuid_t_from_buffer (out, name, error_ret);
        break;
      default:
        throw "compiler error: no C reader for base type "
          + t_base_type::t_base_name (tbase) + name;
    }
  } else if (type->is_enum()) {
    generate_deserialize_i32_from_buffer (out, name, error_ret);
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

void t_c_generator::generate_deserialize_struct_from_buffer(ofstream &out,
                                                t_struct *tstruct,
                                                string prefix,
                                                int error_ret) {
  out <<
    indent() << prefix << " = (" << tstruct->get_name() << "* ) os_zalloc (sizeof(*" << prefix << "));" << endl <<
    indent() << "if ((Xret = " << tstruct->get_name() << "_read_binary_from_buffer (" << prefix <<
      ", Xbuf + Xoffset, Xbuflen - Xoffset, Xerror)) < 0)" << endl <<
    indent() << "  return " << error_ret << ";" << endl <<
    indent() << "Xoffset += Xret;" << endl;
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

void t_c_generator::generate_list_begin_reader_from_buffer(ofstream &out,
                                                         string element_size,
                                                         int error_ret) {
  string element_type8(tmp("Xelement_type8"));
  out <<
    indent() << "/* thrift_protocol_read_list_begin */" << endl;
  scope_up(out);
  out <<
    indent() << "int8_t " <<  element_type8 << ";" << endl;
  generate_deserialize_byte_from_buffer (out, element_type8, error_ret);
  scope_down(out);
  generate_deserialize_i32_from_buffer (out, element_size, error_ret);
}

void t_c_generator::generate_list_end_reader_from_buffer(ofstream &out) {
  out <<
    indent() << "/* thrift_protocol_read_list_end */" << endl;
}

void t_c_generator::generate_deserialize_container_from_buffer (ofstream &out, t_type *ttype,
                                               string prefix, int error_ret) {
  scope_up(out);

  if (ttype->is_map()) {
    throw "compiler error: deserialize of map not supported in C";
  } else if (ttype->is_set()) {
    throw "compiler error: deserialize of set not supported in C";
  } else if (ttype->is_list()) {
    string element_size(prefix + "_size");
    generate_list_begin_reader_from_buffer (out, element_size, error_ret);
    out <<
      indent() << prefix << " = (" << type_name(ttype, false, false) <<
        ") os_zalloc (sizeof(*" << prefix << ") * " << element_size << ");" << endl;
    out <<
      indent() << "/* iterate through list elements */" << endl <<
      indent() << "for (Xi = 0; Xi < " << element_size << "; Xi++)" << endl;

    scope_up(out);
    generate_deserialize_list_element_from_buffer (out, (t_list *) ttype, prefix, "Xi",
                                       error_ret);
    scope_down(out);

    generate_list_end_reader_from_buffer (out);
  }

  scope_down(out);
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

void t_c_generator::generate_deserialize_list_element_from_buffer(ofstream &out,
                                                           t_list *tlist,
                                                           string prefix,
                                                           string index,
                                                           int error_ret) {
  string elem = prefix + "[" + index + "]";
  t_field felem (tlist->get_elem_type(), elem);
  generate_deserialize_field_from_buffer (out, &felem, "", "", error_ret);
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
