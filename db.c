#include "db.h"
#include "tree.h"
#include <stdlib.h>
#include <unistd.h>

MetaCommandResult do_meta_command(buf_t* buf, table_t* table) {
  if(strcmp(buf->buf, ".exit") == 0) {
    db_close(table);
    exit(EXIT_SUCCESS);
  }
  else if (strcmp(buf->buf, ".constants") == 0) {
    printf("Constants:\n");
    print_constants();
    return META_COMMAND_SUCCESS;
  }
  else if (strcmp(buf->buf, ".btree") == 0) {
    printf("Tree:\n");
    print_leaf_node(get_page(table->pager, 0));
    return META_COMMAND_SUCCESS;
  } 
  else {
    return META_COMMAND_UNRICOGNIZED_COMMAND;
  }
}

PrepareResult prepare_statement(buf_t* buf, statement_t* statement) {
  if(strncmp(buf->buf, "insert", 6) == 0) {
    statement->kind = STATEMENT_INSERT;
    char* keyword = strtok(buf->buf, " ");
    char* id_string = strtok(NULL, " ");
    char* username = strtok(NULL, " ");
    char* email = strtok(NULL, " ");

    if ( id_string == NULL || username == NULL || email == NULL ) {
      return PREPARE_SYTAX_ERROR;
    }

    int id = atoi(id_string);
    if (id < 0) {
      return PREPARE_NEGATIVE_ID;
    }

    if (strlen(username) > COLUMN_USERNAME_SIZE) {
      return PREPARE_STRING_TOO_LONG;
    }

    if (strlen(email) > COLUMN_EMAIL_SIZE) {
      return PREPARE_STRING_TOO_LONG;
    }

    statement->row_to_insert.id = id;
    strcpy(statement->row_to_insert.username, username);
    strcpy(statement->row_to_insert.email, email);

    return PREPARE_SUCCESS;
  }
  if(strncmp(buf->buf, "select", 6) == 0) {
    statement->kind = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }

  return PREPARE_UNRECOGNIZED_STATEMENT;
}

ExecuteResult execute_insert(statement_t* statement, table_t* table) {
  void* node = get_page(table->pager, table->root_page_num);
  uint32_t num_cells = (*leaf_node_num_cells(node));
  if (num_cells >= LEAF_NODE_MAX_CELLS)
    return EXECUTE_TABLE_FULL;

  row_t* row_to_insert = &(statement->row_to_insert);
  uint32_t key_to_insert = row_to_insert->id;
  cursor_t* cursor = table_find(table, key_to_insert);

  if (cursor->cell_num < num_cells) {
    uint32_t key_at_index = *leaf_node_key(node, cursor->cell_num);
    if (key_at_index == key_to_insert) {
      return EXECUTE_DUPLICATE_KEY;
    }
  }

  leaf_node_insert(cursor, row_to_insert->id, row_to_insert);

  free(cursor);

  return EXECUTE_SUCCESS;
}

void print_row(row_t* row) {
  printf("(%d %s %s)\n", row->id, row->username, row->email);
}

ExecuteResult execute_select(statement_t* statement, table_t* table) {
  cursor_t* cursor = table_start(table);

  row_t row;
  while(!cursor->end_of_table) {
    deserialize_row(cursor_value(cursor), &row);
    print_row(&row);
    cursor_advance(cursor);
  }

  free(cursor);

  return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(statement_t* statement, table_t* table) {
  switch(statement->kind) {
    case STATEMENT_INSERT: return execute_insert(statement, table);
    case STATEMENT_SELECT: return execute_select(statement, table);
  }
}

table_t* db_open(const char* filename) {
  page_t* pager = page_open(filename);

  table_t* table = malloc(sizeof(table_t));
  table->pager = pager;
  table->root_page_num = 0;

  if (pager->num_pages == 0) {
    void* root_node = get_page(pager, 0);
    initialize_leaf_node(root_node);
  }

  return table;
}

void db_close(table_t* table) {
  page_t* pager = table->pager;

  for (uint32_t i = 0; i < pager->num_pages; i++) {
    if (pager->pages[i] == NULL) {
      continue;
    }

    page_flush(pager, i);
    free(pager->pages[i]);
    pager->pages[i] = NULL;
  }

  int result  = close(pager->file_descriptor);
  if (result == -1) {
    printf("Error closing db file.\n");
    exit(EXIT_FAILURE);
  }

  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
    void* page = pager->pages[i];
    if (page) {
      free(page);
      pager->pages[i] = NULL;
    }
  }

  free(pager);
  free(table);
}