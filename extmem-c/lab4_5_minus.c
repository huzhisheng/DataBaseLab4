#include "common.h"
extern Buffer buf;

// lab4_5_minus.c һ��ɨ�贴�������������ʱ�����1501��ʼ�Ĵ��̿�
// lab4_5_minus.c ����ɨ��洢���ӽ������ڴ�701��ʼ�Ĵ��̿�
static int temp_block_no = 1501;
static int output_blk_no = 701;

// ����������8������������ź��������д��TEMP_BLK���̿�, ͬʱע����д�ÿ�����8�ֽ�Ϊ��һ��Ŀ��
static void sortItemInBuffer(){
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
static void firstScan(int R_beg_no, int R_end_no, int S_beg_no, int S_end_no){
    
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


// �������������ģ��
static char* output_blk_ptr = NULL;
static int output_left_free = 0;
static int output_item_count = 0;   // ͳ�Ƽ���������Ԫ������
static int last_output[2] = {0, 0}; // ���Ҫȥ��, ÿ�����һ��Ԫ��ʱҪ���ϴεĽ��бȽ��Ƿ��ظ�
static void outputItem(int* item, int attr_count){
    if((last_output[0] == item[0]) && (last_output[1] == item[1])){
        return;
    }
    output_item_count++;
    last_output[0] = item[0];
    last_output[1] = item[1];
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


// beg_res_blk_no�����Ž���Ŀ�ʼ���̿��
static void secondScan(){
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
    int item1[2];
    int item2[2];
    getNextItem(item1, group_blk_ptr1, group_left_count1, group_count1);  // R��ĵ�ǰѡ����Ԫ��
    getNextItem(item2, group_blk_ptr2, group_left_count2, group_count2);  // S��ĵ�ǰѡ����Ԫ��
    while(item1[0] && item2[0]){   // ���Ӳ��� -- ��R���S���κ�һ����ɨ����ʱ����ѭ��
        // ��ΪS����������R��, ��S��Ϊ��
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
            }else{  // item1��item2��ȫ��ͬ
                getNextItem(item1, group_blk_ptr1, group_left_count1, group_count1);
                getNextItem(item2, group_blk_ptr2, group_left_count2, group_count2);
            }
        }
    }

    while(item2[0]){    // ���S����Ԫ��ûȡ��
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
    printf("R��S�Ĳ(S-R)��%d��Ԫ��\n", output_item_count);
    printf("IO��дһ��%d��\n", buf.numIO);
    freeBuffer(&buf);
}