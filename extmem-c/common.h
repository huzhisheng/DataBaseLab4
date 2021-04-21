#ifndef COMMON_H
#define COMMON_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "extmem.h"

#define TABLE_S_MAX_BLOCK_NO 16
#define TABLE_R_MAX_BLOCK_NO 48


#define BLOCK_SIZE 64
#define ITEM_SIZE 8
#define ATTR_SIZE 4

#define BUF_BLOCK_NUM 8
#define BLOCK_ITEM_NUM (BLOCK_SIZE/ITEM_SIZE-1)

void lab4_1();
void lab4_2();

#endif