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
  FunctionRefTy,
  ClosureTy
};

typedef struct _bt_value_t {
  int32_t type;
  int32_t size; // as in fields
  // bt_value_t *data[];
} bt_value_t;

typedef struct _bt_int64_t {
  int32_t type;
  int32_t size; // as in fields
  int64_t data;
} bt_int64_t;

typedef struct _bt_fptr_t {
  int32_t type;
  int32_t size; // as in fields
  void *ptr;
  int64_t nargs;
} bt_fptr_t;

#define bt_value_data(t) ((bt_value_t**)((char*)(t) + sizeof(bt_value_t)))

extern char *bt_true, *bt_false;

extern "C" char *bt_new_int64(int num);
extern "C" char *bt_binary_int64(int op, char *lhs, char *rhs);
extern "C" int32_t bt_as_bool(char *cond);
extern "C" char *bt_new_fptr(char *fp, int nargs);
extern "C" char *bt_get_callable(char *val);
extern "C" char *bt_box(char *val);
extern "C" char *bt_unbox(char *box);
extern "C" char *bt_set_box(char *box, char *new_val);
extern "C" char *bt_error(void);

bool bt_is_int64(bt_value_t *val);
bool bt_is_fptr(bt_value_t *val);
bool bt_is_box(bt_value_t *val);

int64_t bt_to_int64(bt_value_t *val);

void init_butterfly(void);
void init_butterfly_per_module(void);

#endif
