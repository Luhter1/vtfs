#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include "vtfs.h"
#include "fs.h"

const struct inode_operations vtfs_inode_ops = {
  .lookup = vtfs_lookup,
  .create = vtfs_create,
  .unlink = vtfs_unlink,
  .mkdir  = vtfs_mkdir,
  .rmdir  = vtfs_rmdir,
  .link   = vtfs_link,
};

const struct inode_operations vtfs_file_inode_ops = {
  .setattr = simple_setattr,
  .getattr = simple_getattr,
};

const struct file_operations vtfs_dir_ops = {
  .owner = THIS_MODULE,
  .iterate_shared = vtfs_iterate,
};

const struct file_operations vtfs_file_ops = {
  .owner = THIS_MODULE,
  .read  = vtfs_read,
  .write = vtfs_write,
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

  if (S_ISDIR(mode)) {
      inode->i_op = &vtfs_inode_ops;
      inode->i_fop = &vtfs_dir_ops;
  } else if (S_ISREG(mode)) {
      inode->i_op = &vtfs_file_inode_ops;
      inode->i_fop = &vtfs_file_ops;
  }
  inode->i_ino = i_ino;
  inode->i_sb  = sb;
  inode->i_size = 0;

  return inode;
}

// заполняет структуру super_block инфой о файловой системе
int vtfs_fill_super(struct super_block *sb, void *data, int silent) {
  struct vtfs_sb_info *sbi;
  struct vtfs_entry *root_entry;
  struct inode *root_inode;

  sbi = kzalloc(sizeof(*sbi), GFP_KERNEL);
  if (!sbi)
      return -ENOMEM;

  // Создаем корневую директорию
  root_entry = kzalloc(sizeof(*root_entry), GFP_KERNEL);
  if (!root_entry) {
      kfree(sbi);
      return -ENOMEM;
  }

  strcpy(root_entry->name, "/");
  root_entry->mode = S_IFDIR | 0777;
  root_entry->ino = 1000;
  root_entry->parent = NULL;
  root_entry->data = NULL;
  root_entry->size = 0;
  root_entry->target = NULL;
  INIT_LIST_HEAD(&root_entry->children);
  atomic_set(&root_entry->refcount, 1);

  sbi->root_entry = root_entry;
  sb->s_fs_info = sbi;

  root_inode = vtfs_get_inode(sb, NULL, S_IFDIR | 0777, 1000);
  if (!root_inode) {
      kfree(root_entry);
      kfree(sbi);
      return -ENOMEM;
  }
  root_inode->i_private = root_entry;
  
  sb->s_root = d_make_root(root_inode);
  if (sb->s_root == NULL) {
      kfree(root_entry);
      kfree(sbi);
      return -ENOMEM;
  }
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