#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include "loglib_api.h"
#include "zlib.h"

// --- local defines ---
#define RINGBUFSIZE 20
#define RINGBUFSTART 1
#define LOG_FILE_NAME "my_logfile.txt"
// --- static types ---
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static MSG_QUEUE_t* queue = NULL;

// --- static functions ---
static void do_archivation(void* data, size_t len);
static void do_rearchivation(void);

MSG_QUEUE_t* create_queue(unsigned int capacity) 
{
    MSG_QUEUE_t* queue = (MSG_QUEUE_t*) malloc(sizeof(struct __QUEUE));
    if (queue == NULL)
    {
        printf("Error memory allocation\n");
    }
    queue->capacity = capacity;
    queue->size = 0;
    queue->front = 0;  
    queue->rear = capacity - 1;
    queue->array = (char**) malloc(queue->capacity * sizeof(char *));
    if (queue->array == NULL)
    {
        printf("Error memory allocation\n");
        free(queue);
        return NULL;
    }
    return queue; 
}

void destroy_queue(MSG_QUEUE_t* queue)
{
    if (queue == NULL)
    {
        return;
    }
    if (queue->array != NULL)
    {
        free(queue->array);
        queue->array = NULL;
    }
    free(queue);
    queue = NULL;
}

int is_full(MSG_QUEUE_t* queue)
{
    int ret = -1;
    if (queue == NULL)
    {
        printf("queue does not exist\n");
    }
    else
    {
        pthread_mutex_lock(&mutex);
        if (queue->size == queue->capacity)
        {
            ret = 1;
        }
        else
        {
            ret = 0;
        }
        pthread_mutex_unlock(&mutex);
    }
    return ret;
}

int is_empty(MSG_QUEUE_t* queue)
{
    int ret = -1;
    if (queue == NULL)
    {
       printf("queue does not exist\n");
    }
    else
    {
        pthread_mutex_lock(&mutex);
        if (queue->size == 0)
        {
            ret = 1;
        }
        else
        {
            ret = 0;
        }
        pthread_mutex_unlock(&mutex);
    }
    return ret;
}
  
void add_to_queue(MSG_QUEUE_t* queue, char* msg)
{
    if (is_full(queue))
    {
        fprintf(stderr, "Msg Queue is full\n");
        return;
    }
    pthread_mutex_lock(&mutex);
    queue->rear = (queue->rear + 1)%queue->capacity;
    queue->array[queue->rear] = msg;
    queue->size = queue->size + 1;
    pthread_mutex_unlock(&mutex);

    printf("%s added to queue: rear <%d>, front <%d>\n", queue->array[queue->rear], queue->rear, queue->front);
}

int log_msg(char* msg)
{
    char* pmem = NULL;
    size_t len = strnlen(msg, MAX_MSG_LEN);
    if (msg == NULL)
    {
        printf("Error: empty message \n");
        return -1;
    }
    pmem = (char*) calloc (1, len + 1);
    if (pmem == NULL)
    {
        printf("Error: log_log calloc error\n");
        return -2;
    }
    memset(pmem, 0, len + 1);
    sprintf(pmem, "%s", msg);
    add_to_queue(queue, pmem);
    return 0 ;
}

char* get_from_queue(MSG_QUEUE_t* queue)
{
    if (is_empty(queue))
    {
        fprintf(stderr, "Msg Queue is EMPTY\n");
        return NULL;
    }
    pthread_mutex_lock(&mutex);
    char* txt = queue->array[queue->front];
    queue->front = (queue->front + 1)%queue->capacity;
    queue->size = queue->size - 1;
    pthread_mutex_unlock(&mutex);
    return txt;
}

char* front(MSG_QUEUE_t* queue)
{
    if (is_empty(queue))
    {
        return NULL;
    }
    return queue->array[queue->front];
}

char* rear(MSG_QUEUE_t* queue)
{
    if (is_empty(queue))
        return NULL;
    return queue->array[queue->rear];
}

size_t get_log_file_size(const char *file_name)
{
  struct stat st;

  if(stat(file_name,&st)==0)
    return (st.st_size);
  else
    return -1;
}

void* log_write(void* ptr)
{
    char* str = NULL;
    char* mem = NULL;
    char* mtail = NULL;
    char tmp [128] = "";
    FILE* pFile = NULL;
    int len = 0;
    size_t written = 0;
    int written_times = 0;
    long int fsize = 0;
    char* pbuf = NULL;
    size_t bytes_read = 0;
    ssize_t size = 0;

    printf("Writer thread enter\n"); 
    pFile = fopen(LOG_FILE_NAME, "w+");
    if (pFile == NULL)
    {
        printf("Error opening logfile\n");
        return NULL;
    }

    mem = (char*) calloc (1, 4 * MAX_TEXTBUF_SIZE);
    if (mem == NULL)
    {
        printf("Error memory allocation\n");
        return NULL;
    }
    mtail = mem;
    printf("Enter to while loop\n");
    while(1)
    {
        if (0 == is_empty(queue))
        {
            memset(tmp, 0, 128);
            str = get_from_queue(queue);
            sprintf(tmp, "%s\n", str);
            len = strnlen(tmp, 128);
            if ((mtail + len - mem) > MAX_TEXTBUF_SIZE)
            {
                written = fwrite (mem , sizeof(char), (mtail - mem), pFile);
                if (written > 0)
                {
                    free(mem);
                    mem = (char*) calloc (1, MAX_TEXTBUF_SIZE);
                    if (mem == NULL)
                    {
                        printf("Error memory allocation\n");
                        fclose(pFile);
                        return NULL;
                    }
                }
                written_times++;
                mtail = mem;
            }
            memcpy(mtail, tmp, len);
            mtail += len;
            free(str);
        }
        else
        {
            if ((mtail - mem) > 0)
            {
                fwrite (mem , sizeof(char), (mtail - mem), pFile);
                mtail = mem;
            }
        }
        
        size = get_log_file_size(LOG_FILE_NAME);
        if (size > MAX_LOGFILE_SIZE)
        {
            fwrite (mem , sizeof(char), (mtail - mem), pFile);
            fclose (pFile);
            pFile = fopen(LOG_FILE_NAME, "rb");
            if (pFile == NULL)
            {
                printf("Error opening logfile\n");
                return NULL;
            }

            fseek(pFile, 0, SEEK_END);
            fsize = ftell(pFile);
            fseek(pFile, 0, SEEK_SET);
            pbuf = (char*) malloc(fsize + 1);
            if (pbuf == NULL)
            {
                fclose (pFile);
                printf("%s: Error: Mem allocation\n", __func__);
            }
            else
            {
                bytes_read = fread(pbuf, 1, (size_t)fsize, pFile);
                while (((size_t)fsize - bytes_read) > 0)
                {
                    bytes_read += fread(pbuf + bytes_read, 1, fsize - bytes_read, pFile);
                }
            }
            fclose (pFile);
            do_archivation(pbuf, fsize);
            free(pbuf);
            pbuf = NULL;
            pFile = fopen(LOG_FILE_NAME, "w+");
            if (pFile == NULL)
            {
                printf("Error opening logfile\n");
                free(mem);
                mem = NULL;
                return NULL;
            }
        }
    }
    printf("Buf has been written %d times\n", written_times);
    free(mem);
    mem = NULL;

    return NULL;
}

int init_log_msg_queue(int cap)
{
    int ret = 0;
    pthread_t writer;

    queue = create_queue(cap);
    if (queue == NULL)
    {
        printf("Error: Cannot create log message queue for %d elements\n", cap);
        ret = -1;
    }
    else
    {
        printf("log message queue successfully created\n");
        pthread_create (&writer, NULL, &log_write, NULL);
    //    pthread_join(writer, NULL);
        pthread_detach(writer);
        printf("log thread started\n");
    }
    
    return ret;
}

void deinit_msg_queue(void)
{
    destroy_queue(queue);
}

static void do_archivation(void* data, size_t len)
{
    char filename[20] = "";
    gzFile fi = 0;
    
    if (data == NULL)
    {
        printf("wrong data supposed to be archived\n");
        return;
    }
    memset(filename, 0, 20);
    snprintf(filename, 20, "logfile1.txt.gz");
    
    do_rearchivation();

    fi = (gzFile )gzopen(filename,"wb");
    gzwrite(fi,(void const *) data, len);
    gzclose(fi);
}

static void do_rearchivation(void)
{
  int res = 0;
  char PrevFileName[20] = {'\0'};
  char NextFileName [20] = {'\0'};
  int file_number = 0;

  sprintf( NextFileName, "logfile%d.txt.gz", RINGBUFSIZE );
  if( access( NextFileName, F_OK ) != -1 )
  {
    // file exists
    if( remove( NextFileName ) != 0 )
    {
      perror( "Error deleting file" );
    }
    else
    {
      puts( "File successfully deleted" );
    }
  }
  /*Pushing fifo elements*/
  for (file_number=RINGBUFSIZE; file_number>1; file_number--)
  {
    sprintf( NextFileName, "logfile%d.txt.gz", file_number );
    sprintf( PrevFileName, "logfile%d.txt.gz", file_number-1 );
    if( access( PrevFileName, F_OK ) != -1 )
    {
      // file exists
      res = rename( PrevFileName , NextFileName );
      if ( res == 0 )
      {
        printf ( "File successfully renamed\n" );
      }
      else
      {
        printf( "Error renaming file" );
      }
    }
  }

}

