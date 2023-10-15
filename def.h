#ifndef __DEF_H__
#define __DEF_H__
#include <stdint.h>
typedef uint8_t db_bool;
#define size_of_attribute(T, Attr) sizeof(((T*)0)->Attr)
#define db_true 1
#define db_false 0
#endif