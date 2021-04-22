#include "common.h"

extern Buffer buf;
static int output_blk_no = 101;

// 输出缓冲区控制模块, 因为不同题目的输出具体要求不同, 因此没有抽象成通用函数
static char* output_blk_ptr = NULL;
static int output_left_free = 0;
static int output_item_count = 0;   // 统计集合运算后的元组数量

static void outputItem(int* item, int attr_count){
    output_item_count++;

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

static void searchBlock(char* block_buf, int target_value){
    translateBlock(block_buf);
    int* blk_int_ptr = (int*)block_buf;
    int item[2] = {0};
    for(int i=0; i<7; i++){
        if(blk_int_ptr[2*i] == target_value){
            item[0] = blk_int_ptr[2*i];
            item[1] = blk_int_ptr[2*i + 1];
            printf("(X=%d, Y=%d)\n", item[0], item[1]);
            outputItem(item, 2);
        }
    }
}

void searchLinear(int beg_blk_no, int end_blk_no, int target){
    // 重置一下static变量, 因为lab4_1和lab4_3都会调用此函数, 如果不重置一些参数就会叠加
    output_blk_no = 101;
    output_blk_ptr = NULL;
    output_left_free = 0;
    output_item_count = 0;   // 统计集合运算后的元组数量

    for(unsigned int i=beg_blk_no; i<=end_blk_no; i++){
        char* block_buf = readBlockFromDisk(i, &buf);
        printf("读入数据块%d\n", i);
        searchBlock(block_buf, target);
        freeBlockInBuffer(block_buf, &buf);
    }

    outputRefresh();

    printf("\n满足选择条件的元组一共%d个\n", output_item_count);
    printf("\nIO读写一共%d次\n", buf.numIO);
}

// 基于线性搜索的关系选择算法
void lab4_1(){

    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    printf("==========================================\n");
    printf("基于线性搜索的选择算法 S.C=50\n");
    printf("==========================================\n\n");
    searchLinear(TABLE_R_MAX_BLOCK_NO+1, TABLE_S_MAX_BLOCK_NO, 50);
    freeBuffer(&buf);
}
