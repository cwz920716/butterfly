#ifndef _SHARED_H
#define _SHARED_H

#include <iostream>     // std::cout
#include <fstream>      // std::ifstream

int readFile(char *src_file, char *dest_buf, int len) {
  std::ifstream is(src_file, std::ifstream::binary);
  if (is) {
    is.read(dest_buf, len);
    int cnt = is.gcount();
    is.close();
    return cnt;
  } else
    return -1;
}

#endif
