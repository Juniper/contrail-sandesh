#ifdef SANDESH
#include <stdio.h>
#endif // !SANDESH
#include "t_type.h"
#include "t_typedef.h"
#ifdef SANDESH
#include "t_program.h"
#include "t_struct.h"
#endif // !SANDESH

#include "md5.h"

void t_type::generate_fingerprint() {
  std::string material = get_fingerprint_material();
  md5_state_t ctx;
  md5_init(&ctx);
  md5_append(&ctx, (md5_byte_t*)(material.data()), (int)material.size());
  md5_finish(&ctx, (md5_byte_t*)fingerprint_);
}

t_type* t_type::get_true_type() {
  t_type* type = this;
  while (type->is_typedef()) {
    type = ((t_typedef*)type)->get_type();
  }
  return type;
}

#ifdef SANDESH
bool t_type::has_annotation(bool check_self,
                            const std::vector<t_field*> &members,
                            const t_program *program) const {
  bool has_annotation = false;
  // First check self annotations
  if (check_self) {
    has_annotation = t_type::has_key_annotation();
    if (has_annotation) {
      return has_annotation;
    }
  }
  // Next check member annotations
  std::vector<t_field*>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    if (((*m_iter)->get_type())->is_struct()) {
      const char *sname = ((*m_iter)->get_type())->get_name().c_str();
      t_struct *cstruct = program->get_struct(sname);
      if (!cstruct) {
        // Check in includes
        const std::vector<t_program*>& includes = program->get_includes();
        std::vector<t_program*>::const_iterator iter;
        for (iter = includes.begin(); iter != includes.end(); ++iter) {
          cstruct = (*iter)->get_struct(sname);
          if (cstruct) {
            break;
          }
        }
      }
      if (!cstruct) {
        throw "compiler error: CANNOT FIND struct: " + std::string(sname);
      }
      has_annotation = cstruct->has_key_annotation();
    } else {
      has_annotation = (*m_iter)->has_key_annotation();
    }
    if (has_annotation) {
      return has_annotation;
    }
  }
  return has_annotation;
}

bool t_struct::has_key_annotation() const {
  return t_type::has_annotation(true, members_, program_);
}

bool t_sandesh::has_key_annotation() const {
  return has_annotation(false, members_, program_);
}
#endif // !SANDESH
