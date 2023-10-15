#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "buffer.h"
#include "row.h"
#include "page.h"
#include "table.h"
#include "tree.h"
#include "db.h"

// ------- command line -------- 
void readline_from_stdin(buf_t*);
void print_prompt();

// ----------- sql -------------

int main(int argc, char** argv) {
  char* db_name;

  if (argc < 2) {
    db_name = DEFAULT_DB_NAME;
  } else {
    db_name = argv[1];
  }

  buf_t* read_buf = new_buf();
  table_t* table = db_open(db_name);
  while(1) {
    print_prompt();
    readline_from_stdin(read_buf);

    if(read_buf->buf[0] == '.') {
      switch(do_meta_command(read_buf, table)) {
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
      case PREPARE_SYTAX_ERROR:
        printf("Syntax error. Cound not parse statement.\n");
        continue;
      case PREPARE_NEGATIVE_ID: 
        printf("ID Must be positive.\n");
        continue;
      case PREPARE_STRING_TOO_LONG:
        printf("String is to long.\n");
        continue;
      case PREPARE_UNRECOGNIZED_STATEMENT:
        printf("Unrecognized keyword at start of '%s'\n", read_buf->buf);
        break;
    }

    switch(execute_statement(&statement, table)) {
      case EXECUTE_SUCCESS:
        printf("Executed.\n");
        break;
      case EXECUTE_DUPLICATE_KEY:
        printf("Error: Duplicate key.\n");
        break;
      case EXECUTE_TABLE_FULL:
        printf("Error: Table full.\n");
        break;
    }
  }
  return 0;
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
