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
    struct vtfs_entry *parent;
    struct list_head children;
    char *data;
    size_t size;
    atomic_t refcount; 
    struct vtfs_entry *target;
};

struct vtfs_sb_info {
    struct vtfs_entry *root_entry;
};


#endif // FS_H