#include "buffer.h"

buf_t* new_buf() {
  buf_t* buf = (buf_t*) malloc(sizeof(buf_t));
  buf->size = 0;
  return buf;
}