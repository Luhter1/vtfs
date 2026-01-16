#ifndef FS_H
#define FS_H
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

#define MAX_NAME_LEN 255

struct vtfs_entry {
    char name[MAX_NAME_LEN];
    umode_t mode;
    ino_t ino;
    struct list_head list;
    char *data;
    size_t size;
};

struct vtfs_sb_info {
    struct list_head entries;
};


#endif // FS_H