/* memory managment, T13.816-T14.330 $DVS:time$ */

#include <stdlib.h>
#include <stdint.h>
#include "memory.h"
#include "utils/log.h"

#if defined(_WIN32) || defined(_WIN64)

void xdag_mem_tempfile_path(const char *tempfile_path)
{
}

int xdag_mem_init(size_t size)
{
	return 0;
}

void *xdag_malloc(size_t size)
{
	return malloc(size);
}

void xdag_free(void *mem)
{
	free(mem);
}

void xdag_mem_finish(void)
{
}

int xdag_free_all(void)
{
	return -1;
}

#else

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <errno.h>
#include <limits.h>

#define MEM_PORTION     	((size_t)1 << 28)
#define TMPFILE_TEMPLATE 	"xdag-tmp-XXXXXX"
#define TMPFILE_TEMPLATE_LEN	15
#define CACHE_SIZE		1073741824 // Byte representation of 10GB
#define NEW_FILE		1073741824

static int g_fd = -1;
static size_t g_pos = 0, g_fsize = 0, g_size = 0;
static void *g_mem;
static pthread_mutex_t g_mem_mutex = PTHREAD_MUTEX_INITIALIZER;
static char g_tmpfile_path[PATH_MAX] = "";
static char g_tmpfile[PATH_MAX];

void xdag_mem_tempfile_path(const char *tempfile_path)
{
	strcpy(g_tmpfile_path, tempfile_path);
}

int xdag_mem_init(size_t size)
{
	if (!size) {
		return 0;
	}
	if(strcmp(g_tmpfile_path,"RAM") == 0) {
		/* This will leave g_fd as -1, and malloc will then be called in xdag_malloc instead of pointer into mmap */
		return 0;
	}

	size--;
	size |= MEM_PORTION - 1;
	size++;

	size_t wrote = snprintf(g_tmpfile, PATH_MAX,"%s%s", g_tmpfile_path, TMPFILE_TEMPLATE);
	if (wrote >= PATH_MAX){
		xdag_fatal("Error: Temporary file path exceed the max length that is %d characters", PATH_MAX);
		return -1;
	}
	g_fd = mkstemp(g_tmpfile);
	if (g_fd < 0) {
		xdag_fatal("Unable to create temporary file %s errno:%d", g_tmpfile, errno);
		return -1;
	}
        printf("Temporary file created: %s\n", g_tmpfile);

	g_mem = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, g_fd, 0);
	if (g_mem == MAP_FAILED) {
		close(g_fd); return -1;
	}

	g_size = size;
	
	return 0;
}


int xdag_mem_reinit(size_t size)
{
/*
        if (!size) {
                return 0;
        }
        if(strcmp(g_tmpfile_path,"RAM") == 0) {
                // This will leave g_fd as -1, and malloc will then be called in xdag_malloc instead of pointer into mmap
                return 0;
        }
*/
//printf("CIAO1");
//fflush(stdout);
//size=2*NEW_FILE;
/*
        size--;
        size |= MEM_PORTION - 1;
        size++;
*/
//printf("CIAO2");
//fflush(stdout);

/*
        size_t wrote = snprintf(g_tmpfile, PATH_MAX,"%s%s", g_tmpfile_path, TMPFILE_TEMPLATE);
        if (wrote >= PATH_MAX){
                xdag_fatal("Error: Temporary file path exceed the max length that is %d characters", PATH_MAX);
                return -1;
        }
*/

//char *tmpf = "tmp-XXXXXX";
char tmpf[50];
tmpf[0]='t';
tmpf[1]='t';
tmpf[2]='t';
tmpf[3]='t';
tmpf[4]='t';
tmpf[5]='t';
tmpf[6]='t';
tmpf[7]='t';
tmpf[8]='t';
tmpf[9]='t';
tmpf[10]='t';
tmpf[11]='-';
tmpf[12]='X';
tmpf[13]='X';
tmpf[14]='X';
tmpf[15]='X';
tmpf[16]='X';
tmpf[17]='X';
tmpf[18]='X';
tmpf[19]='X';
tmpf[20]='X';
tmpf[21]='X';
tmpf[22]='\0';




        g_fd = mkstemp(tmpf);


        if (g_fd < 0) {
		printf("g_fd NEGATIVE");
		fflush(stdout);
                //xdag_fatal("Unable to create temporary file %s errno:%d", tmpf, errno);
                return -1;
        }
        //printf("Temporary file created: %s\n", tmpf);
	fflush(stdout);

        g_mem = mmap(0, g_size, PROT_READ | PROT_WRITE, MAP_SHARED, g_fd, 0);
        if (g_mem == MAP_FAILED) {
		printf("MAP FAILED");
		fflush(stdout);
                close(g_fd); return -1;
        }
	printf("MAP SUCCESS");
	fflush(stdout);
	
	
  //      g_size = size;
	
	g_fsize = 0;
	g_pos = 0;
        return 0;

	
}



void *xdag_malloc(size_t size)
{
	uint8_t *res;

	if (g_fd < 0) {
		return malloc(size);
	}

	if (!size) {
		return 0;
	}

	pthread_mutex_lock(&g_mem_mutex);
	
	if(g_fsize>NEW_FILE){
	xdag_mem_reinit(size);
}



	size--;
	size |= 0xf;
	size++;

	while (g_pos + size > g_fsize) {
	printf("ENTRATO");
	fflush(stdout);
		if (g_fsize + MEM_PORTION > g_size) {
			pthread_mutex_unlock(&g_mem_mutex);
			return 0;
		}
		g_fsize += MEM_PORTION;
		ftruncate(g_fd, g_fsize);


/*

        mlock(g_mem + g_fsize - MEM_PORTION, MEM_PORTION);





                if(g_fsize >= CACHE_SIZE)
        munlock(g_mem + g_fsize - CACHE_SIZE, MEM_PORTION);

*/


	}






/*

	mlock(g_mem + g_fsize - MEM_PORTION, MEM_PORTION);





		if(g_fsize >= CACHE_SIZE)
	munlock(g_mem + g_fsize - CACHE_SIZE, MEM_PORTION);

*/


	res = (uint8_t*)g_mem + g_pos;
	g_pos += size;









	
	pthread_mutex_unlock(&g_mem_mutex);
	
	return res;
}

void xdag_free(void *mem)
{
}

void xdag_mem_finish(void)
{
	if (g_fd < 0) return;
	
	pthread_mutex_lock(&g_mem_mutex);
	
	munmap(g_mem, g_size);
	ftruncate(g_fd, 0);
	close(g_fd);
	remove(g_tmpfile);
}

int xdag_free_all(void)
{
	g_pos = 0;
	return 0;
}

#endif

char** xdagCreateStringArray(int count, int stringLen)
{
	char** stringArray = malloc(count * sizeof(char*));
	for (int i = 0; i < count; ++i) {
		stringArray[i] = malloc(stringLen);
	}
	return  stringArray;
}

void xdagFreeStringArray(char** stringArray, int count)
{
	for (int i = 0; i < count; ++i) {
		free(stringArray[i]);
	}
	free(stringArray);
}
