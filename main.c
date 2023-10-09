#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUF_SIZE 1024

// ---------- buffer ------------- 
typedef struct __buf {
  char buf[MAX_BUF_SIZE];
  ssize_t size;
} buf_t;

buf_t* new_buf();

// ------- command line -------- 
void readline_from_stdin(buf_t*);
void print_prompt();

// ----------- sql -------------
typedef enum { META_COMMAND_SUCCESS, META_COMMAND_UNRICOGNIZED_COMMAND } MetaCommandResult;
MetaCommandResult do_meta_command(buf_t* buf);

typedef enum { STATEMENT_INSERT, STATEMENT_SELECT } StatementKind;
typedef struct __statement {
  StatementKind kind;
} statement_t;

typedef enum { PREPARE_SUCCESS, PREPARE_UNRECOGNIZED_STATEMENT } PrepareResult; 
PrepareResult prepare_statement(buf_t*, statement_t*);

int main(int argc, char** argv) {
  buf_t* read_buf = new_buf();
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
    return PREPARE_SUCCESS;
  }
  if(strncmp(buf->buf, "select", 6) == 0) {
    statement->kind = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }

  return PREPARE_UNRECOGNIZED_STATEMENT;
}
