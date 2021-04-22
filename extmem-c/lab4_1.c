#include "common.h"

extern Buffer buf;
static int output_blk_no = 101;

// �������������ģ��, ��Ϊ��ͬ��Ŀ���������Ҫ��ͬ, ���û�г����ͨ�ú���
static char* output_blk_ptr = NULL;
static int output_left_free = 0;
static int output_item_count = 0;   // ͳ�Ƽ���������Ԫ������

static void outputItem(int* item, int attr_count){
    output_item_count++;

    if(output_blk_ptr == NULL){
        output_blk_ptr = getNewBlockInBuffer(&buf);
        output_left_free = 14;  // ��8�ֽڿռ�����һ����Ŀ��
    }
    int* output_int_ptr = (int*)output_blk_ptr;
    for(int i=0; i<attr_count; i++){
        output_int_ptr[14 - output_left_free] = item[i];
        output_left_free -= 1;
        if(output_left_free == 0){
            printf("ע: ���д�����: %d\n", output_blk_no);
            writeNextBlockNum(output_blk_ptr, output_blk_no+1);
            reverseTranslateBlock(output_blk_ptr);
            writeBlockToDisk(output_blk_ptr, output_blk_no++, &buf);
            output_blk_ptr = getNewBlockInBuffer(&buf);
            memset(output_blk_ptr, 0, 64);
            output_int_ptr = (int*)output_blk_ptr;
            output_left_free = 14;  // ��8�ֽڿռ�����һ����Ŀ��
        }
    }
}

static void outputRefresh(){
    if(output_left_free < 14){
        printf("ע: ���д�����: %d\n", output_blk_no);
        writeNextBlockNum(output_blk_ptr, output_blk_no+1);
        reverseTranslateBlock(output_blk_ptr);
        writeBlockToDisk(output_blk_ptr, output_blk_no++, &buf);
        output_blk_ptr = getNewBlockInBuffer(&buf);
        memset(output_blk_ptr, 0, 64);
        output_left_free = 14;  // ��8�ֽڿռ�����һ����Ŀ��
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
    // ����һ��static����, ��Ϊlab4_1��lab4_3������ô˺���, ���������һЩ�����ͻ����
    output_blk_no = 101;
    output_blk_ptr = NULL;
    output_left_free = 0;
    output_item_count = 0;   // ͳ�Ƽ���������Ԫ������

    for(unsigned int i=beg_blk_no; i<=end_blk_no; i++){
        char* block_buf = readBlockFromDisk(i, &buf);
        printf("�������ݿ�%d\n", i);
        searchBlock(block_buf, target);
        freeBlockInBuffer(block_buf, &buf);
    }

    outputRefresh();

    printf("\n����ѡ��������Ԫ��һ��%d��\n", output_item_count);
    printf("\nIO��дһ��%d��\n", buf.numIO);
}

// �������������Ĺ�ϵѡ���㷨
void lab4_1(){

    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    printf("==========================================\n");
    printf("��������������ѡ���㷨 S.C=50\n");
    printf("==========================================\n\n");
    searchLinear(TABLE_R_MAX_BLOCK_NO+1, TABLE_S_MAX_BLOCK_NO, 50);
    freeBuffer(&buf);
}
