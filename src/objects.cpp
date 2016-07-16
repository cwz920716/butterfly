#include "common.h"
#include "ast.h"

char *LogErrorN(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}

extern "C"
char *bt_new_int64(int num) {
  // for now, just use malloc
  // gc support will be added in future
  int64_t num_l = num;
  bt_value_t *ptr = (bt_value_t *)aligned_alloc(8, sizeof(bt_value_t) + sizeof(uint64_t));
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

int bt_is_int64(bt_value_t *val) {
  return val->type == I64Ty && val->size == 1;
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
