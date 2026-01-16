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
  struct vtfs_entry *parent_entry = parent_inode->i_private;
  struct vtfs_entry *entry;
  const char *name = child_dentry->d_name.name;
  struct inode *inode = NULL;

  printk(KERN_INFO "vtfs_lookup called for %s\n", child_dentry->d_name.name);

  if(!S_ISDIR(parent_inode->i_mode))
    return ERR_PTR(-ENOTDIR);

  if (!parent_entry)
    return ERR_PTR(-ENOENT);

  // Поиск в children родительской директории
  list_for_each_entry(entry, &parent_entry->children, list){
      if(strcmp(entry->name, name) == 0){
        inode = vtfs_get_inode(
          parent_inode->i_sb,
          parent_inode,
          entry->mode,
          entry->ino
        );
        if (!inode)
          return ERR_PTR(-ENOMEM);
        inode->i_private = entry;
        inode->i_size = entry->size;
        break;
      }
  }

  d_add(child_dentry, inode);
  return NULL;
}

int vtfs_create(
  struct mnt_idmap *id,
  struct inode *parent_inode,
  struct dentry *child_dentry,
  umode_t mode,
  bool b
) {
  struct vtfs_entry *parent_entry = parent_inode->i_private;
  struct vtfs_entry *entry, *new_entry;
  struct inode *inode;

  if (!S_ISDIR(parent_inode->i_mode))
    return -ENOTDIR;

  if (!parent_entry)
    return -ENOENT;

  // Проверка на существование файла с таким же именем
  list_for_each_entry(entry, &parent_entry->children, list) {
    if (strcmp(entry->name, child_dentry->d_name.name) == 0)
      return -EEXIST;
  }

  new_entry = kzalloc(sizeof(*new_entry), GFP_KERNEL);
  if (!new_entry)
    return -ENOMEM;

  // Безопасное копирование имени файла с гарантией завершения нулевым символом
  strncpy(new_entry->name, child_dentry->d_name.name, MAX_NAME_LEN - 1);
  new_entry->name[MAX_NAME_LEN - 1] = '\0';
  new_entry->mode = S_IFREG | mode;
  new_entry->ino = GLOB_INODE_COUNTER++;
  new_entry->parent = parent_entry;
  new_entry->data = NULL;
  new_entry->size = 0;
  new_entry->target = NULL;
  INIT_LIST_HEAD(&new_entry->children);
  atomic_set(&new_entry->refcount, 1);

  list_add(&new_entry->list, &parent_entry->children);

  inode = vtfs_get_inode(parent_inode->i_sb, parent_inode,
                          new_entry->mode, new_entry->ino);
  if(!inode){
    list_del(&new_entry->list);
    kfree(new_entry);
    return -ENOMEM;
  }
  inode->i_private = new_entry;
  inode->i_size = 0;

  d_add(child_dentry, inode);

  return 0;
}