#include "common.h"
extern Buffer buf;

// lab4_4.c 一遍扫描创建的有序单组会临时存放在1201开始的磁盘块
// lab4_4.c 二遍扫描存储连接结果存放在从401开始的磁盘块
static int temp_block_no = 1201;
static int output_blk_no = 401;

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


// 48块/8有6组, 每组占一个缓冲块, 再拿一块作为输出缓冲块, 还剩一块可以作为record块

// 日志块, 用于解决连接操作时R表中出现某一关键属性值的元组数量过多(跨越了单个块)而导致重复读取数据块的问题

// 日志块的作用比较难以解释, 大致就是假如: 
// 属性X等于30的数据(30, y)在两个表中出现次数都很多, 这时如果要进行连接操作, 那么就必须以某一个表S的记录
// (30, y1)作为基准，同时重复扫描另一个表R中的具有该关键值的记录(30, y)多次, 每扫描完一遍再从表S中获取下一个
// 记录, 如果该记录依然是(30, y2)的形式, 那么就继续扫描表R中的所有(30, y)的数据, 而如果另一个表中(30, y)的元
// 组数量也很多, 分布在多个块中, 那么就很难重复地去扫描, 因此我们可以把另一个表中这些元组的y数据记录在record缓
// 冲区块中，因此当第二次需要扫描另一个块中所有的(30, y)元组时就可以直接在record块中查找这些y值即可。record块
// 的容量是16个, 意思是最多可存储16个(30, y)元组的y值, 而这个容量在本次实验中已经够用了。
static int record_left_free = 16;
static char* record_blk_ptr = NULL;
static int record_now_x = -1;   // 记录record块中现在记录的y值对应的元组(x, y)的x值
static void recordAttr(int item_attr){
    if(record_blk_ptr == NULL){
        record_blk_ptr = getNewBlockInBuffer(&buf);
    }

    int* record_int_ptr = (int*)record_blk_ptr;
    
    if(record_left_free > 0 ){
        record_int_ptr[16 - record_left_free] = item_attr;
        record_left_free -= 1;
    }
}

static void recordRefresh(){
    record_left_free = 16;
}

static int recordGetAttr(int index_no){
    int* record_int_ptr = (int*)record_blk_ptr;
    return record_int_ptr[index_no];
}


// 输出缓冲区控制模块
static char* output_blk_ptr = NULL;
static int output_left_free = 0;
static int output_item_count = 0;   // 统计总共连接结果
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

static void outputJoin(int attr_y){
    // 所谓连接操作就是(x1, y1)和(x1, y2)组装成(x1, y1, x1, y2)即可
    // 而所有的y1都已经记录在record块中
    int item[4];
    item[0] = record_now_x;
    item[1] = attr_y;   // 根据实验PPT, 连接时S表的放在R表前面
    item[2] = record_now_x;
    for(int i=0; i<16-record_left_free; i++){
        item[3] = recordGetAttr(i);
        outputItem(item, 4);
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
    int next_item1[2] = {0, 0};
    getNextItem(item1, group_blk_ptr1, group_left_count1, group_count1);  // R表的当前选出来元组
    getNextItem(item2, group_blk_ptr2, group_left_count2, group_count2);  // S表的当前选出来元组
    while(item1[0] && item2[0]){   // 连接操作 -- 当R表或S表任何一个表扫描完时结束循环
        // 因为S表数量多于R表, 以S表为主
        // 遇到一条S表中的记录(a, b)就去R表中查找所有(a, y)数据记录将y保存到record块中, 并连接后输出到输出块中, 再取S表中的下一条记录
        if(item1[0] < item2[0]){
            if(next_item1[0] > item1[0]){
                item1[0] = next_item1[0];
                item1[1] = next_item1[1];
            }else{
                getNextItem(item1, group_blk_ptr1, group_left_count1, group_count1);
            }
        }else if(item1[0] > item2[0]){
            getNextItem(item2, group_blk_ptr2, group_left_count2, group_count2);
        }else{
            if(item1[0] != record_now_x){   // 取出R表中所有的当前x值对应的所有y值, 并存到record块中
                recordRefresh();
                record_now_x = item1[0];
                while(item1[0] && item1[0] == item2[0]){
                    recordAttr(item1[1]);
                    getNextItem(next_item1, group_blk_ptr1, group_left_count1, group_count1);
                    if(next_item1[0] == item1[0]){
                        item1[0] = next_item1[0];
                        item1[1] = next_item1[1];
                    }else{
                        break;
                    }
                }
            }
            outputJoin(item2[1]);
            getNextItem(item2, group_blk_ptr2, group_left_count2, group_count2);
        }
    }
    outputRefresh();
}

void lab4_4(){
    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
    printf("\n==========================================\n");
    printf("基于排序的连接操作算法\n");
    printf("==========================================\n\n");
    firstScan(1, 16, 17, 48);
    secondScan();
    printf("\n总共连接%d次\n", output_item_count);
    printf("\nIO读写一共%d次\n", buf.numIO);
    freeBuffer(&buf);
}