#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include "vtfs.h"

struct dentry* vtfs_lookup(
  struct inode* parent_inode,  // родительская нода
  struct dentry* child_dentry, // объект, к которому мы пытаемся получить доступ
  unsigned int flag            // неиспользуемое значение
) {
  printk(KERN_INFO "vtfs_lookup called for %s\n", child_dentry->d_name.name);
  return NULL; // возвращаем NULL, так как файловая система пуста
}