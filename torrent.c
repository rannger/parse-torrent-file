#include "torrent.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <assert.h>

str_t str_alloc(uint32_t len)
{
	len += 1;
	str_t ret = (str_t)malloc(sizeof(char)*len);
	bzero(ret,len);
	return ret;
}

void str_release(str_t str)
{
	free(str);
}

list_t list_alloc(uint32_t len)
{
	list_t list = (list_t)malloc(sizeof(struct list_t));
	list->head = (list_node_t*)malloc(sizeof(list_node_t*)*len);
	list->size = 0;
	list->max = len;

	return list;
}

list_node_t node_alloc(void)
{
	list_node_t node = (list_node_t)malloc(sizeof(struct list_node_t));
	node->val = NULL;
	node->type = E_TYPE_MAX;	
	return node;
}

void node_release(list_node_t* node)
{
	free(node);
}

BOOL list_append(list_t list,list_node_t node)
{
	BOOL ret = NO;
	if (list->size<list->max) {
		list->head[list->size] = node;
		list->size ++;
		ret = YES;
	} else {
		ret = NO;
	}
	return ret;
}

BOOL list_del_last(list_t list)
{
	BOOL ret = NO;
	if (!list->size) {
		ret = NO;
	} else {
		list_node_t node = list->head[list->size];
		list->head[list->size] = NULL;
		free(node);
		list->size --;
		ret = YES;
	}
	return ret;
}

void list_release(list_t list)
{
	while(list_del_last(list));
	free(list->head);	
	free(list);
}

fi_t fi_alloc(uint32_t length)
{
	fi_t ret = (fi_t)malloc(sizeof(struct fi_t)+sizeof(str_t)*length);
	ret->length = length;
	for(int i=0;i<length;++i)
		ret->path[i] = NULL;
	return ret;
}

void fi_release(fi_t fi)
{
	for(int i=0;i<fi->length;++i)
		str_release(fi->path[i]);
	free(fi);
}

int32_t file_size(char* path)
{
	struct stat statbuf;
	if (stat(path, &statbuf) == -1) {
		  /* check the value of errno */
		  return -1;
	}
	return statbuf.st_size;
}

void read_torrent_file(char* path,char* buf,int32_t buf_len)
{
	int fd = open(path,O_RDONLY);
	if (fd<0) exit(-1);
	int s = 0;
	do {
		int res = read(fd,buf+s,buf_len-s);
		if (res>0) s+=res;
		if (res<0) {
			close(fd);
			exit(-1);
		} else if (res<buf_len) {
			struct pollfd pfd = {
				fd,
				POLLIN,
				0
			};
			if (poll(&pfd,1,0) > 0) {
				if (pfd.revents & POLLIN) continue;
			} else {
				break;
			}
		} else {
			break;
		}
	}while(1);

	close(fd);
}


str_t rdstr(const char* buf,const uint32_t len,int *next);
uint32_t rdnum(const char* buf,const uint32_t len,int *next);
list_t rdlist(const char* buf,const uint32_t len,int* next);
dict_t rddict(const char* buf,const uint32_t len,int *next);
void parse(const char* buf,const uint32_t len)
{
	//torrent_file_t file = (torrent_file_t)malloc(sizeof(struct torrent_file_t));
	uint32_t index = 0;
	while(index<len) {
		switch (buf[index]) {
			case 'd'://dict
			{
				++index;
				int next = 0;
				str_t fstkey = rddict(buf+index,len-index,&next);
				index += next;
			}
				break;
			case 'l'://list
			{
				index++;
				int next = 0;
				list_t list =rdlist(buf+index,len-index,&next);
				index += next;
			}
				break;
			case 'i'://integer
			{
				index++;
				int next = 0;
				uint32_t num = rdnum(buf+index,len-index,&next);
				index += next;
			}
				break;
			default://string
			{
				index++;
				int next = 0;
				str_t ret = rdstr(buf+index,len-index,&next);
				index += next;
			}
				break;
		}
	}
}

str_t rdstr(const char* buf,const uint32_t len,int *next)
{
	int str_len = 0;
	sscanf(buf,"%d",&str_len);
	assert(str_len>0);
	str_t ret = str_alloc(str_len);
	
	int index = 0;
	for(;isdigit(buf[index]);index++)
		continue;
	strncpy(ret,buf+index,str_len);
	*next = index+str_len;
	return ret;
}

uint32_t rdnum(const char* buf,const uint32_t len,int *next)
{
	uint32_t ret = -1;
	sscanf(buf,"%d",&ret);
	assert(ret>=0);
	
	int i=0;	
	for(i=0;'e'!=buf[i];++i)
		continue;
	++i;
	*next = i;
	return ret;
}

list_t rdlist(const char* buf,const uint32_t len,int* next)
{
	list_t ret = NULL;
	int index=0;
	while(index<len && 'e'!=buf[index]) {
		switch (buf[index]) {
			case 'd'://dict
			{
				++index;
				int next = 0;
				str_t fstkey = rddict(buf+index,len-index,&next);
				index += next;
			}
				break;
			case 'l'://list
			{
				index++;
				int next = 0;
				list_t list =rdlist(buf+index,len-index,&next);
				index += next;
			}
				break;
			case 'i'://integer
			{
				index++;
				int next = 0;
				uint32_t num = rdnum(buf+index,len-index,&next);
				index += next;
			}
				break;
			default://string
			{
				index++;
				int next = 0;
				str_t ret = rdstr(buf+index,len-index,&next);
				index += next;
			}
				break;
		}

	}
	return ret;
}

dict_t rddict(const char* buf,const uint32_t len,int *next)
{
	dict_t ret = NULL;
	int index=0;
	while(index<len && 'e'!=buf[index]) {
		switch (buf[index]) {
			case 'd'://dict
			{
				++index;
				int next = 0;
				str_t fstkey = rddict(buf+index,len-index,&next);
				index += next;
			}
				break;
			case 'l'://list
			{
				index++;
				int next = 0;
				list_t list =rdlist(buf+index,len-index,&next);
				index += next;
			}
				break;
			case 'i'://integer
			{
				index++;
				int next = 0;
				uint32_t num = rdnum(buf+index,len-index,&next);
				index += next;
			}
				break;
			default://string
			{
				index++;
				int next = 0;
				str_t ret = rdstr(buf+index,len-index,&next);
				index += next;
			}
				break;
		}

	}
	return ret;
}

void decode(char* path)
{
	const int32_t buf_size = file_size(path);
	if (buf_size<=0) {
		exit(-1);
	}

	char * buf = (char*)malloc(sizeof(char)*buf_size);
	read_torrent_file(path,buf,buf_size);

	parse(buf,buf_size);
	free(buf);
}

