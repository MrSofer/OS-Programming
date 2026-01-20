#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "message_slot.h"

#define MAX_MESSAGE_LENGTH 128

int main(int argc, char *argv[]) {
    int fd;
    unsigned long channel_id;
    char *message = "";
    int censorship = 0;

    // if (argc < 4) {
    //     fprintf(stderr, "Usage: %s <device_file> <channel_id> [message] [--censored]\n", argv[0]);
    //     exit(1);
    // }

    // char *device_file = argv[1];
    // if (sscanf(argv[2], "%lu", &channel_id) != 1) {
    //     fprintf(stderr, "Error: Invalid channel ID '%s'\n", argv[2]);
    //     exit(1);
    // }

    // if (argc >= 5) {
    //     if (strcmp(argv[argc - 1], "--censored") == 0) {
    //         censorship = 1;
    //         if (argc == 5) {
    //             message = argv[3];
    //         } else if (argc == 4) {
    //             message = "";
    //         }
    //     } else {
    //         message = argv[3];
    //     }
    // }

    if (argc != 5) {
        fprintf(stderr, "Usage: %s <device_file> <channel_id> <censorship_mode> <message>\n", argv[0]);
        exit(1);
    }

    char *device_file = argv[1];

    if (sscanf(argv[2], "%lu", &channel_id) != 1) {
        fprintf(stderr, "Error: Invalid channel ID '%s'\n", argv[2]);
        exit(1);
    }

    int censorship_arg = atoi(argv[3]);
    if (censorship_arg != 0 && censorship_arg != 1) {
         fprintf(stderr, "Error: Censorship mode must be 0 or 1\n");
         exit(1);
    }
    censorship = censorship_arg;
    message = argv[4];

    if (strlen(message) > MAX_MESSAGE_LENGTH) {
        fprintf(stderr, "Error: Message too long (max %d bytes)\n", MAX_MESSAGE_LENGTH);
        exit(1);
    }

    fd = open(device_file, O_RDWR);
    if (fd < 0) {
        perror("Error opening device");
        exit(1);
    }

    if (ioctl(fd, MSG_SLOT_CHANNEL, channel_id) < 0) {
        perror("Error setting channel ID");
        close(fd);
        exit(1);
    }

    if (ioctl(fd, MSG_SLOT_SET_CEN, censorship) < 0) {
        perror("Error setting censorship");
        close(fd);
        exit(1);
    }

    ssize_t bytes_written = write(fd, message, strlen(message));
    if (bytes_written < 0) {
        perror("Error writing to device");
    }

    close(fd);

    return 0;
}