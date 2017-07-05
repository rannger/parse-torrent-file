#ifndef __TORRENT_H
#define __TORRENT_H
#include <stdint.h>


typedef uint8_t BOOL;
#define YES (1)
#define NO (0)

#define __BEGIN_TYPE_DEFINE(struct_name) \
struct struct_name {

#define __END_TYPE_DEFINE(struct_name) \
}; \
typedef struct struct_name* struct_name;

#define __MEMBER_DEFINE(name,type) \
type name;

typedef enum value_type_t
{
	E_NUM = 0,
	E_STR,
	E_LIST,
	E_FI,
	E_TYPE_MAX = E_FI
} value_type_t;

typedef char* str_t;

__BEGIN_TYPE_DEFINE(list_node_t)
	__MEMBER_DEFINE(val,void*)
	__MEMBER_DEFINE(type,value_type_t)
	__MEMBER_DEFINE(next,struct list_node_t*)
__END_TYPE_DEFINE(list_node_t)

__BEGIN_TYPE_DEFINE(list_t)
	__MEMBER_DEFINE(head,list_node_t)
	__MEMBER_DEFINE(tail,list_node_t)
	__MEMBER_DEFINE(size,uint32_t)
__END_TYPE_DEFINE(list_t)

__BEGIN_TYPE_DEFINE(torrent_single_file_info_t)
	__MEMBER_DEFINE(length,uint32_t)
__END_TYPE_DEFINE(torrent_single_file_info_t)

__BEGIN_TYPE_DEFINE(fi_t)
	__MEMBER_DEFINE(length,uint32_t)
	__MEMBER_DEFINE(path,list_t)
__END_TYPE_DEFINE(fi_t)

__BEGIN_TYPE_DEFINE(torrent_mulit_file_info_t)
	__MEMBER_DEFINE(files,list_t) //item is fi_t
__END_TYPE_DEFINE(torrent_mulit_file_info_t)

__BEGIN_TYPE_DEFINE(torrent_info_t)
	__MEMBER_DEFINE(piece_len,uint64_t)
	__MEMBER_DEFINE(pieces,uint8_t*)
	__MEMBER_DEFINE(name,str_t)
	__MEMBER_DEFINE(single_file_info,torrent_single_file_info_t)
	__MEMBER_DEFINE(mulit_file_info,torrent_mulit_file_info_t)
__END_TYPE_DEFINE(torrent_info_t)

__BEGIN_TYPE_DEFINE(torrent_file_t)
	__MEMBER_DEFINE(announce,str_t)
	__MEMBER_DEFINE(announce_list,list_t)
	__MEMBER_DEFINE(createion_date,uint64_t)
	__MEMBER_DEFINE(comment,str_t)
	__MEMBER_DEFINE(creator,str_t)
	__MEMBER_DEFINE(encoding,str_t)
	__MEMBER_DEFINE(info,torrent_info_t)
__END_TYPE_DEFINE(torrent_file_t)


void free_torrent_file(torrent_file_t info);
torrent_file_t decode(char* path);
#endif
