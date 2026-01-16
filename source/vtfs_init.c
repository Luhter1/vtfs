#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include "vtfs.h"
#include "fs.h"

const struct inode_operations vtfs_inode_ops = {
  .lookup = vtfs_lookup,
  .create = vtfs_create,
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
      inode->i_op = NULL;
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
  struct inode *root_inode;

  sbi = kzalloc(sizeof(*sbi), GFP_KERNEL);
  if (!sbi)
      return -ENOMEM;

  INIT_LIST_HEAD(&sbi->entries);

  // struct vtfs_entry *e = kzalloc(sizeof(*e), GFP_KERNEL);
  // strcpy(e->name, "test.txt");
  // e->mode = S_IFREG | 0755;
  // e->ino  = 1001;
  // list_add(&e->list, &sbi->entries);

  sb->s_fs_info = sbi;

  root_inode = vtfs_get_inode(sb, NULL, S_IFDIR | 0755, 1000);
  sb->s_root = d_make_root(root_inode);
  if (sb->s_root == NULL) {
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