#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "message_slot.h"
#include <errno.h>

#define BUFFER_SIZE 129

int main(int argc, char *argv[]) {
    int fd;
    unsigned long channel_id;
    char buffer[BUFFER_SIZE];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <device_file> <channel_id>\n", argv[0]);
        exit(1);
    }

    char *device_file = argv[1];
    if (sscanf(argv[2], "%lu", &channel_id) != 1) {
        fprintf(stderr, "Error: Invalid channel ID '%s'\n", argv[2]);
        exit(1);
    }

    fd = open(device_file, O_RDONLY);
    if (fd < 0) {
        perror("Error opening device");
        exit(1);
    }

    if (ioctl(fd, MSG_SLOT_CHANNEL, channel_id) < 0) {
        perror("Error setting channel ID");
        close(fd);
        exit(1);
    }

    ssize_t bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("Error reading from device");
        close(fd);
        exit(1);
    } else {
        buffer[bytes_read] = '\0';
        /*printf("Read %zd bytes from channel %lu on %s: \"%s\"\n",
               bytes_read, channel_id, device_file, buffer);*/
        write(STDOUT_FILENO, buffer, bytes_read);
    }

    close(fd);
    return 0;
}