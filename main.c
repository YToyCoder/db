#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_BUF_SIZE 1024

#define size_of_attribute(T, Attr) sizeof(((T*)0)->Attr)
typedef uint8_t db_bool;
#define db_true 1
#define db_false 0
#define DEFAULT_DB_NAME ".db.db"

// ---------- buffer ------------- 
typedef struct __buf {
  char buf[MAX_BUF_SIZE];
  size_t size;
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

// ----------- table & page & cursor ------------ 
#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct {
  int file_descriptor;
  uint32_t file_length;
  uint32_t num_pages;
  void* pages[TABLE_MAX_PAGES];
} page_t;

void* get_page(page_t*, uint32_t page_num);
page_t* page_open(const char* filename);
void page_flush(page_t* pager, uint32_t page_num);

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

void* row_slot(table_t* table, uint32_t row_num);
table_t* new_table();
void free_table(table_t* table);
cursor_t* table_start(table_t* table);
cursor_t* table_end(table_t* table);
void* cursor_value(cursor_t* cursor);
void  cursor_advance(cursor_t* cursor);

// ------------- node ----------------

typedef enum { NODE_INTERNAL, NODE_LEAF } NodeKind;

/*
 * Common Node Header Layout
 */
#define NODE_TYPE_SIZE sizeof(uint8_t)
#define NODE_TYPE_OFFSET 0
#define IS_ROOT_SIZE sizeof(uint8_t)
#define IS_ROOT_OFFSET NODE_TYPE_SIZE
#define PARENT_POINTER_SIZE sizeof(uint32_t)
#define PARENT_POINTER_OFFSET (IS_ROOT_OFFSET + IS_ROOT_SIZE)
#define COMMON_NODE_HEADER_SIZE (NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE)

/*
+ * Leaf Node Header Layout
+ */
#define LEAF_NODE_NUM_CELLS_SIZE sizeof(uint32_t)
#define LEAF_NODE_NUM_CELLS_OFFSET COMMON_NODE_HEADER_SIZE
#define LEAF_NODE_HEADER_SIZE (COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE)

/*
 * Leaf Node Body Layout
 */
#define LEAF_NODE_KEY_SIZE sizeof(uint32_t)
#define LEAF_NODE_KEY_OFFSET 0
#define LEAF_NODE_VALUE_SIZE  ROW_SIZE
#define LEAF_NODE_VALUE_OFFSET (LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE)
#define LEAF_NODE_CELL_SIZE (LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE)
#define LEAF_NODE_SPACE_FOR_CELLS (PAGE_SIZE - LEAF_NODE_HEADER_SIZE)
#define LEAF_NODE_MAX_CELLS (LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE)

uint32_t* leaf_node_num_cells(void* node) {
  return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

void* leaf_node_cell(void* node, uint32_t cell_num) {
  return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t* leaf_node_key(void* node, uint32_t cell_num) {
  return leaf_node_cell(node, cell_num);
}

void* leaf_node_value(void* node, uint32_t cell_num) {
  return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}


void initialize_leaf_node(void* node) { *leaf_node_num_cells(node) = 0; }

void leaf_node_insert(cursor_t* cursor, uint32_t key, row_t* value);

// ----------- sql -------------
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
  EXECUTE_TABLE_FULL
} ExecuteResult;

ExecuteResult execute_statement(statement_t* statement, table_t* table);

table_t* db_open(const char* filename);
void db_close(table_t*);
void print_constants();
void print_leaf_node(void* node);

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
      case EXECUTE_TABLE_FULL:
        printf("Error: Table full.\n");
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

ExecuteResult execute_insert(statement_t* statement, table_t* table) {
  void* node = get_page(table->pager, table->root_page_num);
  if ((*leaf_node_num_cells(node) >= LEAF_NODE_MAX_CELLS))
    return EXECUTE_TABLE_FULL;

  row_t* row_to_insert = &(statement->row_to_insert);
  cursor_t* cursor = table_end(table);

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

page_t* page_open(const char* filename) {
  int fd = open(filename, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);

  if ( fd == -1 ) {
    printf("Unable to open file.\n");
    exit(EXIT_FAILURE);
  }

  off_t file_length = lseek(fd, 0 , SEEK_END);

  page_t* pager = malloc(sizeof(page_t));
  pager->file_descriptor = fd;
  pager->file_length = file_length;
  pager->num_pages = file_length / PAGE_SIZE;

  if (file_length % PAGE_SIZE != 0) {
    printf("Db file is not a whole number of pages. Corrupt file.\n");
    exit(EXIT_FAILURE);
  }

  for (uint32_t i=0; i<TABLE_MAX_PAGES; i++) {
    pager->pages[i] = NULL;
  }

  return pager;
}

void* get_page(page_t* pager, uint32_t page_num) {
  if(page_num > TABLE_MAX_PAGES) {
    printf("Tried to fetch page number out of bounds. %d > %d", page_num, TABLE_MAX_PAGES);
    exit(EXIT_FAILURE);
  }

  if (pager->pages[page_num] == NULL) {
    void* page = malloc(PAGE_SIZE);
    uint32_t num_pages = pager->file_length / PAGE_SIZE;

    if (pager->file_length % PAGE_SIZE) {
      num_pages += 1;
    }

    if (page_num <= num_pages) {
      lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
      ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
      if (bytes_read == -1) {
        printf("Error reading file \n");
        exit(EXIT_FAILURE);
      }
    }

    pager->pages[page_num] = page;

    if (page_num >= pager->num_pages) {
      pager->num_pages = page_num + 1;
    }
  }

  return pager->pages[page_num];
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

void page_flush(page_t* pager, uint32_t page_num) {
  if (pager->pages[page_num] == NULL) {
    printf("Tried to flush null page\n");
    exit(EXIT_FAILURE);
  }

  off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);

  if (offset == -1) {
    printf("Error seeking.\n");
    exit(EXIT_FAILURE);
  }

  ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);

  if (bytes_written == -1) {
    printf("Error writing\n");
    exit(EXIT_FAILURE);
  }
}

cursor_t* table_start(table_t* table) {
  cursor_t* cursor = malloc(sizeof(cursor_t));
  cursor->table = table;
  cursor->page_num = table->root_page_num;
  cursor->cell_num = 0;

  void* root_node = get_page(table->pager, table->root_page_num);
  uint32_t num_cells = *leaf_node_num_cells(root_node);
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
    cursor->end_of_table = db_true;
  }
}

void leaf_node_insert(cursor_t* cursor, uint32_t key, row_t* value) {
  void* node = get_page(cursor->table->pager, cursor->page_num);

  uint32_t num_cells = *leaf_node_num_cells(node);
  if (num_cells >= LEAF_NODE_MAX_CELLS) {
    // Node full
    printf("Need to implement splitting a leaf node.\n");
    exit(EXIT_FAILURE);
  }

  if (cursor->cell_num < num_cells) {
    // Make room for new cell
    for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
      memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1),
             LEAF_NODE_CELL_SIZE);
    }
  }

  *(leaf_node_num_cells(node)) += 1;
  *(leaf_node_key(node, cursor->cell_num)) = key;
  serialize_row(value, leaf_node_value(node, cursor->cell_num));
}

void print_constants() {
  printf("ROW_SIZE: %d\n", ROW_SIZE);
  printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
  printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
  printf("LEAF_NODE_CELL_SIZE: %d\n", LEAF_NODE_CELL_SIZE);
  printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
  printf("LEAF_NODE_MAX_CELLS: %d\n", LEAF_NODE_MAX_CELLS);
}

void print_leaf_node(void* node) {
  uint32_t num_cells = *leaf_node_num_cells(node);
  printf("leaf (size %d)\n", num_cells);
  for (uint32_t i = 0; i < num_cells; i++) {
    uint32_t key = *leaf_node_key(node, i);
    printf("  - %d : %d\n", i, key);
  }
}