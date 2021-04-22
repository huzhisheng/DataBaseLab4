#include "common.h"
extern Buffer buf;

#define TEMP_BLK_BEGIN 1201
static int temp_block_no = TEMP_BLK_BEGIN;

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
            
            if(!compareItem(item1, item2)){    // �������Yֵ����
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


static int R_sorted_beg_no;
static int S_sorted_beg_no;
static int R_sorted_end_no;
static int S_sorted_end_no;
static int total_block_count1;
static int total_block_count2;
/*
����˵��
R_beg_no: R����ʼ���̿�
R_end_no: R��������̿�
S_beg_no: S����ʼ���̿�
S_end_no: S��������̿�
*/
void firstScan(int R_beg_no, int R_end_no, int S_beg_no, int S_end_no){
    
    if (buf.numFreeBlk != BUF_BLOCK_NUM)
    {
        perror("Buffer is dirty\n");
        return;
    }

    // ��һ��ɨ������, ��Ҫ����Ŀ鰴��8��һ���ȡ���ڴ��ٽ���ð���������д�ص����̵�Temp���̿�
    // ��һ��ɨ������, ��R��Ľ���8��һ������
    R_sorted_beg_no = temp_block_no;    // ����8��һ��������R����ʼ���̿�
    R_sorted_end_no = -1;               // ����8��һ��������R��������̿�

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
    // ��һ��ɨ������, ��S��Ľ���8��һ������
    S_sorted_beg_no = temp_block_no;    // ����8��һ��������S����ʼ���̿�
    S_sorted_end_no = -1;               // ����8��һ��������S��������̿�

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

    // ��һ��ɨ�����ʱbufferӦ���ǿյ�
    if (buf.numFreeBlk != BUF_BLOCK_NUM)
    {
        perror("Buffer is dirty\n");
        return;
    }
}



// beg_res_blk_no�����Ž���Ŀ�ʼ���̿��
void secondScan(int beg_res_blk_no){
    int group_count1 = ceil((float)total_block_count1 / BUF_BLOCK_NUM);   //�����R��ķ����� -- 2
    int group_count2 = ceil((float)total_block_count2 / BUF_BLOCK_NUM);   //�����S��ķ����� -- 4
    int processed_block_count = 0;  // ���Ѵ���Ŀ�������
    
    // ��¼R��ÿ�黹ʣ����Ԫ��û���� -- �����鳤��Ϊ2, δ����10
    int* group_left_count1 = (int*)malloc(sizeof(int) * group_count1);   
    memset((void*)group_left_count1, 0, sizeof(int) * group_count1);
    for(int i=0; i<group_count1; i++){
        group_left_count1[i] = BLOCK_ITEM_NUM;
    }
    // ��¼S��ÿ�黹ʣ����Ԫ��û���� -- �����鳤��Ϊ4, δ����10
    int* group_left_count2 = (int*)malloc(sizeof(int) * group_count2);   
    memset((void*)group_left_count2, 0, sizeof(int) * group_count2);
    for(int i=0; i<group_count2; i++){
        group_left_count2[i] = BLOCK_ITEM_NUM;
    }

    // ���R�����Ļ�������ַ, ���һ���Ѿ����������Ӧ��ָ��ΪNULL -- �����鳤��Ϊ2, δ����10
    char** group_blk_ptr1 = (char**)malloc(sizeof(char*) * group_count1); 
    for(int i=0; i<group_count1; i++){
        group_blk_ptr1[i] = readBlockFromDisk(R_sorted_beg_no + i*BUF_BLOCK_NUM, &buf);
        translateBlock(group_blk_ptr1[i]);
    }
    // ���S�����Ļ�������ַ, ���һ���Ѿ����������Ӧ��ָ��ΪNULL -- �����鳤��Ϊ4, δ����10
    char** group_blk_ptr2 = (char**)malloc(sizeof(char*) * group_count2); 
    for(int i=0; i<group_count2; i++){
        group_blk_ptr2[i] = readBlockFromDisk(S_sorted_beg_no + i*BUF_BLOCK_NUM, &buf);
        translateBlock(group_blk_ptr2[i]);
    }

    // �ڶ���ɨ������ʼ
    char* output_blk_ptr = getNewBlockInBuffer(&buf);   // Ϊ�������������һ���ڴ�� -- һ����6��, ��˻�������ʣ2�������, �������
    int output_left_free = 3;               // ��ǰ�������������д�Ŀռ� 
                                            // һ���������ݳ�16�ֽڣ����ÿ����ֻ�ܷ�3�����Ӻ�ļ�¼!
    int* item1 = getNextItem(group_blk_ptr1, group_left_count1, group_count1);  // R��ĵ�ǰѡ����Ԫ��
    int* item2 = getNextItem(group_blk_ptr2, group_left_count2, group_count2);  // S��ĵ�ǰѡ����Ԫ��
    int* next_item1 = getNextItem(group_blk_ptr1, group_left_count1, group_count1); // R�����һ��Ԫ��
    int* next_item2 = getNextItem(group_blk_ptr2, group_left_count2, group_count2); // S�����һ��Ԫ��
    while(1){   // ���Ӳ��� -- ��R���S���κ�һ����ɨ����ʱ����ѭ��
        if(output_left_free == 0){  // �������������, ���뽫���������ˢ��
            
        }
        // whileÿѭ��һ�������������һ��Ԫ��(Item)
        if(item1[0] == item2[0]){   // ���R.A = S.C
            goto output;
        }

    output: // ���Ԫ�鵽���������, �Ȳ��ð���ASCII��д��ֱ�ӾͰ���ֵ��д
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