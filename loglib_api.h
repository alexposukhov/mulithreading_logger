#ifndef __LIBLOG_H
#define __LIBLOG_H

#define MAX_LOGFILE_SIZE 128*1024
#define MAX_TEXTBUF_SIZE 4096
#define MAX_MSG_LEN      1024      

typedef struct __QUEUE
{
    int front, rear, size;
    unsigned int capacity;
    char** array;
} MSG_QUEUE_t;

int init_log_msg_queue(int cap);
void deinit_msg_queue(void);
int log_msg(char* msg);
#endif //__LIBLOG_H