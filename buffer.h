#ifndef __BUFFER_H__
#define __BUFFER_H__
#include <stdio.h>

// ---------- buffer ------------- 
#define MAX_BUF_SIZE 1024

typedef struct __buf {
  char buf[MAX_BUF_SIZE];
  size_t size;
} buf_t;

buf_t* new_buf();

#endif