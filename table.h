#ifndef __TABLE_H__
#define __TABLE_H__

#include "page.h"
#include "def.h"

typedef struct {
  page_t* pager;
  uint32_t root_page_num;
} table_t;

typedef struct {
  table_t* table;
  uint32_t page_num;
  uint32_t cell_num;
  db_bool end_of_table;
} cursor_t;

cursor_t* table_start(table_t* table);
cursor_t* table_end(table_t* table);
cursor_t* table_find(table_t* table, uint32_t key);
void* cursor_value(cursor_t* cursor);
void  cursor_advance(cursor_t* cursor);

#endif