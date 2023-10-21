#include "table.h"
#include "db.h"
#include "tree.h"

#include <stdlib.h>
#include <stdio.h>

cursor_t* table_start(table_t* table) {
  cursor_t* cursor = table_find(table, 0);

  void* node = get_page(table->pager, cursor->page_num);
  uint32_t num_cells = *leaf_node_num_cells(node);
  cursor->end_of_table = (num_cells == 0);

  return cursor;
}

cursor_t* table_end(table_t* table) {
  cursor_t* cursor = malloc(sizeof(cursor_t));
  cursor->table = table;
  cursor->page_num = table->root_page_num;
  cursor->end_of_table = db_true;

  void* root_node = get_page(table->pager, table->root_page_num);
  uint32_t num_cells = *leaf_node_num_cells(root_node);
  cursor->cell_num = num_cells;

  return cursor;
}

cursor_t* table_find(table_t* table, uint32_t key) {
  uint32_t root_page_num = table->root_page_num;
  void* root_node = get_page(table->pager, root_page_num);

  switch(get_node_kind(root_node)) {
    case NODE_LEAF:
      return leaf_node_find(table, root_page_num, key);
    case NODE_INTERNAL:
      return internal_node_find(table, root_page_num, key);
  }
}

void* cursor_value(cursor_t* cursor) {
  uint32_t page_num = cursor->page_num;
  void* page = get_page(cursor->table->pager, page_num);
  return leaf_node_value(page, cursor->cell_num);
}

void  cursor_advance(cursor_t* cursor) {
  uint32_t page_num = cursor->page_num;
  void* node = get_page(cursor->table->pager, page_num);

  cursor->cell_num += 1;
  if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
    /* Advance to next leaf node */
    uint32_t next_page_num = *leaf_node_next_leaf(node);
    if (next_page_num == 0) {
      cursor->end_of_table = db_true;
    } else {
      cursor->page_num = next_page_num;
      cursor->cell_num = 0;
    }
  }
}