#ifndef FS_H
#define FS_H
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

struct vtfs_entry {
    char name[500];
    umode_t mode;
    ino_t ino;
    struct list_head list;
};

struct vtfs_sb_info {
    struct list_head entries;
};


#endif // FS_H