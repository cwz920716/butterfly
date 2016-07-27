#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include "common.h"

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

std::string token_desc[] = {
                "tok_error",
                "tok_eof",
                "tok_define",
                "tok_set",
                "tok_lambda",
                "tok_if",
                "tok_cond",
                "tok_begin",
                "tok_open",
                "tok_close",
                "+",
                "-",
                "*",
                "/",
                ">",
                "<",
                "=",
                "and",
                "or",
                "not",
                "box",
                "unbox",
                "setbox",
                "nil",
                "tok_symbol",
                "tok_integer",
                "tok_space",
                "tok_newline",
              }; 

bool isWhitespace(char ch) {
  return ch == '\t' || ch == ' ' || ch == '\v' || ch == '\f' || ch == '\r';
}

bool isDelim(char ch) {
  return isspace(ch) || ch == ')' || ch == '\0';
}

bool maybeInt(const char *str) {
  char ch = str[0];
  return ch == '+' || ch == '-' || isdigit(ch);
}

int readInt(const char *str) {
  int i = 0;
  while (isdigit(str[i])) i++;
  return i;
}

token_type getIntToken(const char *text, int *len) {
  int sign = 0;
  if (text[0] == '+' || text[0] == '-') {
    sign = 1;
  }

  int int_len = readInt(text + sign);
  if (!isDelim(text[sign+int_len])) {
    return tok_error;
  }

  if (sign == 1 && int_len == 0) {
    if (text[0] == '+') {
      *len = 1;
      return tok_add;
    }
    if (text[0] == '-') {
      *len = 1;
      return tok_sub;
    }
  }

  *len = sign + int_len;
  return tok_integer;
}

int isIdChar0(char ch) {
  return isalpha(ch);
}

int isIdChar(char ch) {
  return isIdChar0(ch) || isdigit(ch) || ch == '!' || ch == '-' || ch == '?';
}

bool equalsKeyword(const char *a, int n, const char *b) {
  int b_n = strlen(b);
  if (n != b_n) return false;
  if (strncmp(a, b, n) == 0)
    return true;
  else
    return false;
}

token_type getIdToken(const char *text, int *len) {
  int n = 0;
  if (isIdChar0(text[0])) {
    do {
      n++;
    } while (isIdChar(text[n]));
  }

  if (n == 0) return tok_error;

  *len = n;
  if (equalsKeyword(text, n, "define"))
    return tok_define;
  if (equalsKeyword(text, n, "lambda"))
    return tok_lambda;
  if (equalsKeyword(text, n, "if"))
    return tok_if;
  if (equalsKeyword(text, n, "cond"))
    return tok_cond;
  if (equalsKeyword(text, n, "begin"))
    return tok_begin;
  if (equalsKeyword(text, n, "and"))
    return tok_and;
  if (equalsKeyword(text, n, "or"))
    return tok_or;
  if (equalsKeyword(text, n, "not"))
    return tok_not;
  if (equalsKeyword(text, n, "box"))
    return tok_box;
  if (equalsKeyword(text, n, "nil"))
    return tok_nil;
  if (equalsKeyword(text, n, "set!"))
    return tok_set;

  return tok_symbol;
}

token_type getToken(const char *text, int *len) {
  token_type ret = tok_error;
  int n = 0;

  if (text[0] == '\0') {
    return tok_eof;
  }
  
  if (text[0] == '\n') {
    *len = 1;
    return tok_newline;
  }

  if (text[0] == '(') {
    *len = 1;
    return tok_open;
  }

  if (text[0] == ')') {
    *len = 1;
    return tok_close;
  }

  if (text[0] == '*') {
    *len = 1;
    return tok_mul;
  }

  if (text[0] == '/') {
    *len = 1;
    return tok_div;
  }

  if (text[0] == '>') {
    *len = 1;
    return tok_gt;
  }

  if (text[0] == '<') {
    *len = 1;
    return tok_lt;
  }

  if (text[0] == '=') {
    *len = 1;
    return tok_eq;
  }

  if (isWhitespace(text[0])) {
    do {
      n++;
    } while (isWhitespace(text[n]));
    *len = n;
    return tok_space;
  }

  if (maybeInt(text)) {
    return getIntToken(text, len);
  }

  if (isIdChar0(text[0])) {
    return getIdToken(text, len);
  }

  return ret;
}

#ifdef _LEXER_MAIN_
int main(int argc, char **argv) {
  const char *test_scm = "(sum-of-squares (+ 5 1) (* 5 2))\n(+ (* 3 5) (- 10 6))\n(define (abs x) (if (< x 0) (- x) x))";
  Lexer lex(test_scm);
  Token cur;

  do {
    cur = lex.getNextToken();
    std::cout << token_desc[cur.type] << ":\t" << cur.literal << std::endl;
  } while (cur.type != tok_eof && cur.type != tok_eof);

  return 0;
}
#endif
