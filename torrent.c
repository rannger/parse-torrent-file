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
#include <time.h>

#define __ASSERT(exp) \
do { \
	if (!(exp)) \
	{ \
		printf("%s\n", __func__); \
		assert((exp)); \
	} \
} while(0);

#define __EXIT(code) \
do { \
	printf("%s,%d",__FILE__,__LINE__); \
	exit(code); \
} while(0);

str_t str_alloc(uint32_t len)
{
	len += 1;
	str_t ret = (str_t)malloc(sizeof(char)*len);
	bzero(ret,len);
	return ret;
}

#define __str_release(str) free(str);

list_t list_alloc(void)
{
	list_t list = (list_t)malloc(sizeof(struct list_t));
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
	return list;
}

list_node_t node_alloc(void)
{
	list_node_t node = (list_node_t)malloc(sizeof(struct list_node_t));
	node->val = NULL;
	node->type = E_TYPE_MAX;	
	node->next = NULL;
	return node;
}

void list_release(list_t list);
void free_fi(fi_t fi);
void node_release(list_node_t node)
{

	switch (node->type) {
		case E_NUM:
			break;
		case E_STR:
			__str_release(node->val);
			break;
		case E_FI:
			free_fi((fi_t)node->val);
			break;
		case E_LIST:
			list_release((list_t)node->val);
			break;
		default:
			break;
	}
	free(node);
}

BOOL list_append(list_t list,list_node_t node)
{
	BOOL ret = YES;
	if (0==list->size) {
		list->head = node;
		list->tail = node;
	} else {
		list->tail->next = node;
		list->tail = node;
	}
	list->size++;
	return ret;
}

BOOL list_del_head(list_t list)
{
	BOOL ret = NO;
	if (0!=list->size) {
		list_node_t head = list->head;
		list->head = head->next;
		node_release(head);
		list->size -= 1;
		ret = YES;
	}
	return ret;
}

uint32_t len(list_t list)
{
	return list->size;
}

void list_release(list_t list)
{
	while(list_del_head(list))
		continue;
	free(list);
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
	if (fd<0) 
		__EXIT(-1);
	int s = 0;
	do {
		int res = read(fd,buf+s,buf_len-s);
		if (res>0) s+=res;
		if (res<0) {
			close(fd);
			__EXIT(-1);
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

#define __isdigitch(t) ((t)<='9'&&(t)>='0')

str_t rdstr(const char* buf,const uint32_t len,int *next);
uint64_t rdnum(const char* buf,const uint32_t len,int *next);
list_t rdlist(const char* buf,const uint32_t len,int* next);
torrent_info_t rdinfo(const char* buf,const uint32_t len,int *next);
torrent_mulit_file_info_t rdmfi(const char* buf,const uint32_t len,int* next);
fi_t rdfi(const char* buf,const uint32_t len,int* next);
void free_torrent_mulit_file_info(torrent_mulit_file_info_t info);
void free_torrent_info(torrent_info_t info);
void print_node(list_node_t node);
void print_list(list_t list);
void print_file_info(fi_t info);

void print_list(list_t list)
{
	printf("[");
	for(list_node_t node = list->head;NULL!=node;node = node->next) {
		print_node(node);
		printf(",");
	}
	printf("]");
}

void print_file_info(fi_t info)
{
	print_list(info->path);
}

void print_node(list_node_t node)
{
	switch (node->type) {
		case E_NUM:
			printf("%u",node->val);
			break;
		case E_STR:
			printf("%s",node->val);
			break;
		case E_LIST:
			print_list(node->val);			
			break;
		case E_FI:
			print_file_info((fi_t)(node->val));
			break;
		default:
			break;
	}
}

void print_single_file_info(torrent_single_file_info_t info)
{
	printf("length:%u",info->length);
}

void print_mulit_file_info(torrent_mulit_file_info_t info)
{
	print_list(info->files);
}

void print_time(time_t i)
{		
	char buffer[26] = {0};
	struct tm* tm_info = NULL;
	tm_info = localtime(&i);
	strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	printf("%s",buffer);
}


void print_torrent_info(torrent_info_t info)
{
	puts("{");
	printf("\t\t\t\tpiece_len:%u\n",info->piece_len);
	printf("\t\t\t\tname:%s\n",info->name);
	printf("\t\t\t\t");
	if (NULL!=info->single_file_info) {
		print_single_file_info(info->single_file_info);
	} else if (NULL!=info->mulit_file_info) {
		print_mulit_file_info(info->mulit_file_info);
	}
	puts("\n\t\t}");
}

void print_torrent_file(torrent_file_t file)
{
	puts("{");
	printf("\t\tannounce:%s\n",file->announce);
	printf("\t\t:announce list:");
	print_list(file->announce_list);
	puts("");
	printf("\t\tcreateion_date:");
	print_time(file->createion_date);
	printf("\n");
	printf("\t\tcomment:%s\n",file->comment);
	printf("\t\tcreator:%s\n",file->creator);
	printf("\t\tinfo:");
	print_torrent_info(file->info);
	puts("}");
}


str_t rdstr(const char* buf,const uint32_t len,int *next)
{
	int str_len = 0;
	sscanf(buf,"%d",&str_len);
	__ASSERT(str_len>0);
	str_t ret = str_alloc(str_len);
	
	int index = 0;
	for(;__isdigitch(buf[index]);index++)
		continue;
	strncpy(ret,buf+index+1,str_len);
	*next = index+str_len+1;
	return ret;
}

uint64_t rdnum(const char* buf,const uint32_t len,int *next)
{
	uint64_t ret = -1;
	sscanf(buf+1,"%lld",&ret);
	__ASSERT(ret>=0);
	
	int i=0;	
	for(i=0;'e'!=buf[i];++i)
		continue;
	++i;
	*next = i;
	return ret;
}

list_t rdlist(const char* buf,const uint32_t len,int* next)
{
	list_t ret = list_alloc();
	int index=1;
	while(index<len && 'e'!=buf[index]) {
		list_node_t node = NULL;
		switch (buf[index]) {
			case 'd'://dict
				__ASSERT(0);
				break;
			case 'l'://list
			{
				int next = 0;
				list_t list =rdlist(buf+index,len-index,&next);
				index += next;
				node = node_alloc();
				node->val = list;
				node->type = E_LIST;
				node->next = NULL;
			}
				break;
			case 'i'://integer
			{
				int next = 0;
				uint64_t num = rdnum(buf+index,len-index,&next);
				index += next;

				node = node_alloc();
				node->val = num;
				node->type = E_NUM;
				node->next = NULL;
			}
				break;
			default://string
			{
				int next = 0;
				str_t str = rdstr(buf+index,len-index,&next);
				index += next;
				node = node_alloc();
				node->val = str;
				node->next = NULL;
				node->type = E_STR;
			}
				break;
		}
		if (NULL!=node) 
			list_append(ret,node);
	}
	if ('e'==buf[index]) ++index;	
	*next = index;

	return ret;
}

torrent_file_t alloc_torrent_file(void)
{
	torrent_file_t ret = (torrent_file_t)malloc(sizeof(struct torrent_file_t));
	bzero(ret,sizeof(struct torrent_file_t));
	return ret;
}


void free_torrent_file(torrent_file_t info)
{
	if (NULL!=info->announce)
		__str_release(info->announce);
	if (NULL!=info->announce_list)
		list_release(info->announce_list);
	if (NULL!=info->comment)
		__str_release(info->comment);
	if (NULL!=info->creator)
		__str_release(info->creator);
	if (NULL!=info->encoding)
		__str_release(info->encoding);
	if (NULL!=info->info)
		free_torrent_info(info->info);
	free(info);
}

torrent_file_t torrent_file(const char* buf,uint32_t len)
{
	uint32_t index = 0;
	torrent_file_t ret = alloc_torrent_file();
	__ASSERT(buf[index]=='d');
	++index;
	while(index<len && 'e'!=buf[index]) {
		int next = 0;
		str_t key = rdstr(buf+index,len-index,&next);
		index += next;
		if (strcmp(key,"announce")==0) {
			int next = 0;
			str_t announce = rdstr(buf+index,len-index,&next);
			index += next;
			ret->announce = announce;
		} else if (strcmp(key,"announce-list")==0) {
			int next = 0;
			list_t list = rdlist(buf+index,len-index,&next);
			index += next;
			ret->announce_list = list;
		} else if (strcmp(key,"creation date")==0) {
			int next = 0;
			uint64_t timestamp = rdnum(buf+index,len-index,&next);
			index += next;
			ret->createion_date = timestamp;
		} else if (strcmp(key,"comment")==0) {
			int next = 0;
			str_t comment = rdstr(buf+index,len-index,&next);
			index += next;
			ret->comment = comment;
		} else if (strcmp(key,"created by")==0) {
			int next = 0;
			str_t creator = rdstr(buf+index,len-index,&next);
			index += next;
			ret->creator = creator;
		} else if (strcmp(key,"info")==0) {
			int next = 0;
			torrent_info_t info = rdinfo(buf+index,len-index,&next);
			index += next;
			ret->info = info;
		} else if (strcmp(key,"encoding")==0) {
			int next = 0;
			str_t encoding = rdstr(buf+index,len-index,&next);
			index += next;
			ret->encoding = encoding;
		} else {
			fprintf(stderr,"unknow key:%s(%s)",key,__func__);
			__ASSERT(0);			
		}
	}

	return ret;
}

torrent_info_t alloc_torrent_info(void)
{
	torrent_info_t ret = (torrent_info_t)malloc(sizeof(struct torrent_info_t));
	bzero(ret,sizeof(struct torrent_info_t));
	return ret;
}

void free_torrent_info(torrent_info_t info)
{
	if (info->pieces!=NULL)
		free(info->pieces);
	if (info->name!=NULL)
		__str_release(info->name);	
	if (info->single_file_info!=NULL)
		free(info->single_file_info);
	if (info->mulit_file_info!=NULL)
		free_torrent_mulit_file_info(info->mulit_file_info);
	free(info);
}

torrent_info_t rdinfo(const char* buf,const uint32_t len,int *next)
{
	uint32_t index = 0;
	torrent_info_t ret = alloc_torrent_info();
	__ASSERT(buf[index]=='d');
	++index;

	while(index<len && 'e'!=buf[index]) {
		int next = 0;
		str_t key = rdstr(buf+index,len-index,&next);
		index += next;
		if (strcmp(key,"piece length")==0) {
			int next = 0;
			uint64_t piece_len = rdnum(buf+index,len-index,&next);
			index += next;
			ret->piece_len = piece_len;
		} else if (strcmp(key,"pieces")==0) {
			const char* buffer = buf+index;
			int next = 0;
			do {
				if (buffer[next]==':')
					break;
				next++;
			}while(1);
			next++;
			uint64_t len = 0;
			sscanf(buffer,"%lld",&len);
			next += len;
			index += next;
		} else if (strcmp(key,"name")==0) {
			int next = 0;
			str_t name = rdstr(buf+index,len-index,&next);
			index += next;
			ret->name = name;
		} else if (strcmp(key,"length")==0) {
			int next = 0;
			uint64_t length = rdnum(buf+index,len-index,&next);
			index += next;
			torrent_single_file_info_t sinfo = (torrent_single_file_info_t)malloc(sizeof(struct torrent_single_file_info_t));
			bzero(sinfo,sizeof(struct torrent_single_file_info_t));
			sinfo->length = length;
			ret->single_file_info = sinfo;
		} else if (strcmp(key,"files")==0) {
			/* code */
			int next = 0;
			torrent_mulit_file_info_t minfo = rdmfi(buf+index,len-index,&next);
			index += next;
			ret->mulit_file_info = minfo;
		}
	}
	*next = index;
	return ret;
}

torrent_mulit_file_info_t alloc_torrent_mulit_file_info(torrent_mulit_file_info_t info)
{
	torrent_mulit_file_info_t ret = (torrent_mulit_file_info_t)malloc(sizeof(struct torrent_mulit_file_info_t));
	bzero(ret,sizeof(struct torrent_mulit_file_info_t));
	return ret;
}

void free_torrent_mulit_file_info(torrent_mulit_file_info_t info)
{
	list_release(info->files);
	free(info);
	return;
}

torrent_mulit_file_info_t rdmfi(const char* buf,const uint32_t len,int* next)
{
	torrent_mulit_file_info_t ret = (torrent_mulit_file_info_t)malloc(sizeof(struct torrent_mulit_file_info_t));
	list_t list = list_alloc();
	uint32_t index = 0;
	__ASSERT('l'==buf[index]);
	++index;

	while(index<len && 'e'!=buf[index]) {
		int next = 0;
		fi_t fi = rdfi(buf+index,len-index,&next);
		index += next;
		list_node_t node = node_alloc();
		node->val = fi;
		node->type = E_FI;
		node->next = NULL;
		list_append(list,node);
	}
	return ret;
}

void free_fi(fi_t fi)
{
	list_release(fi->path);
}

fi_t rdfi(const char* buf,const uint32_t len,int* next)
{
	fi_t ret = (fi_t)malloc(sizeof(struct fi_t));
	uint32_t index = 0;
	__ASSERT('d'==buf[index]);
	index++;
	while(index<len && 'e'!=buf[index]) {
		int next = 0;
		str_t key = rdstr(buf+index,len-index,&next);
		__ASSERT(strlen(key)==next);
		index+=next;
		if (strcmp(key,"length") == 0) {
			int next = 0;
			uint64_t length = rdnum(buf+index,len-index,&next);
			index += next;
			ret->length = length;
		} else if (strcmp(key,"path") == 0) {
			int next = 0;
			list_t path = rdlist(buf+index,len-index,&next);
			index += next;
			ret->path = path;
		}
	}

	return ret;
}

torrent_file_t decode(char* path)
{
	const int32_t buf_size = file_size(path);
	if (buf_size<=0) {
		__EXIT(-1);
	}

	char * buf = (char*)malloc(sizeof(char)*buf_size);
	read_torrent_file(path,buf,buf_size);

	torrent_file_t ret = torrent_file(buf,buf_size);
	free(buf);
	return ret;
}

