#ifndef __DB_H__
#define __DB_H__

#include "buffer.h"
#include "table.h"
#include "row.h"

#define DEFAULT_DB_NAME ".db.db"

typedef enum { META_COMMAND_SUCCESS, META_COMMAND_UNRICOGNIZED_COMMAND } MetaCommandResult;
MetaCommandResult do_meta_command(buf_t* buf, table_t* table);

typedef enum { STATEMENT_INSERT, STATEMENT_SELECT } StatementKind;
typedef struct __statement {
  StatementKind kind;
  row_t row_to_insert;
} statement_t;

typedef enum { 
  PREPARE_SUCCESS, 
  PREPARE_NEGATIVE_ID, 
  PREPARE_STRING_TOO_LONG,
  PREPARE_SYTAX_ERROR, 
  PREPARE_UNRECOGNIZED_STATEMENT 
} PrepareResult; 

PrepareResult prepare_statement(buf_t*, statement_t*);

typedef enum {
  EXECUTE_SUCCESS,
  EXECUTE_DUPLICATE_KEY,
  EXECUTE_TABLE_FULL
} ExecuteResult;

ExecuteResult execute_statement(statement_t* statement, table_t* table);

table_t* db_open(const char* filename);
void db_close(table_t*);

#endif