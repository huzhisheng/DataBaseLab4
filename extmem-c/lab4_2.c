#include "common.h"
extern Buffer buf;

#define TEMP_BLK_BEGIN 1001
static int temp_block_no = TEMP_BLK_BEGIN;

// ��������Ԫ�������
void swapItem(int* a, int* b){
    int temp1 = a[0];
    int temp2 = a[1];
    a[0] = b[0];
    a[1] = b[1];
    b[0] = temp1;
    b[1] = temp2;
}

// �õ��������еĵ�int_no��Ԫ���ָ��
int* getItemPtr(int int_no){
    char* byte_ptr = buf.data + 1;
    for(int i=0; i < int_no / BLOCK_ITEM_NUM; i++){
        byte_ptr += BLOCK_SIZE + 1;
    }
    byte_ptr += (int_no % BLOCK_ITEM_NUM) * ITEM_SIZE;
    return (int*)byte_ptr;
}

//��һ����������block�е����ݴ�ASCII���ʾ��Ϊ��ֵ����(��ռ�ö����ڴ�ռ�)
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

//��һ����������block�е����ݴ���ֵ��ʾ��ΪASCII��ʾ(��ռ�ö����ڴ�ռ�)
void reverseTranslateBlock(char* block_buf){
    char str[5];
    memset(str, 0, 5);
    int* int_block_buf = (int*)block_buf;
    for(int i=0; i<BLOCK_ITEM_NUM; i++){
        int item_attr1 = int_block_buf[2 * i];
        int item_attr2 = int_block_buf[2 * i + 1];
        itoa(item_attr1, str, 10);
        for(int i=0; i<ATTR_SIZE; i++){
            block_buf[i * ITEM_SIZE + i] = str[i];
        }
        memset(str, 0, 5);
        itoa(item_attr2, str, 10);
        for(int i=0; i<ATTR_SIZE; i++){
            block_buf[i * ITEM_SIZE + ATTR_SIZE + i] = str[i];
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

// ����������8������������ź��������д��TEMP_BLK���̿�, ͬʱע����д�ÿ�����8�ֽ�Ϊ��һ��Ŀ��
void sortItemInBuffer(){
    char* blk_ptr = buf.data + 1;
    int block_count = buf.numAllBlk-buf.numFreeBlk;
    for(int i=0; i < block_count; i++){
        translateBlock(blk_ptr);
        blk_ptr += BLOCK_SIZE + 1;
    }
    // �򵥵�ð������
    for(int i=0; i < block_count * BLOCK_ITEM_NUM - 1; i++){    // ��ѭ��Ϊ����������len��������len-1��
        for(int j=0; j < block_count * BLOCK_ITEM_NUM - 1 - i; j++){
            int* item1 = getItemPtr(j);
            int* item2 = getItemPtr(j+1);
            if(item1[1] > item2[1]){    // �������Yֵ����
                swapItem(item1, item2);
            }
        }
    }
    blk_ptr = buf.data + 1;
    // �ź���󣬽�������������д�����̵�temp����
    for(int i=0; i < block_count; i++){
        reverseTranslateBlock(blk_ptr);
        if(i < block_count-1)
            writeNextBlockNum(blk_ptr, temp_block_no + 1);
        else
            writeNextBlockNum(blk_ptr, 0);  // ÿ������һ���next_block_numΪ0
        writeBlockToDisk(blk_ptr, temp_block_no++, &buf);
        blk_ptr += BLOCK_SIZE + 1;
    }
}
/*
����˵��
beg_blk_no: Ҫ����Ĵ��̿����ʼ��ַ
end_blk_no: Ҫ����Ĵ��̿�Ľ�����ַ
beg_res_blk_no: �ź���Ĵ��̿����ʼ��ַ
*/
void sortItemInDisk(int beg_blk_no, int end_blk_no, int beg_res_blk_no){
    if (buf.numFreeBlk != BUF_BLOCK_NUM)
    {
        perror("Buffer is dirty\n");
        return;
    }
    int temp_block_beg_no = temp_block_no;  // ����˿̵�temp_block_no

    // ��һ��ɨ������, ��Ҫ����Ŀ鰴��8��һ���ȡ���ڴ��ٽ���ð���������д�ص����̵�Temp���̿�
    int total_block_count = end_blk_no - beg_blk_no + 1;
    int processed_block_count = 0;
    while(processed_block_count < total_block_count){
        while(buf.numFreeBlk){
            readBlockFromDisk(beg_blk_no + processed_block_count, &buf);
            processed_block_count += 1;
        }
        sortItemInBuffer();
    }

    // �ڶ���ɨ������
    int group_count = ceil((float)total_block_count / BUF_BLOCK_NUM);   //�����������
    processed_block_count = 0;  // ���Ѵ����Ŀ�������
    
    if (buf.numFreeBlk != BUF_BLOCK_NUM)
    {
        perror("Buffer is dirty\n");
        return;
    }

    // ��¼ÿ�黹ʣ����Ԫ��û����
    int* group_left_count = (int*)malloc(sizeof(int) * group_count);   
    memset((void*)group_left_count, 0, sizeof(int) * group_count);
    for(int i=0; i<group_count; i++){
        group_left_count[i] = BLOCK_ITEM_NUM;
    }

    // ��Ÿ���Ļ�������ַ, ���һ���Ѿ����������Ӧ��ָ��ΪNULL
    char** group_blk_ptr = (char**)malloc(sizeof(char*) * group_count); 
    for(int i=0; i<group_count; i++){
        group_blk_ptr[i] = readBlockFromDisk(temp_block_beg_no + i*BUF_BLOCK_NUM, &buf);
        translateBlock(group_blk_ptr[i]);
    }

    // ��ÿ��ĵ�һ���ȡ����������ʼ���еڶ���ɨ������, ���Ƶ��д��ӵ���˼��
    char* output_blk_ptr = getNewBlockInBuffer(&buf);   // Ϊ�������������һ���ڴ��
    int output_left_free = BLOCK_ITEM_NUM;              // ��ǰ�������������д�Ŀռ�
    while(processed_block_count < total_block_count){
        if(output_left_free == 0){  // �������������, ���뽫���������ˢ��
            if(processed_block_count < total_block_count - 1)
                writeNextBlockNum(output_blk_ptr, beg_res_blk_no+1);
            else{
                writeNextBlockNum(output_blk_ptr, 0);   // ���һ�����������, next_block_num=0
            }
            writeBlockToDisk(output_blk_ptr, beg_res_blk_no++, &buf);
            processed_block_count++;
            if(processed_block_count < total_block_count){
                output_blk_ptr = getNewBlockInBuffer(&buf);
                output_left_free = BLOCK_ITEM_NUM;
            }
        }
        // whileÿѭ��һ�������������һ��Ԫ��(Item)

        // �ҵ��ؼ�ֵ��СԪ��
        int min_value = 0xfffffff;
        int* min_item = NULL;
        int min_group = -1; //��СԪ���������
        for(int i=0; i<group_count; i++){
            if(group_blk_ptr[i] == NULL)
                continue;
            int* item_ptr = (int*)(group_blk_ptr[i]) + (BLOCK_ITEM_NUM - group_left_count[i])*2;
            // ���谴��(X, Y)�е�Y��������
            if(item_ptr[1] < min_value){
                min_value = item_ptr[1];
                min_item = item_ptr;
                min_group = i;
            }
        }

        // ����СԪ��д�����������, �Ȳ��ð���ASCII��д��ֱ�ӾͰ���ֵ��д
        memcpy(output_blk_ptr + (BLOCK_ITEM_NUM - output_left_free)*ITEM_SIZE, min_item, ITEM_SIZE);
        output_left_free -= 1;

        group_left_count[min_group] -= 1;
        if(group_left_count[min_group] == 0){   // �÷����һ���Ѵ�����, ���÷������һ��
            int next_block_num = getNextBlockNum(group_blk_ptr[min_group]);
            if(next_block_num != 0){
                freeBlockInBuffer(group_blk_ptr[min_group], &buf);
                group_blk_ptr[min_group] = readBlockFromDisk(next_block_num, &buf);
                group_left_count[min_group] = BLOCK_ITEM_NUM;
            }else{
                freeBlockInBuffer(group_blk_ptr[min_group], &buf);
                group_blk_ptr[min_group] = NULL;
            }
        }
    }
}

void lab4_2(){
    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    sortItemInDisk(1, 16, 301);
    sortItemInDisk(17, 48, 317);
}