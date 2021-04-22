#include "common.h"
extern Buffer buf;

// lab4_5_minus.c 一遍扫描创建的有序单组会临时存放在1501开始的磁盘块
// lab4_5_minus.c 二遍扫描存储连接结果存放在从701开始的磁盘块
static int temp_block_no = 1501;
static int output_blk_no = 701;

// 将缓冲区的8个块进行排序，排好序后依次写到TEMP_BLK磁盘块, 同时注意填写好块的最后8字节为下一块的块号
static void sortItemInBuffer(){
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
static void firstScan(int R_beg_no, int R_end_no, int S_beg_no, int S_end_no){
    
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


// 输出缓冲区控制模块
static char* output_blk_ptr = NULL;
static int output_left_free = 0;
static int output_item_count = 0;   // 统计集合运算后的元组数量
static int last_output[2] = {0, 0}; // 结果要去重, 每次输出一个元组时要和上次的进行比较是否重复
static void outputItem(int* item, int attr_count){
    if((last_output[0] == item[0]) && (last_output[1] == item[1])){
        return;
    }
    output_item_count++;
    last_output[0] = item[0];
    last_output[1] = item[1];
    if(output_blk_ptr == NULL){
        output_blk_ptr = getNewBlockInBuffer(&buf);
        output_left_free = 14;  // 留8字节空间存放下一个块的块号
    }
    int* output_int_ptr = (int*)output_blk_ptr;
    for(int i=0; i<attr_count; i++){
        output_int_ptr[14 - output_left_free] = item[i];
        output_left_free -= 1;
        if(output_left_free == 0){
            printf("注: 结果写入磁盘: %d\n", output_blk_no);
            writeNextBlockNum(output_blk_ptr, output_blk_no+1);
            reverseTranslateBlock(output_blk_ptr);
            writeBlockToDisk(output_blk_ptr, output_blk_no++, &buf);
            output_blk_ptr = getNewBlockInBuffer(&buf);
            memset(output_blk_ptr, 0, 64);
            output_int_ptr = (int*)output_blk_ptr;
            output_left_free = 14;  // 留8字节空间存放下一个块的块号
        }
    }
}

static void outputRefresh(){
    if(output_left_free < 14){
        printf("注: 结果写入磁盘: %d\n", output_blk_no);
        writeNextBlockNum(output_blk_ptr, output_blk_no+1);
        reverseTranslateBlock(output_blk_ptr);
        writeBlockToDisk(output_blk_ptr, output_blk_no++, &buf);
        output_blk_ptr = getNewBlockInBuffer(&buf);
        memset(output_blk_ptr, 0, 64);
        output_left_free = 14;  // 留8字节空间存放下一个块的块号
    }
}


// beg_res_blk_no代表存放结果的开始磁盘块号
static void secondScan(){
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
    int item1[2];
    int item2[2];
    getNextItem(item1, group_blk_ptr1, group_left_count1, group_count1);  // R表的当前选出来元组
    getNextItem(item2, group_blk_ptr2, group_left_count2, group_count2);  // S表的当前选出来元组
    while(item1[0] && item2[0]){   // 连接操作 -- 当R表或S表任何一个表扫描完时结束循环
        // 因为S表数量多于R表, 以S表为主
        if(item1[0] < item2[0]){
            getNextItem(item1, group_blk_ptr1, group_left_count1, group_count1);
        }else if(item1[0] > item2[0]){
            outputItem(item2, 2);
            getNextItem(item2, group_blk_ptr2, group_left_count2, group_count2);
        }else{
            if(item1[1] < item2[1]){
                getNextItem(item1, group_blk_ptr1, group_left_count1, group_count1);
            }else if(item1[1] > item2[1]){
                outputItem(item2, 2);
                getNextItem(item2, group_blk_ptr2, group_left_count2, group_count2);
            }else{  // item1和item2完全相同
                getNextItem(item1, group_blk_ptr1, group_left_count1, group_count1);
                getNextItem(item2, group_blk_ptr2, group_left_count2, group_count2);
            }
        }
    }

    while(item2[0]){    // 如果S表还有元素没取完
        outputItem(item2, 2);
        getNextItem(item2, group_blk_ptr2, group_left_count2, group_count2);
    }

    outputRefresh();
}

void lab4_5_minus(){
    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
    firstScan(1, 16, 17, 48);
    secondScan();
    printf("R和S的差集(S-R)有%d个元素\n", output_item_count);
    printf("IO读写一共%d次\n", buf.numIO);
    freeBuffer(&buf);
}