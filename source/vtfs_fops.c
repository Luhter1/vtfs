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
  struct vtfs_sb_info *sbi = inode->i_sb->s_fs_info;
  loff_t pos = ctx->pos;
  int i = 0;

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

  list_for_each_entry(entry, &sbi->entries, list){
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