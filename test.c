#include "torrent.h"


int main(int argc, char* argv[])
{
	char* file = "/Users/rannger/Desktop/asdf.torrent";
	torrent_file_t ret = decode(file);
	print_torrent_file(ret);
	free_torrent_file(ret);
	return 0;
}
