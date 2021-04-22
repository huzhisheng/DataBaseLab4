#include "common.h"
extern Buffer buf;

// lab4_2.c 一遍扫描创建的有序单组会临时存放在1001开始的磁盘块
// lab4_2.c 二遍扫描的结果存放在301开始的磁盘块
static int temp_block_no = 1001;

// 交换两个元组的数据
void swapItem(int* a, int* b){
    int temp1 = a[0];
    int temp2 = a[1];
    a[0] = b[0];
    a[1] = b[1];
    b[0] = temp1;
    b[1] = temp2;
}

// 得到缓冲区中的第int_no个元组的指针
int* getItemPtr(int int_no){
    char* byte_ptr = buf.data + 1;
    for(int i=0; i < int_no / BLOCK_ITEM_NUM; i++){
        byte_ptr += BLOCK_SIZE + 1;
    }
    byte_ptr += (int_no % BLOCK_ITEM_NUM) * ITEM_SIZE;
    return (int*)byte_ptr;
}

//将一个缓冲区的block中的数据从ASCII码表示变为数值数据(不占用额外内存空间)
void translateBlock(char* block_buf){
    char str[5];
    str[5] = '\0';
    int* int_block_buf = (int*)block_buf;
    for(int i=0; i<BLOCK_ITEM_NUM; i++){
        for (int j = 0; j < ATTR_SIZE; j++)
        {
            str[j] = *(block_buf + i*ITEM_SIZE + j);
        }
        int item_attr1 = atoi(str);
        for (int j = 0; j < ATTR_SIZE; j++)
        {
            str[j] = *(block_buf + i*ITEM_SIZE + ATTR_SIZE + j);
        }
        int item_attr2 = atoi(str);

        int_block_buf[i*2] = item_attr1;
        int_block_buf[i*2+1] = item_attr2;
    }
}

//将一个缓冲区的block中的数据从数值表示变为ASCII表示(不占用额外内存空间)
void reverseTranslateBlock(char* block_buf){
    char str[5];
    
    int* int_block_buf = (int*)block_buf;
    for(int i=0; i<BLOCK_ITEM_NUM; i++){
        int item_attr1 = int_block_buf[2 * i];
        int item_attr2 = int_block_buf[2 * i + 1];
        memset(str, 0, 5);
        itoa(item_attr1, str, 10);
        for(int j=0; j<ATTR_SIZE; j++){
            block_buf[i * ITEM_SIZE + j] = str[j];
        }
        memset(str, 0, 5);
        itoa(item_attr2, str, 10);
        for(int j=0; j<ATTR_SIZE; j++){
            block_buf[i * ITEM_SIZE + ATTR_SIZE + j] = str[j];
        }
    }
}


void writeNextBlockNum(char* block_buf, int block_no){
    char str[8];
    memset(str,0,8);
    itoa(block_no, str, 10);
    memcpy(block_buf + BLOCK_ITEM_NUM * ITEM_SIZE, str, 8);
}

int getNextBlockNum(char* block_buf){
    char str[8];
    memset(str,0,8);
    for(int i=0; i<8; i++){
        str[i] = block_buf[BLOCK_ITEM_NUM * ITEM_SIZE + i];
    }
    return atoi(str);
}

// 排序函数, 将两个元组进行比较, 判断item1是否应该在item2前面, 是就返回1否则返回0
int compareItem(int* item1, int* item2){
    if(item1[0] < item2[0]){
        return 1;
    }else if(item1[0] == item2[0] && item1[1] <= item2[1]){
        return 1;
    }else{
        return 0;
    }
}

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

// 从第一遍扫描划分出的不同分组中得到下一个元组的指针
void getNextItem(int* item, char** group_blk_ptr, int* group_left_count, int group_count){
    // 找到关键值最小元组
    int* min_item = NULL;
    int min_group = -1; //最小元组所在组号
    for(int i=0; i<group_count; i++){
        if(group_blk_ptr[i] == NULL){
            continue;
        }
        int* item_ptr = (int*)(group_blk_ptr[i]) + (BLOCK_ITEM_NUM - group_left_count[i])*2;
        // 假设按照(X, Y)中的Y属性排序
        if(min_item == NULL){
            min_item = item_ptr;
            min_group = i;
        }
        else if(compareItem(item_ptr, min_item)){
            min_item = item_ptr;
            min_group = i;
        }
    }

    // 必须在这里就把min_item取到item中, 否则下面可能会将最小item所在组换到下一block
    if(min_item){
        item[0] = min_item[0];
        item[1] = min_item[1];
    }else{
        item[0] = 0;
        item[1] = 0;
    }
    

    group_left_count[min_group] -= 1;
    if(group_left_count[min_group] == 0){   // 该分组的一块已处理完, 换该分组的下一块
        int next_block_num = getNextBlockNum(group_blk_ptr[min_group]);
        if(next_block_num != 0){
            freeBlockInBuffer(group_blk_ptr[min_group], &buf);
            group_blk_ptr[min_group] = readBlockFromDisk(next_block_num, &buf);
            translateBlock(group_blk_ptr[min_group]);
            group_left_count[min_group] = BLOCK_ITEM_NUM;
        }else{
            freeBlockInBuffer(group_blk_ptr[min_group], &buf);
            group_blk_ptr[min_group] = NULL;
        }
    }
    return;
}

/*
参数说明
beg_blk_no: 要排序的磁盘块的起始地址
end_blk_no: 要排序的磁盘块的结束地址
beg_res_blk_no: 排好序的磁盘块的起始地址
*/
void sortItemInDisk(int beg_blk_no, int end_blk_no, int beg_res_blk_no){
    if (buf.numFreeBlk != BUF_BLOCK_NUM)
    {
        perror("Buffer is dirty\n");
        return;
    }
    int temp_block_beg_no = temp_block_no;  // 保存此刻的temp_block_no

    // 第一遍扫描排序, 对要排序的块按照8块一组读取到内存再进行冒泡排序后，再写回到磁盘的Temp磁盘块
    int total_block_count = end_blk_no - beg_blk_no + 1;
    int processed_block_count = 0;
    while(processed_block_count < total_block_count){
        while(buf.numFreeBlk){
            readBlockFromDisk(beg_blk_no + processed_block_count, &buf);
            processed_block_count += 1;
        }
        sortItemInBuffer();
    }

    // 第二遍扫描排序
    int group_count = ceil((float)total_block_count / BUF_BLOCK_NUM);   //计算出分组数
    processed_block_count = 0;  // 将已处理的块数重置
    
    if (buf.numFreeBlk != BUF_BLOCK_NUM)
    {
        perror("Buffer is dirty\n");
        return;
    }

    // 记录每组还剩多少元组没处理 -- 该数组长度为4, 未超过10
    int* group_left_count = (int*)malloc(sizeof(int) * group_count);   
    memset((void*)group_left_count, 0, sizeof(int) * group_count);
    for(int i=0; i<group_count; i++){
        group_left_count[i] = BLOCK_ITEM_NUM;
    }

    // 存放各组的缓冲区地址, 如果一组已经处理完则对应的指针为NULL -- 该数组长度为4, 未超过10
    char** group_blk_ptr = (char**)malloc(sizeof(char*) * group_count); 
    for(int i=0; i<group_count; i++){
        group_blk_ptr[i] = readBlockFromDisk(temp_block_beg_no + i*BUF_BLOCK_NUM, &buf);
        translateBlock(group_blk_ptr[i]);
    }

    // 将每组的第一块读取到缓冲区后开始进行第二遍扫描排序, 类似弹夹打子弹的思想
    char* output_blk_ptr = getNewBlockInBuffer(&buf);   // 为输出缓冲区申请一个内存块
    int output_left_free = BLOCK_ITEM_NUM;              // 当前输出缓冲区还可写的空间
    int min_item[2];
    while(processed_block_count < total_block_count){
        if(output_left_free == 0){  // 输出缓冲区已满, 必须将输出缓冲区刷新
            reverseTranslateBlock(output_blk_ptr);
            if(processed_block_count < total_block_count - 1)
                writeNextBlockNum(output_blk_ptr, beg_res_blk_no+1);
            else{
                writeNextBlockNum(output_blk_ptr, 0);   // 最后一个输出缓冲区, next_block_num=0
            }
            printf("注: 结果写入磁盘: %d\n", beg_res_blk_no);
            writeBlockToDisk(output_blk_ptr, beg_res_blk_no++, &buf);
            processed_block_count++;
            if(processed_block_count < total_block_count){
                output_blk_ptr = getNewBlockInBuffer(&buf);
                output_left_free = BLOCK_ITEM_NUM;
            }else{
                break;
            }
        }
        // while每循环一次输出缓冲区多一个元组(Item)
        // 找到关键值最小元组
        getNextItem(min_item, group_blk_ptr, group_left_count, group_count);
        
        // 将最小元组写到输出缓冲区, 先不用按照ASCII码写，直接就按照值来写
        memcpy(output_blk_ptr + (BLOCK_ITEM_NUM - output_left_free)*ITEM_SIZE, min_item, ITEM_SIZE);
        output_left_free -= 1;
        
    }
    
}

void lab4_2(){
    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
    printf("关系R排序后输出到文件 301.blk 到 316.blk\n");
    sortItemInDisk(1, 16, 301);
    printf("关系S排序后输出到文件 317.blk 到 348.blk\n");
    sortItemInDisk(17, 48, 317);
    printf("IO读写一共%d次\n", buf.numIO);
    freeBuffer(&buf);
}