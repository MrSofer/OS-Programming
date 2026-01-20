#include "message_slot.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

#define MAX_CHANNELS_PER_SLOT 16
#define CHANNEL_ALLOC_INCREMENT 8


struct message_channel {
    char *content;
    size_t length;
    int channel_id;
};

struct message_slot {
    int minor_num;
    struct message_channel *message_channels;
    unsigned int num_channels;
};


static struct message_slot *message_slots;

static int device_open(struct inode *inode, struct file *file) {
    int minor_num = iminor(inode);
    struct file_private_data *file_data;
    printk(KERN_INFO "message_slot: device_open called for minor %d\n", minor_num);

    file_data = kmalloc(sizeof(struct file_private_data), GFP_KERNEL);
    if (!file_data) {
        printk(KERN_ERR "message_slot: Can't allocate private_data for minor %d\n", minor_num);
        return -ENOMEM;
    }
    file_data->channel_id = -1;
    file_data->censorship_enabled = 0;

    file->private_data = file_data;

    printk(KERN_INFO "message_slot: Private data allocated for minor %d\n", minor_num);
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    struct file_private_data *file_data = (struct file_private_data *)file->private_data;
    printk(KERN_INFO "message_slot: device_release called\n");
    if (file_data) {
        kfree(file_data);
        file->private_data = NULL;
        printk(KERN_INFO "message_slot: Freed private data\n");
    }
    return 0;
}

static long device_ioctl(struct file *file, unsigned int ioctl_command_id, unsigned long ioctl_param) {
    struct file_private_data *fpd = (struct file_private_data *)file->private_data;
    switch (ioctl_command_id) {
        case MSG_SLOT_CHANNEL:
            if (ioctl_param > 0) {
                fpd->channel_id = ioctl_param;
            } else {
                return -EINVAL;
            }
            break;
        case MSG_SLOT_SET_CEN:
            if (ioctl_param == 0 || ioctl_param == 1) {
                fpd->censorship_enabled = ioctl_param;
            } else {
                return -EINVAL;
            }
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static ssize_t device_write(struct file *file, const char __user *buffer, size_t count, loff_t *f_pos) {
    struct file_private_data *fpd = (struct file_private_data *)file->private_data;
    char *k_buffer;
    int i;
    int minor_num = iminor(file->f_inode);
    struct message_slot *current_slot = &message_slots[minor_num];
    struct message_channel *current_channel = NULL;
    int channel_found = 0;
    struct message_channel *temp_channels;

    if (fpd->channel_id != -1) {
        if (count == 0 || count > 128) {
            return -EMSGSIZE;
        }

        k_buffer = kmalloc(count, GFP_KERNEL);
        if (!k_buffer) {
            printk(KERN_ERR "message_slot: kmalloc failed in device_write\n");
            return -ENOMEM;
        }
        if (copy_from_user(k_buffer, buffer, count) != 0) {
            printk(KERN_ERR "message_slot: copy_from_user failed in device_write\n");
            kfree(k_buffer);
            return -EFAULT;
        }

        for (i = 0; i < current_slot->num_channels; ++i) {
            if (current_slot->message_channels[i].channel_id == fpd->channel_id) {
                current_channel = &current_slot->message_channels[i];
                channel_found = 1;
                break;
            }
        }

        if (!channel_found) {
            struct message_channel *new_channel = kmalloc(sizeof(struct message_channel), GFP_KERNEL);
            if (!new_channel) {
                printk(KERN_ERR "message_slot: Failed to allocate new channel\n");
                kfree(k_buffer);
                return -ENOMEM;
            }
            new_channel->channel_id = fpd->channel_id;
            new_channel->content = NULL;
            new_channel->length = 0;

            temp_channels = krealloc(current_slot->message_channels,
                                                        (current_slot->num_channels + 1) * sizeof(struct message_channel),
                                                        GFP_KERNEL);
            if (!temp_channels) {
                printk(KERN_ERR "message_slot: Failed to reallocate channel array\n");
                kfree(k_buffer);
                kfree(new_channel);
                return -ENOMEM;
            }
            current_slot->message_channels = temp_channels;
            current_slot->message_channels[current_slot->num_channels] = *new_channel;
            kfree(new_channel);
            current_channel = &current_slot->message_channels[current_slot->num_channels];
            current_slot->num_channels++;
            printk(KERN_INFO "message_slot: Channel %lu created\n", (unsigned long)fpd->channel_id);
        }

        if (current_channel->content != NULL) {
            kfree(current_channel->content);
        }

        current_channel->content = kmalloc(count + 1, GFP_KERNEL);
        if (!current_channel->content) {
            printk(KERN_ERR "message_slot: Failed to allocate message content\n");
            kfree(k_buffer);
            return -ENOMEM;
        }

        strncpy(current_channel->content, k_buffer, count);
        current_channel->content[count] = '\0';

        if (fpd->censorship_enabled == 1) {
            for (i = 3; i < count; i += 4) {
                printk("Censoring character at position %d\n", i);
                current_channel->content[i] = '#';
            }
        }

        current_channel->length = count;
        kfree(k_buffer);
        return count;
    } else {
        return -EINVAL;
    }
}

static ssize_t device_read(struct file *file, char __user *buffer, size_t count, loff_t *f_pos){
    struct file_private_data *fpd = (struct file_private_data *)file->private_data;
    int i;
    int minor_num = iminor(file->f_inode);
    struct message_slot *current_slot = &message_slots[minor_num];
    struct message_channel *current_channel = NULL;
    int channel_found = 0;
    int bytes_read = 0;

    if (fpd->channel_id != -1) {
        for (i = 0; i < current_slot->num_channels; ++i) {
            if (current_slot->message_channels[i].channel_id == fpd->channel_id) {
                current_channel = &current_slot->message_channels[i];
                channel_found = 1;
                break;
            }
        }
        if (!channel_found){
            return -EWOULDBLOCK;
        }

        if (current_channel) {
            if (current_channel->content != NULL && current_channel->length != 0) {
                //bytes_read = min((size_t)count, current_channel->length);
                if (count < current_channel->length) {
                    return -ENOSPC;
                }
                bytes_read = current_channel->length;
                if (copy_to_user(buffer, current_channel->content, bytes_read) != 0) {
                    printk(KERN_ERR "message_slot: copy_to_user failed\n");
                    return -EFAULT;
                }
                printk(KERN_INFO "message_slot: Read %d bytes from channel %lu\n", bytes_read, (unsigned long)fpd->channel_id);
                return bytes_read;
            } else {
                return -EWOULDBLOCK;
            }
        } else {
            return -EINVAL;
        }
    } else {
        return -EINVAL;
    }
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = device_ioctl,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};


static int __init init_msg_slt(void) { 
    int ret = register_chrdev(MAJOR_NUM, "message_slot", &fops);
    if (ret < 0) {
        printk(KERN_ERR "message_slot: Can't register major number %d\n", MAJOR_NUM);
        return ret;
    }

    message_slots = kmalloc(256 * sizeof(struct message_slot), GFP_KERNEL);
    if (!message_slots) {
        printk(KERN_ERR "message_slot: Failed to allocate message_slots\n");
        unregister_chrdev(MAJOR_NUM, "message_slot");
        return -ENOMEM;
    }
    memset(message_slots, 0, 256 * sizeof(struct message_slot));

    printk(KERN_INFO "message_slot: Module initialized\n");
    return 0;
}

static void __exit cleanup_msg_slt(void) {
    int i;
    unsigned long j;
    
    printk(KERN_INFO "message_slot: Cleaning up module\n");
    unregister_chrdev(MAJOR_NUM, "message_slot");
    printk(KERN_INFO "message_slot: Unregistered character device\n");
    if (message_slots) {
        for (i = 0; i < 256; i++) {
            if (message_slots[i].message_channels) {
                for (j = 0; j < message_slots[i].num_channels; ++j) {
                    if (message_slots[i].message_channels[j].content) {
                        kfree(message_slots[i].message_channels[j].content);
                    }
                }
                kfree(message_slots[i].message_channels);
            }
        }
        kfree(message_slots);
        printk(KERN_INFO "message_slot: Freed message_slots memory\n");
    }
    printk(KERN_INFO "message_slot: Module unloaded\n");
}


module_init(init_msg_slt);
module_exit(cleanup_msg_slt);