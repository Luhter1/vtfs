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

// операции с каталогами и файлами (inode operations)
struct dentry* vtfs_lookup(
  struct inode* parent_inode,  // родительская нода
  struct dentry* child_dentry, // объект, к которому мы пытаемся получить доступ
  unsigned int flag            // неиспользуемое значение
);
int vtfs_create(struct mnt_idmap *id, struct inode *parent_inode, struct dentry *child_dentry, umode_t mode, bool b);
int vtfs_unlink(struct inode *parent_inode, struct dentry *child_dentry);
int vtfs_mkdir(struct mnt_idmap *id, struct inode *parent_inode, struct dentry *child_dentry, umode_t mode);
int vtfs_rmdir(struct inode *parent_inode, struct dentry *child_dentry);
int vtfs_link(struct dentry *old_dentry, struct inode *parent_inode, struct dentry *new_dentry);

// операции с файлами (file operations)
int vtfs_iterate(struct file *filp, struct dir_context *ctx);
ssize_t vtfs_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos);
ssize_t vtfs_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos);
#endif // VTFS_INIT_H