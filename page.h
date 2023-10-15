#ifndef __PAGE_H__
#define __PAGE_H__
#include <stdint.h>

#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 100
#define ROWS_PER_PAGE (PAGE_SIZE / ROW_SIZE)
#define TABLE_MAX_ROWS (ROWS_PER_PAGE * TABLE_MAX_PAGES)

typedef struct {
  int file_descriptor;
  uint32_t file_length;
  uint32_t num_pages;
  void* pages[TABLE_MAX_PAGES];
} page_t;

void* get_page(page_t*, uint32_t page_num);
page_t* page_open(const char* filename);
void page_flush(page_t* pager, uint32_t page_num);

#endif