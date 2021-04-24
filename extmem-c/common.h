#ifndef COMMON_H
#define COMMON_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "extmem.h"

#define TABLE_R_MAX_BLOCK_NO 16
#define TABLE_S_MAX_BLOCK_NO 48


#define BLOCK_SIZE 64
#define ITEM_SIZE 8
#define ATTR_SIZE 4

#define BUF_BLOCK_NUM 8
#define BLOCK_ITEM_NUM (BLOCK_SIZE/ITEM_SIZE-1)


// lab4_1.c
void lab4_1();
void searchLinear(int beg_blk_no, int end_blk_no, int target, int output_blk_no);
// lab4_2.c
void lab4_2();
void translateBlock(char* block_buf);
void reverseTranslateBlock(char* block_buf);
void writeNextBlockNum(char* block_buf, int block_no);
int compareItem(int* item1, int* item2);
int getNextBlockNum(char* block_buf);
int* getItemPtr(int int_no);
void swapItem(int* a, int* b);
void getNextItem(int* item, char** group_blk_ptr, int* group_left_count, int group_count);
// lab4_3.c
void lab4_3();
// lab4_4.c
void lab4_4();
// lab4_5_intersect.c
void lab4_5_intersect();
// lab4_5_union.c
void lab4_5_union();
// lab4_5_minus.c
void lab4_5_minus();
// lab4_5_test.c
void lab4_5_test();
#endif