#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include "vtfs.h"
#include "fs.h"

// filp - указатель на открытый каталог
// функция поочередно вызывается для каждого файла (увеличивая ctx->pos)
int vtfs_iterate(struct file *filp, struct dir_context *ctx) {
  struct vtfs_entry *entry;
  struct inode *inode = file_inode(filp);
  struct vtfs_entry *dir_entry = inode->i_private;
  loff_t pos = ctx->pos;
  int i = 0;

  if (!dir_entry)
    return -ENOENT;

  if(ctx->pos == 0) {
    if(!dir_emit(ctx, ".", 1, inode->i_ino, DT_DIR))
      return 0;

    ctx->pos++;
    pos++;
  }

  if(ctx->pos == 1) {
    ino_t pino = filp->f_path.dentry->d_parent->d_inode->i_ino;

    if(!dir_emit(ctx, "..", 2, pino, DT_DIR))
      return 0;

    ctx->pos++;
    pos++;
  }

  // Итерируем по children текущей директории
  list_for_each_entry(entry, &dir_entry->children, list){
    if(i++ < pos - 2)
        continue;

    if(!dir_emit(ctx,
          entry->name,
          strlen(entry->name),
          entry->ino,
          S_ISDIR(entry->mode) ? DT_DIR : DT_REG))
      return 0;

    ctx->pos++;
  }

  return 0;
}

ssize_t vtfs_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos) {
  struct inode *inode = file_inode(filp);
  struct vtfs_entry *entry = inode->i_private;
  ssize_t ret;

  if (!entry || !entry->data)
    return 0;

  if (*ppos >= entry->size)
    return 0;

  if (count > entry->size - *ppos)
    count = entry->size - *ppos;

  if (copy_to_user(buf, entry->data + *ppos, count))
    return -EFAULT;

  *ppos += count;
  inode->i_size = entry->size;
  ret = count;
  return ret;
}

ssize_t vtfs_write(
  struct file *filp,
  const char __user *buf,
  size_t count, 
  loff_t *ppos
){
  struct inode *inode = file_inode(filp);
  struct vtfs_entry *entry = inode->i_private;
  char *new_data;

  if (!entry)
    return -EIO;


  if (!entry->data) {
    entry->data = kzalloc(*ppos + count, GFP_KERNEL);
    if (!entry->data)
      return -ENOMEM;
  } else if (*ppos + count > entry->size) {
    new_data = krealloc(entry->data, *ppos + count, GFP_KERNEL);
    if (!new_data)
      return -ENOMEM;
    entry->data = new_data;
  }

  if (copy_from_user(entry->data + *ppos, buf, count))
    return -EFAULT;

  *ppos += count;
  if (*ppos > entry->size)
    entry->size = *ppos;
  inode->i_size = entry->size;

  return count;
}