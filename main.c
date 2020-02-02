#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <unistd.h>
#include "log.h"

void* generate_str1(void* a)
{
    static int num = 0;
    char msg [64] = "";
    int i = 0;
    for (i = 0; i < 1000; i++)
    {
        memset(msg, 64, 0);
        sprintf(msg, "this is from thread 1: msg %d", num++);
        log_debug(msg);
    }
    return NULL;
}

void* generate_str2(void* a)
{
    static int num = 0;
    char msg [64] = "";
    int i = 0;
    for (i = 0; i < 800; i++)
    {
        memset(msg, 64, 0);
        sprintf(msg, "this is from thread 2: msg %d", num++);
        log_info(msg);
    }
    return NULL;
}

void* generate_str3(void* a)
{
    static int num = 0;
    char msg [64] = "";
    int i = 0;
    for (i = 0; i < 600; i++)
    {
        memset(msg, 64, 0);
        sprintf(msg, "this is from thread 3: msg %d", num++);
        log_warn(msg);
    }
    return NULL;
}

void* generate_str4(void* a)
{
    static int num = 0;
    char msg [64] = "";
    int i = 0;
    for (i = 0; i < 400; i++)
    {
        memset(msg, 64, 0);
        sprintf(msg, "this is from thread 4: msg %d", num++);
        log_error(msg);
    }
    return NULL;
}

void* generate_str5(void* a)
{
    static int num = 0;
    char msg [64] = "";
    int i = 0;
    for (i = 0; i < 200; i++)
    {
        memset(msg, 64, 0);
        sprintf(msg, "this is from thread 5: msg %d", num++);
        log_fatal(msg);
    }
    return NULL;
}

int main()
{
    pthread_t thread1, thread2, thread3, thread4, thread5;
    int pid = 0 ;

    pid = fork();
    if(pid == 0)
    {
        log_init();
        pthread_create (&thread1, NULL, &generate_str1, NULL);
        pthread_create (&thread2, NULL, &generate_str2, NULL);
        pthread_create (&thread3, NULL, &generate_str3, NULL);
        pthread_create (&thread4, NULL, &generate_str4, NULL);
        pthread_create (&thread5, NULL, &generate_str5, NULL);
        
        pthread_join(thread1, NULL);
        pthread_join(thread2, NULL);
        pthread_join(thread3, NULL);
        pthread_join(thread4, NULL);
        pthread_join(thread5, NULL);
        sleep(2);
        log_deinit(); 
    }
    return 0;
}