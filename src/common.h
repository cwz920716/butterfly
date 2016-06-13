#ifndef _COMMON_H
#define _COMMON_H

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum token_type {
  tok_error,
  tok_eof,

  // commands
  tok_define,
  tok_lambda,
  tok_if,

  // operators
  tok_open,
  tok_close,
  tok_add,
  tok_sub,
  tok_mul,
  tok_div,
  tok_gt,
  tok_lt,
  tok_eq,

  // primitive types
  tok_symbol,
  tok_integer,

  // whitespaces
  tok_space,
  tok_newline
};

struct Token {
  token_type type;
  std::string literal;
};

token_type getToken(const char *text, int *len);

class Lexer {
public:
  const char *text;
  int next;
  
  Lexer(const char *_text) : text(_text), next(0) {}

  Token getNextToken() {
    token_type tok;
    Token token;
    const char* src = text + next;

    do {
      int tok_len = 0;
      tok = getToken(src, &tok_len);
      std::string lit(src, tok_len);
      token.type = tok;
      token.literal = lit;
      src += tok_len;
      next += tok_len;
    } while (tok == tok_space || tok == tok_newline);

    return token;
  }
};

#endif
