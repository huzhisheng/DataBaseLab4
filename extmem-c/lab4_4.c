#include "common.h"
extern Buffer buf;

// lab4_4.c һ��ɨ�贴�������������ʱ�����1201��ʼ�Ĵ��̿�
// lab4_4.c ����ɨ��洢���ӽ������ڴ�401��ʼ�Ĵ��̿�
static int temp_block_no = 1201;
static int output_blk_no = 401;

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


// 48��/8��6��, ÿ��ռһ�������, ����һ����Ϊ��������, ��ʣһ�������Ϊrecord��

// ��־��, ���ڽ�����Ӳ���ʱR���г���ĳһ�ؼ�����ֵ��Ԫ����������(��Խ�˵�����)�������ظ���ȡ���ݿ������

// ��־������ñȽ����Խ���, ���¾��Ǽ���: 
// ����X����30������(30, y)���������г��ִ������ܶ�, ��ʱ���Ҫ�������Ӳ���, ��ô�ͱ�����ĳһ����S�ļ�¼
// (30, y1)��Ϊ��׼��ͬʱ�ظ�ɨ����һ����R�еľ��иùؼ�ֵ�ļ�¼(30, y)���, ÿɨ����һ���ٴӱ�S�л�ȡ��һ��
// ��¼, ����ü�¼��Ȼ��(30, y2)����ʽ, ��ô�ͼ���ɨ���R�е�����(30, y)������, �������һ������(30, y)��Ԫ
// ������Ҳ�ܶ�, �ֲ��ڶ������, ��ô�ͺ����ظ���ȥɨ��, ������ǿ��԰���һ��������ЩԪ���y���ݼ�¼��record��
// �������У���˵��ڶ�����Ҫɨ����һ���������е�(30, y)Ԫ��ʱ�Ϳ���ֱ����record���в�����Щyֵ���ɡ�record��
// ��������16��, ��˼�����ɴ洢16��(30, y)Ԫ���yֵ, ����������ڱ���ʵ�����Ѿ������ˡ�
static int record_left_free = 16;
static char* record_blk_ptr = NULL;
static int record_now_x = -1;   // ��¼record�������ڼ�¼��yֵ��Ӧ��Ԫ��(x, y)��xֵ
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


// �������������ģ��
static char* output_blk_ptr = NULL;
static int output_left_free = 0;
static int output_item_count = 0;   // ͳ���ܹ����ӽ��
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

static void outputJoin(int attr_y){
    // ��ν���Ӳ�������(x1, y1)��(x1, y2)��װ��(x1, y1, x1, y2)����
    // �����е�y1���Ѿ���¼��record����
    int item[4];
    item[0] = record_now_x;
    item[1] = attr_y;   // ����ʵ��PPT, ����ʱS��ķ���R��ǰ��
    item[2] = record_now_x;
    for(int i=0; i<16-record_left_free; i++){
        item[3] = recordGetAttr(i);
        outputItem(item, 4);
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
    int next_item1[2] = {0, 0};
    getNextItem(item1, group_blk_ptr1, group_left_count1, group_count1);  // R��ĵ�ǰѡ����Ԫ��
    getNextItem(item2, group_blk_ptr2, group_left_count2, group_count2);  // S��ĵ�ǰѡ����Ԫ��
    while(item1[0] && item2[0]){   // ���Ӳ��� -- ��R���S���κ�һ����ɨ����ʱ����ѭ��
        // ��ΪS����������R��, ��S��Ϊ��
        // ����һ��S���еļ�¼(a, b)��ȥR���в�������(a, y)���ݼ�¼��y���浽record����, �����Ӻ�������������, ��ȡS���е���һ����¼
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
            if(item1[0] != record_now_x){   // ȡ��R�������еĵ�ǰxֵ��Ӧ������yֵ, ���浽record����
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
    printf("������������Ӳ����㷨\n");
    printf("==========================================\n\n");
    firstScan(1, 16, 17, 48);
    secondScan();
    printf("\n�ܹ�����%d��\n", output_item_count);
    printf("\nIO��дһ��%d��\n", buf.numIO);
    freeBuffer(&buf);
}