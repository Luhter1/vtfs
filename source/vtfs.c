#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include "vtfs_init.h"

#define MODULE_NAME "vtfs"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("secs-dev");
MODULE_DESCRIPTION("A simple FS kernel module");

#define LOG(fmt, ...) pr_info("[" MODULE_NAME "]: " fmt, ##__VA_ARGS__)

struct file_system_type vtfs_fs_type = {
  .owner = THIS_MODULE,
  .name = "vtfs",
  .mount = vtfs_mount, // функция монтирования
  .kill_sb = vtfs_kill_sb, // функция размонтирования
};


static int __init vtfs_init(void) {
  register_filesystem(&vtfs_fs_type);
  LOG("VTFS joined the kernel\n");
  return 0;
}

static void __exit vtfs_exit(void) {
  unregister_filesystem(&vtfs_fs_type);
  LOG("VTFS left the kernel\n");
}

module_init(vtfs_init);
module_exit(vtfs_exit);
