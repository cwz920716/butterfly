#ifndef _OBJECTS_H
#define _OBJECTS_H

#include <cstdint>

#define container_of(ptr, type, member) \
    ((type *) ((char *)(ptr) - offsetof(type, member)))

enum DataType {
  I64Ty,
  SymTy,
  BoxTy,
  ConsTy,
  FunctRefTy,
  ClosureTy
};

typedef struct _bt_value_t {
  int32_t type;
  int32_t size; // as in fields
  // bt_value_t *data[];
} bt_value_t;

#define bt_value_data(t) ((bt_value_t**)((char*)(t) + sizeof(bt_value_t)))

extern char *bt_true, *bt_false;

extern "C" char *bt_new_int64(int num);
extern "C" char *bt_binary_int64(int op, char *lhs, char *rhs);
extern "C" int32_t bt_as_bool(char *cond);

bool bt_is_int64(bt_value_t *val);
int64_t bt_to_int64(bt_value_t *val);

void init_butterfly(void);
void init_butterfly_per_module(void);

#endif
