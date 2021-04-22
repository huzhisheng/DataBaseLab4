#include "common.h"
extern Buffer buf;

#define TEMP_BLK_BEGIN 1201
static int temp_block_no = TEMP_BLK_BEGIN;

// 将缓冲区的8个块进行排序，排好序后依次写到TEMP_BLK磁盘块, 同时注意填写好块的最后8字节为下一块的块号
void sortItemInBuffer(){
    char* blk_ptr = buf.data + 1;
    int block_count = buf.numAllBlk-buf.numFreeBlk;
    for(int i=0; i < block_count; i++){
        translateBlock(blk_ptr);
        blk_ptr += BLOCK_SIZE + 1;
    }
    // 简单的冒泡排序
    for(int i=0; i < block_count * BLOCK_ITEM_NUM - 1; i++){    // 外循环为排序趟数，len个数进行len-1趟
        for(int j=0; j < block_count * BLOCK_ITEM_NUM - 1 - i; j++){
            int* item1 = getItemPtr(j);
            int* item2 = getItemPtr(j+1);
            
            if(!compareItem(item1, item2)){    // 假设根据Y值排序
                swapItem(item1, item2);
            }
        }
    }
    blk_ptr = buf.data + 1;
    // 排好序后，将缓冲区的数据写到磁盘的temp区域
    for(int i=0; i < block_count; i++){
        reverseTranslateBlock(blk_ptr);
        if(i < block_count-1)
            writeNextBlockNum(blk_ptr, temp_block_no + 1);
        else
            writeNextBlockNum(blk_ptr, 0);  // 每组的最后一块的next_block_num为0
        writeBlockToDisk(blk_ptr, temp_block_no++, &buf);
        blk_ptr += BLOCK_SIZE + 1;
    }
}


static int R_sorted_beg_no;
static int S_sorted_beg_no;
static int R_sorted_end_no;
static int S_sorted_end_no;
static int total_block_count1;
static int total_block_count2;
/*
参数说明
R_beg_no: R表起始磁盘块
R_end_no: R表结束磁盘块
S_beg_no: S表起始磁盘块
S_end_no: S表结束磁盘块
*/
void firstScan(int R_beg_no, int R_end_no, int S_beg_no, int S_end_no){
    
    if (buf.numFreeBlk != BUF_BLOCK_NUM)
    {
        perror("Buffer is dirty\n");
        return;
    }

    // 第一遍扫描排序, 对要排序的块按照8块一组读取到内存再进行冒泡排序后，再写回到磁盘的Temp磁盘块
    // 第一遍扫描排序, 对R表的进行8块一组排序
    R_sorted_beg_no = temp_block_no;    // 经过8块一组排序后的R表起始磁盘块
    R_sorted_end_no = -1;               // 经过8块一组排序后的R表结束磁盘块

    total_block_count1 = R_end_no - R_beg_no + 1;
    int processed_block_count = 0;
    while(processed_block_count < total_block_count1){
        while(buf.numFreeBlk){
            readBlockFromDisk(R_beg_no + processed_block_count, &buf);
            processed_block_count += 1;
        }
        sortItemInBuffer();
    }
    R_sorted_end_no = temp_block_no - 1;
    // 第一遍扫描排序, 对S表的进行8块一组排序
    S_sorted_beg_no = temp_block_no;    // 经过8块一组排序后的S表起始磁盘块
    S_sorted_end_no = -1;               // 经过8块一组排序后的S表结束磁盘块

    total_block_count2 = S_end_no - S_beg_no + 1;
    processed_block_count = 0;
    while(processed_block_count < total_block_count2){
        while(buf.numFreeBlk){
            readBlockFromDisk(S_beg_no + processed_block_count, &buf);
            processed_block_count += 1;
        }
        sortItemInBuffer();
    }
    S_sorted_end_no = temp_block_no - 1;

    // 第一遍扫描结束时buffer应该是空的
    if (buf.numFreeBlk != BUF_BLOCK_NUM)
    {
        perror("Buffer is dirty\n");
        return;
    }
}



// beg_res_blk_no代表存放结果的开始磁盘块号
void secondScan(int beg_res_blk_no){
    int group_count1 = ceil((float)total_block_count1 / BUF_BLOCK_NUM);   //计算出R表的分组数 -- 2
    int group_count2 = ceil((float)total_block_count2 / BUF_BLOCK_NUM);   //计算出S表的分组数 -- 4
    int processed_block_count = 0;  // 将已处理的块数重置
    
    // 记录R表每组还剩多少元组没处理 -- 该数组长度为2, 未超过10
    int* group_left_count1 = (int*)malloc(sizeof(int) * group_count1);   
    memset((void*)group_left_count1, 0, sizeof(int) * group_count1);
    for(int i=0; i<group_count1; i++){
        group_left_count1[i] = BLOCK_ITEM_NUM;
    }
    // 记录S表每组还剩多少元组没处理 -- 该数组长度为4, 未超过10
    int* group_left_count2 = (int*)malloc(sizeof(int) * group_count2);   
    memset((void*)group_left_count2, 0, sizeof(int) * group_count2);
    for(int i=0; i<group_count2; i++){
        group_left_count2[i] = BLOCK_ITEM_NUM;
    }

    // 存放R表各组的缓冲区地址, 如果一组已经处理完则对应的指针为NULL -- 该数组长度为2, 未超过10
    char** group_blk_ptr1 = (char**)malloc(sizeof(char*) * group_count1); 
    for(int i=0; i<group_count1; i++){
        group_blk_ptr1[i] = readBlockFromDisk(R_sorted_beg_no + i*BUF_BLOCK_NUM, &buf);
        translateBlock(group_blk_ptr1[i]);
    }
    // 存放S表各组的缓冲区地址, 如果一组已经处理完则对应的指针为NULL -- 该数组长度为4, 未超过10
    char** group_blk_ptr2 = (char**)malloc(sizeof(char*) * group_count2); 
    for(int i=0; i<group_count2; i++){
        group_blk_ptr2[i] = readBlockFromDisk(S_sorted_beg_no + i*BUF_BLOCK_NUM, &buf);
        translateBlock(group_blk_ptr2[i]);
    }

    // 第二遍扫描排序开始
    char* output_blk_ptr = getNewBlockInBuffer(&buf);   // 为输出缓冲区申请一个内存块 -- 一共有6组, 因此缓冲区还剩2个块可用, 不会溢出
    int output_left_free = 3;               // 当前输出缓冲区还可写的空间 
                                            // 一条连接数据长16字节，因此每个块只能放3条连接后的记录!
    int* item1 = getNextItem(group_blk_ptr1, group_left_count1, group_count1);  // R表的当前选出来元组
    int* item2 = getNextItem(group_blk_ptr2, group_left_count2, group_count2);  // S表的当前选出来元组
    int* next_item1 = getNextItem(group_blk_ptr1, group_left_count1, group_count1); // R表的下一个元组
    int* next_item2 = getNextItem(group_blk_ptr2, group_left_count2, group_count2); // S表的下一个元组
    while(1){   // 连接操作 -- 当R表或S表任何一个表扫描完时结束循环
        if(output_left_free == 0){  // 输出缓冲区已满, 必须将输出缓冲区刷新
            
        }
        // while每循环一次输出缓冲区多一个元组(Item)
        if(item1[0] == item2[0]){   // 如果R.A = S.C
            goto output;
        }

    output: // 输出元组到输出缓冲区, 先不用按照ASCII码写，直接就按照值来写
        memcpy(output_blk_ptr + (3 - output_left_free)*16, item1, ITEM_SIZE);
        memcpy(output_blk_ptr + (3 - output_left_free)*16 + ITEM_SIZE, item2, ITEM_SIZE);
        output_left_free -= 1;
        if(item1[0] == item2)

    }

}

void lab4_4(){
    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
}