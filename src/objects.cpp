#include "common.h"
#include "ast.h"

char *LogErrorN(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}

#define ISA(val, Ty) if (val->type == Ty) return Ty;

extern "C"
int32_t bt_typeof(char *val) {
  bt_value_t *bt_val = (bt_value_t *) val;
  return bt_val->type;
}

extern "C"
char *bt_new_int64(int num) {
  // for now, just use malloc
  // gc support will be added in future
  uintptr_t num_l = num;
  bt_value_t *ptr = (bt_value_t *)aligned_alloc(8, sizeof(bt_value_t) + sizeof(bt_value_t *));
  if (!ptr) {
    // allocation failed. should perform gc
    // for now, return Null
    return LogErrorN("alloc failed.");
  }

  ptr->type = I64Ty;
  ptr->size = 1;
  bt_value_t **data = bt_value_data(ptr);
  data[0] = (bt_value_t *) num_l;

  return (char *) ptr;
}

extern "C"
char *bt_binary_int64(int op, char *lhs, char *rhs) {
  bt_value_t *lhs_ref = (bt_value_t *) lhs;
  bt_value_t *rhs_ref = (bt_value_t *) rhs;

  if (!bt_is_int64(lhs_ref) || !bt_is_int64(rhs_ref)) {
    return nullptr;
  }

  auto lhs_v = bt_to_int64(lhs_ref);
  auto rhs_v = bt_to_int64(rhs_ref);
  uint64_t res_v = 0;

  switch (op) {
  case tok_add:
    res_v = lhs_v + rhs_v;
    break;
  case tok_sub:
    res_v = lhs_v - rhs_v;
    break;
  case tok_mul:
    res_v = lhs_v * rhs_v;
    break;
  case tok_div:
    res_v = lhs_v / rhs_v;
    break;
  case tok_eq:
    res_v = (lhs_v == rhs_v) ? 1 : 0;
    return res_v ? bt_true : bt_false;
  case tok_gt:
    res_v = (lhs_v > rhs_v) ? 1 : 0;
    return res_v ? bt_true : bt_false;
  case tok_lt:
    res_v = (lhs_v < rhs_v) ? 1 : 0;
    return res_v ? bt_true : bt_false;
  case tok_and:
    return ( (lhs_v != 0) && (rhs_v != 0) ) ? bt_true : bt_false;
  case tok_or:
    return ( (lhs_v != 0) || (rhs_v != 0) ) ? bt_true : bt_false;
  case tok_not:
    return (lhs_v == 0) ? bt_true : bt_false;
  default:
    return LogErrorN("invalid binary operator or not implemented yet.");
  }

  return bt_new_int64(res_v);
}


extern "C" 
int32_t bt_as_bool(char *cond) {
  if (cond == nullptr) return 0;
  bt_value_t *cond_ref = (bt_value_t *) cond;

  if (bt_is_int64(cond_ref)) {
    return bt_to_int64(cond_ref) != 0;
  } 
  return 1;
}

extern "C"
char *bt_new_fptr(char *fp, int nargs) {
  uintptr_t nargs_p = nargs;
  bt_value_t *ptr = (bt_value_t *)aligned_alloc(8, sizeof(bt_value_t) + sizeof(bt_value_t *) * 2);
  if (!ptr) {
    // allocation failed. should perform gc
    // for now, return Null
    return LogErrorN("alloc failed.");
  }

  ptr->type = FunctionRefTy;
  ptr->size = 2;
  bt_value_t **data = bt_value_data(ptr);
  data[0] = (bt_value_t *) fp;
  data[1] = (bt_value_t *) nargs_p;

  return (char *) ptr;
}

extern "C" 
char *bt_get_callable(char *val) {
  bt_value_t *fptr = (bt_value_t *) val;
  bt_value_t **data;

  if (bt_is_fptr(fptr)) {
    data = bt_value_data(fptr);
    return (char *) data[0];
  }

  if (bt_is_closure(fptr)) {
    data = bt_value_data(fptr);
    return (char *) data[0]; 
  }

  return LogErrorN("not a callable object.");
}

extern "C" 
char *bt_box(char *val) {
  bt_value_t *ref = (bt_value_t *) val;
  bt_value_t *ptr = (bt_value_t *)aligned_alloc(8, sizeof(bt_value_t) + sizeof(bt_value_t *));
  if (!ptr) {
    // allocation failed. should perform gc
    // for now, return Null
    return LogErrorN("alloc failed.");
  }

  ptr->type = BoxTy;
  ptr->size = 1;
  bt_value_t **data = bt_value_data(ptr);
  data[0] = ref;

  return (char *) ptr;
}

extern "C"
char *bt_closure(char *fp, int n, char **members) {
  bt_value_t *ptr = (bt_value_t *)aligned_alloc(8, sizeof(bt_value_t) + sizeof(bt_value_t *) * (n + 1));
  if (!ptr) {
    // allocation failed. should perform gc
    // for now, return Null
    return LogErrorN("alloc failed.");
  }

  ptr->type = ClosureTy;
  ptr->size = n;
  bt_value_t **data = bt_value_data(ptr);
  data[0] = (bt_value_t *) fp;

  for (int i = 0; i < n; i++) {
    bt_value_t *box = (bt_value_t *) members[i];
    if (bt_is_box(box)) {
      data[i+1] = box;
    } else
      return LogErrorN("closure cannot take non-box member.");
  }

  return (char *) ptr;
}

extern "C"
char *bt_getfield(char *object, int n) {
  bt_value_t *bt_val = (bt_value_t *) object;
  bt_value_t **data;

  if (bt_is_closure(bt_val)) {
    data = bt_value_data(bt_val);
    if (n < bt_val->size) {
      return (char *) data[n];
    } else
      return LogErrorN("getfield out-of-bound.");
  }

  return LogErrorN("not a closure.");
}

extern "C" 
char *bt_unbox(char *box) {
  bt_value_t *bt_val = (bt_value_t *) box;
  bt_value_t **data;

  if (bt_is_box(bt_val)) {
    data = bt_value_data(bt_val);
    return (char *) data[0];
  }

  return LogErrorN("not a box object.");
}

extern "C" 
char *bt_set_box(char *box, char *new_val) {
  bt_value_t *bt_val = (bt_value_t *) box;
  bt_value_t **data;

  if (bt_is_box(bt_val)) {
    data = bt_value_data(bt_val);
    bt_value_t *old_data = data[0];
    data[0] = (bt_value_t *) new_val;
    return (char *) old_data;
  }

  return LogErrorN("not a box object.");
}

extern "C" char *bt_error() {
  LogErrorN("Runtime error: not a callable object");
  exit(-1);
  return LogErrorN("This should NEVER be printed out.");
}

bool bt_is_int64(bt_value_t *val) {
  if (!val) return false;
  return val->type == I64Ty && val->size == 1;
}

bool bt_is_fptr(bt_value_t *val) {
  if (!val) return false;
  return val->type == FunctionRefTy && val->size == 2;
}

bool bt_is_box(bt_value_t *val) {
  if (!val) return false;
  return val->type == BoxTy && val->size == 1;
}

bool bt_is_closure(bt_value_t *val) {
  if (!val) return false;
  return val->type == ClosureTy;
}

int64_t bt_to_int64(bt_value_t *val) {
  bt_value_t **data = bt_value_data(val);
  return (int64_t) data[0];
}

char *bt_true, *bt_false;

void init_butterfly(void) {
  bt_true = bt_new_int64(1);
  bt_false = bt_new_int64(0);
}
