#ifndef __TORRENT_H
#define __TORRENT_H
#include <stdint.h>

typedef enum value_type_t
{
	E_NUM = 0,
	E_STR,
	E_LIST,
	E_DICT,
	E_TYPE_MAX = E_DICT
} value_type_t;

typedef struct list_node_t
{
	void* val;
	value_type_t type;
} list_node_t;

typedef list_node_t* list_t;

typedef struct dict_node_t
{
	char* key;
	void* val;
	value_type_t type;
} dict_node_t;

typedef dict_node_t* dict_t;

typedef char* str_t;

typedef struct torrent_file_t
{
	str_t announce;
	list_t announce_list;
	uint32_t announce_list_size;
	uint64_t creation_date;
	str_t comment;
	str_t creator;


} torrent_file_t;

#endif
