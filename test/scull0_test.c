#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

const char * SCULL_DEVICE_FILE = "/dev/scull0";

int main()
{
	int fd;
	ssize_t read_size = 0;
	char buff[4001];

	memset(buff, 0, 4001);

	if(-1 == (fd = open(SCULL_DEVICE_FILE, (O_RDWR | O_CREAT))))
	{
		perror("failed open file");
		exit(1);
	}

	if(-1 == (read_size = read(fd, buff, sizeof(buff))))
	{
		perror("Failed to read");
	}

	printf("read_size=%ld\n", read_size);

	close(fd);
}
