#include "common.h"

extern Buffer buf;
static int result_block_no = 100;
static int result_item_count = 0;

static unsigned char* res_blk = NULL;
static unsigned char* res_blk_ptr = NULL;
static int res_blk_left_free_item = 0;

// 元组写入输出缓冲区
void writeItem(char* item_attr1, char* item_attr2){
    if(res_blk == NULL){
        res_blk = getNewBlockInBuffer(&buf);
        res_blk_left_free_item = BLOCK_ITEM_NUM;
        res_blk_ptr = res_blk;
    }

    if(res_blk_left_free_item == 0){
        printf("注:结果写入磁盘:%d\n", result_block_no);
        writeBlockToDisk(res_blk, result_block_no++, &buf);
        res_blk = getNewBlockInBuffer(&buf);
        res_blk_left_free_item = BLOCK_ITEM_NUM;
        res_blk_ptr = res_blk;
    }

    strncpy(res_blk_ptr, item_attr1, ATTR_SIZE);
    res_blk_ptr += ATTR_SIZE;
    strncpy(res_blk_ptr, item_attr2, ATTR_SIZE);
    res_blk_ptr += ATTR_SIZE;
    res_blk_left_free_item -= 1;

    result_item_count++;
}

void searchBlock(char* block_buf, int target_value){
    char str[5];
    str[5] = '\0';

    for(int i=0; i<BLOCK_ITEM_NUM; i++){
        for (int j = 0; j < ATTR_SIZE; j++)
        {
            str[j] = *(block_buf + i*ITEM_SIZE + j);
        }
        int item_attr = atoi(str);

        for (int j = 0; j < ATTR_SIZE; j++)
        {
            str[j] = *(block_buf + i*ITEM_SIZE + ATTR_SIZE + j);
        }
        int item_attr2 = atoi(str);


        if(target_value == item_attr){
            printf("(X=%d, Y=%d)\n", item_attr, item_attr2);
            writeItem(block_buf + i*ITEM_SIZE, block_buf + i*ITEM_SIZE + ATTR_SIZE);
        }
    }
}

// 基于线性搜索的关系选择算法
void lab4_1(){

    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    for(unsigned int i=1; i<=TABLE_S_MAX_BLOCK_NO; i++){
        char* block_buf = readBlockFromDisk(i, &buf);
        printf("读入数据块%d\n", i);
        searchBlock(block_buf, 50);
        freeBlockInBuffer(block_buf, &buf);
    }

    if(res_blk_left_free_item != 7){
        printf("注:结果写入磁盘:%d\n", result_block_no);
        writeBlockToDisk(res_blk, result_block_no++, &buf);
    }
    printf("满足选择条件的元组一共%d个\n", result_item_count);
    printf("IO读写一共%d次\n", buf.numIO);
    freeBuffer(&buf);
}
