#ifndef VTFS_INIT_H
#define VTFS_INIT_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

// функции инициализации и монтирования файловой системы
struct dentry* vtfs_mount(struct file_system_type* fs_type, int flags, const char* token, void* data);
void vtfs_kill_sb(struct super_block* sb);
struct inode* vtfs_get_inode(struct super_block* sb, const struct inode* dir, umode_t mode, int i_ino);
int vtfs_fill_super(struct super_block *sb, void *data, int silent);

// операции с файлами и каталогами
struct dentry* vtfs_lookup(
  struct inode* parent_inode,  // родительская нода
  struct dentry* child_dentry, // объект, к которому мы пытаемся получить доступ
  unsigned int flag            // неиспользуемое значение
);

#endif // VTFS_INIT_H