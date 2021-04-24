#include "common.h"
extern Buffer buf;

// lab4_3.c �����������ļ������1101��ʼ�Ĵ��̿�
static int beg_index_blk_no = 1101;    //�����ļ����̿���ʼ��
static int end_index_blk_no = 1101;

void createIndexFileForTable(int beg_blk_no, int end_blk_no, int output_blk_no){
    char* output_blk_buf = getNewBlockInBuffer(&buf);
    int* output_int_ptr = (int*)output_blk_buf;
    int output_left_free = BLOCK_ITEM_NUM;

    for(int blk_no = beg_blk_no; blk_no <= end_blk_no; blk_no++){
        char* blk_buf = readBlockFromDisk(blk_no, &buf);
        char str[5];
        str[5] = '\0';
        for (int j = 0; j < ATTR_SIZE; j++)
        {
            str[j] = *(blk_buf + j);
        }
        int item_attr = atoi(str);
        // �������ļ���Ͳ���ΪASCII��洢, ֱ�Ӷ����ƴ洢��ֵ
        output_int_ptr[0] = item_attr;
        output_int_ptr[1] = blk_no;
        output_int_ptr += 2;
        output_left_free -= 1;
        if(output_left_free == 0){
            writeBlockToDisk(output_blk_buf, output_blk_no++, &buf);
            output_blk_buf = getNewBlockInBuffer(&buf);
            output_int_ptr = (int*)output_blk_buf;
            output_left_free = BLOCK_ITEM_NUM;
        }
        freeBlockInBuffer(blk_buf, &buf);
    }
    if(output_left_free != BLOCK_ITEM_NUM){
        writeBlockToDisk(output_blk_buf, output_blk_no++, &buf);
    }else{
        freeBlockInBuffer(output_blk_buf, &buf);
    }
    end_index_blk_no = output_blk_no - 1;
}

void searchTableByIndex(int target, int beg_index_blk_no, int end_index_blk_no){
    int index_blk_no = beg_index_blk_no;
    int left_blk_no = -1;
    int right_blk_no = -1;

    while(1){
        printf("����������%d\n",index_blk_no);
        char* blk_ptr = readBlockFromDisk(index_blk_no, &buf);
        int* int_ptr = (int*)blk_ptr;
        for(int i=0; i<BLOCK_ITEM_NUM; i++){
            if(int_ptr[0] > target){
                goto find_ok;   // ����forѭ����whileѭ��
            }else if(int_ptr[0] == target) {
                right_blk_no = int_ptr[1];
            }else{  // int_ptr[0] < target
                left_blk_no = int_ptr[1];
                right_blk_no = int_ptr[1];
            }
            int_ptr += 2;
        }
        freeBlockInBuffer(blk_ptr, &buf);
        if(index_blk_no == end_index_blk_no){
            break;
        }else{
            index_blk_no++;
        }

    }
find_ok:
    searchLinear(left_blk_no, right_blk_no, 50, 301);
}

void lab4_3(){
    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    printf("\n==========================================\n");
    printf("���������Ĺ�ϵѡ���㷨 S.C=50\n");
    printf("==========================================\n\n");

    // �ȴ��������ļ�
    createIndexFileForTable(217, 248, beg_index_blk_no);
    // �����������ļ�����buffer
    freeBuffer(&buf);
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
    searchTableByIndex(50, beg_index_blk_no, end_index_blk_no);
    freeBuffer(&buf);
}
