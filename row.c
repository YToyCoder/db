#include "row.h"
#include <memory.h>

void serialize_row(row_t* src, void* dst) {
  memcpy(dst + ID_OFFSET, &(src->id), ID_SIZE);
  memcpy(dst + USERNAME_OFFSET, &(src->username), USERNAME_SIZE);
  memcpy(dst + EMAIL_OFFSET, &(src->email), EMAIL_SIZE);
}

void deserialize_row(void* src, row_t* dst) {
  memcpy(&(dst->id), src + ID_OFFSET, ID_SIZE);
  memcpy(&(dst->username), src + USERNAME_OFFSET, USERNAME_SIZE);
  memcpy(&(dst->email), src + EMAIL_OFFSET, EMAIL_SIZE);
}