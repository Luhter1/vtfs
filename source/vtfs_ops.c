#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include "vtfs.h"
#include "fs.h"

int GLOB_INODE_COUNTER = 1001;

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

  if(!S_ISDIR(parent_inode->i_mode))
    return ERR_PTR(-ENOTDIR);

  list_for_each_entry(entry, &sbi->entries, list){
      if(strcmp(entry->name, name) == 0){
        inode = vtfs_get_inode(
          parent_inode->i_sb,
          parent_inode,
          entry->mode,
          entry->ino
        );
        inode->i_private = entry;
        inode->i_size = entry->size;
        break;
      }
  }
    if(!inode) {
      return NULL;
    }

  d_add(child_dentry, inode);
  return NULL; // возвращаем NULL, так как файловая система пуста
}

int vtfs_create(
  struct mnt_idmap *id,
  struct inode *parent_inode, 
  struct dentry *child_dentry, 
  umode_t mode, 
  bool b
) {
  struct vtfs_sb_info *sbi = parent_inode->i_sb->s_fs_info;
  struct vtfs_entry *new_entry;
  struct inode *inode;

  if (!S_ISDIR(parent_inode->i_mode))
    return -ENOTDIR;

  new_entry = kzalloc(sizeof(*new_entry), GFP_KERNEL);
  if (!new_entry)
    return -ENOMEM;

  strncpy(new_entry->name, child_dentry->d_name.name, MAX_NAME_LEN);
  new_entry->mode = S_IFREG | mode;
  new_entry->ino = GLOB_INODE_COUNTER++;
  new_entry->data = NULL;
  new_entry->size = 0;

  list_add(&new_entry->list, &sbi->entries);  

  inode = vtfs_get_inode(parent_inode->i_sb, parent_inode,
                          new_entry->mode, new_entry->ino);
  inode->i_private = new_entry;
  inode->i_size = 0;

  d_add(child_dentry, inode);

  return 0;
}