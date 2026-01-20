#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/types.h>

#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)
#define MSG_SLOT_SET_CEN _IOW(MAJOR_NUM, 1, unsigned int)

const int MAJOR_NUM = 235;

struct file_private_data {
    unsigned int channel_id;
    int censorship_enabled;
};

#endif