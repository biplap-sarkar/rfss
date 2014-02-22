#include <time.h>

struct ft_stat {
	char *file;
	int fd;
	int bytes_received;
	int bytes_total;
	struct timespec starttime;
	struct timespec endtime;
	struct file_transfer_task *next;
};


	
