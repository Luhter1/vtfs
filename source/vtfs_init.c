#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include "vtfs.h"

const struct inode_operations vtfs_inode_ops = {
  .lookup = vtfs_lookup,
};

// создает новую структуру inode для корня файловой системы
struct inode* vtfs_get_inode( 
  struct super_block* sb, 
  const struct inode* dir, 
  umode_t mode, 
  int i_ino
) {
  struct inode *inode = new_inode(sb);
  if (!inode)
    return NULL;

  inode_init_owner(&nop_mnt_idmap, inode, dir, mode);

  inode->i_op = &vtfs_inode_ops;
  inode->i_ino = i_ino;
  inode->i_sb  = sb;

  return inode;
}

// заполняет структуру super_block инфой о файловой системе
int vtfs_fill_super(struct super_block *sb, void *data, int silent) {
  struct inode* inode = vtfs_get_inode(sb, NULL, S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO, 1000);
  sb->s_root = d_make_root(inode);
  if (sb->s_root == NULL) {
  return -ENOMEM;
  }
  printk(KERN_INFO "return 0\n");
  return 0;
}

// функция, которая вызывается при монтировании
struct dentry* vtfs_mount(
  struct file_system_type* fs_type,
  int flags,
  const char* token,
  void* data
) {
  struct dentry* ret = mount_nodev(fs_type, flags, data, vtfs_fill_super);
  if (ret == NULL) {
    printk(KERN_ERR "Can't mount file system");
  } else {
    printk(KERN_INFO "Mounted successfuly");
  }
  return ret;
}

// функция, которая вызывается при размонтировании
void vtfs_kill_sb(struct super_block* sb) {
  printk(KERN_INFO "vtfs super block is destroyed. Unmount successfully.\n");
}