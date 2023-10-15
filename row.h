#ifndef __ROW_H__
#define __ROW_H__
#include <stdint.h>
#include "def.h"

// --------- row ------------ 
#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

typedef struct {
  uint32_t id;
  char username[COLUMN_USERNAME_SIZE];
  char email[COLUMN_EMAIL_SIZE];
} row_t;

#define ID_SIZE size_of_attribute(row_t, id)
#define USERNAME_SIZE size_of_attribute(row_t, username)
#define EMAIL_SIZE size_of_attribute(row_t, email)
#define ID_OFFSET 0
#define USERNAME_OFFSET (ID_OFFSET + ID_SIZE)
#define EMAIL_OFFSET (USERNAME_OFFSET + USERNAME_SIZE)
#define ROW_SIZE (ID_SIZE + USERNAME_SIZE + EMAIL_SIZE)

void serialize_row(row_t* , void*);
void deserialize_row(void* , row_t*);

#endif