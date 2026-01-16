#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include "vtfs.h"
#include "fs.h"

struct dentry* vtfs_lookup(
  struct inode* parent_inode,  // родительская нода
  struct dentry* child_dentry, // объект, к которому мы пытаемся получить доступ
  unsigned int flag            // неиспользуемое значение
) {
  struct vtfs_entry *entry;
  struct vtfs_sb_info *sbi = parent_inode->i_sb->s_fs_info;
  const char *name = child_dentry->d_name.name;
  struct inode *inode = NULL;

  printk(KERN_INFO "vtfs_lookup called for %s\n", child_dentry->d_name.name);

  if (!S_ISDIR(parent_inode->i_mode))
    return ERR_PTR(-ENOTDIR);

  list_for_each_entry(entry, &sbi->entries, list) {
      if (strcmp(entry->name, name) == 0) {
        inode = vtfs_get_inode(
          parent_inode->i_sb,
          parent_inode,
          entry->mode,
          entry->ino
        );
        break;
      }
  }
    if (inode) {
        if (!S_ISDIR(entry->mode)) {
            inode->i_op  = NULL;
        }
    }else{
      return ERR_PTR(-ENOENT);
    }

  d_add(child_dentry, inode);
  return NULL; // возвращаем NULL, так как файловая система пуста
}