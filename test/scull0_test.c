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
    int res = 0;
    struct stat scull_stat;

    memset(buff, 0, 4001);

    res = stat(SCULL_DEVICE_FILE, &scull_stat);
    if(-1 == res)
    {
        perror("Failed to recognize scull device");
        exit(1);
    }

    if(!S_ISCHR(scull_stat.st_mode))
    {
        printf("Not a char device, probably scull not loaded.\n");
        exit(1);
    }

    if(-1 == (fd = open(SCULL_DEVICE_FILE, O_RDWR)))
    {
        perror("Failed open file");
        exit(1);
    }

    if(-1 == (read_size = read(fd, buff, sizeof(buff))))
    {
        perror("Failed to read");
    }

    printf("read_size=%ld\n", read_size);

    close(fd);
}
