#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_BUF_SIZE 1024

#define size_of_attribute(T, Attr) sizeof(((T*)0)->Attr)

// ---------- buffer ------------- 
typedef struct __buf {
  char buf[MAX_BUF_SIZE];
  ssize_t size;
} buf_t;

buf_t* new_buf();

// ------- command line -------- 
void readline_from_stdin(buf_t*);
void print_prompt();

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

// ----------- table ------------ 
#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct {
  uint32_t num_rows;
  void* pages[TABLE_MAX_PAGES];
} table_t;

void* row_slot(table_t* table, uint32_t row_num);
table_t* new_table();
void free_table(table_t* table);

// ----------- sql -------------
typedef enum { META_COMMAND_SUCCESS, META_COMMAND_UNRICOGNIZED_COMMAND } MetaCommandResult;
MetaCommandResult do_meta_command(buf_t* buf);

typedef enum { STATEMENT_INSERT, STATEMENT_SELECT } StatementKind;
typedef struct __statement {
  StatementKind kind;
  row_t row_to_insert;
} statement_t;

typedef enum { PREPARE_SUCCESS, PREPARE_UNRECOGNIZED_STATEMENT } PrepareResult; 
PrepareResult prepare_statement(buf_t*, statement_t*);

typedef enum {
  EXECUTE_SUCCESS, 
  EXECUTE_TABLE_FULL
} ExecuteResult;

ExecuteResult execute_statement(statement_t* statement, table_t* table);

int main(int argc, char** argv) {
  buf_t* read_buf = new_buf();
  table_t* table = new_table();
  while(1) {
    print_prompt();
    readline_from_stdin(read_buf);

    if(read_buf->buf[0] == '.') {
      switch(do_meta_command(read_buf)) {
        case META_COMMAND_SUCCESS:
          continue;
        case META_COMMAND_UNRICOGNIZED_COMMAND:
          printf("unrecognized meta command '%s'\n", read_buf->buf);
          continue;
      }
    }

    statement_t statement;
    switch(prepare_statement(read_buf, &statement)) {
      case PREPARE_SUCCESS: break;
      case PREPARE_UNRECOGNIZED_STATEMENT:
        printf("unrecognized keyword at start of '%s'\n", read_buf->buf);
        break;
    }

    execute_statement(&statement, table);
  }
  return 0;
}

buf_t* new_buf() {
  buf_t* buf = (buf_t*) malloc(sizeof(buf_t));
  buf->size = 0;
  return buf;
}

void print_prompt() {
  printf("db > ");
}

void readline_from_stdin(buf_t* buf) {
  int read_size = 0;
  char read_char;
  while(read_size < MAX_BUF_SIZE && (read_char = getc(stdin)) != '\n') {
    buf->buf[read_size++] = read_char;
  }

  buf->size = read_size;
  buf->buf[read_size] = '\0';
}

MetaCommandResult do_meta_command(buf_t* buf) {
  if(strcmp(buf->buf, ".exit") == 0) {
    exit(0);
  }
  else {
    return META_COMMAND_UNRICOGNIZED_COMMAND;
  }
}

PrepareResult prepare_statement(buf_t* buf, statement_t* statement) {
  if(strncmp(buf->buf, "insert", 6) == 0) {
    statement->kind = STATEMENT_INSERT;
    int arg_assigned = sscanf(buf->buf, "insert %d %s %s", 
                              &(statement->row_to_insert.id), &(statement->row_to_insert.username), &(statement->row_to_insert.email));
    return PREPARE_SUCCESS;
  }
  if(strncmp(buf->buf, "select", 6) == 0) {
    statement->kind = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }

  return PREPARE_UNRECOGNIZED_STATEMENT;
}

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

void* row_slot(table_t* table, uint32_t row_num) {
  uint32_t page_num = row_num / ROWS_PER_PAGE;
  void* page = table->pages[page_num];

  if (page == NULL) {
    page = table->pages[page_num] = malloc(PAGE_SIZE);
  }

  uint32_t row_offset = row_num % ROWS_PER_PAGE;
  uint32_t byte_offset = row_offset * ROW_SIZE;
  return page + byte_offset;
}

ExecuteResult execute_insert(statement_t* statement, table_t* table) {
  if(table->num_rows >= TABLE_MAX_ROWS) {
    return EXECUTE_TABLE_FULL;
  }

  row_t* row_to_insert = &(statement->row_to_insert);
  serialize_row(row_to_insert, row_slot(table, table->num_rows));
  table->num_rows += 1;

  return EXECUTE_SUCCESS;
}

void print_row(row_t* row) {
  printf("(%d %s %s)\n", row->id, row->username, row->email);
}

ExecuteResult execute_select(statement_t* statement, table_t* table) {
  row_t row;
  for(uint32_t i = 0; i < table->num_rows; i++) {
    deserialize_row(row_slot(table, i), &row);
    print_row(&row);
  }
  return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(statement_t* statement, table_t* table) {
  switch(statement->kind) {
    case STATEMENT_INSERT: return execute_insert(statement, table);
    case STATEMENT_SELECT: return execute_select(statement, table);
  }
}

table_t* new_table() {
  table_t* table = (table_t*) malloc(sizeof(table_t));
  table->num_rows = 0;
  for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    table->pages[i] = NULL;
  return table;
}
void free_table(table_t* table) {
  for(int i=0; table->pages[i]; i++) {
    free(table->pages[i]);
  }
  free(table);
}