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
        inode->i_nlink = atomic_read(&entry->refcount);
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
  new_entry->target = NULL; // Оригинальный файл
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
  inode->i_nlink = 1;

  d_add(child_dentry, inode);

  return 0;
}

// Удаление файла (unlink)
int vtfs_unlink(
  struct inode *parent_inode,
  struct dentry *child_dentry
) {
  struct vtfs_entry *parent_entry = parent_inode->i_private;
  struct vtfs_entry *entry, *target_entry;
  struct inode *inode = d_inode(child_dentry);
  const char *name = child_dentry->d_name.name;

  printk(KERN_INFO "vtfs_unlink called for %s\n", name);

  if (!parent_entry)
    return -ENOENT;

  // Поиск файла в children
  list_for_each_entry(entry, &parent_entry->children, list) {
    if (strcmp(entry->name, name) == 0) {
      // Определяем target_entry (оригинальный файл)
      target_entry = entry->target ? entry->target : entry;
      
      // Уменьшаем refcount оригинального файла
      if (atomic_dec_and_test(&target_entry->refcount)) {
        // Если refcount == 0, удаляем данные
        if (target_entry->data)
          kfree(target_entry->data);
        printk(KERN_INFO "vtfs_unlink: data for %s freed (refcount=0)\n", name);
      } else {
        printk(KERN_INFO "vtfs_unlink: removed link to %s (refcount=%d)\n",
               name, atomic_read(&target_entry->refcount));
      }
      
      // Удаляем entry из parent->children
      list_del(&entry->list);
      
      // Освобождаем структуру entry (но не data, если это link)
      kfree(entry);
      
      if (inode) {
        drop_nlink(inode);
        inode->i_ctime = parent_inode->i_ctime = current_time(inode);
      }
      
      return 0;
    }
  }

  return -ENOENT;
}

// Создание директории
int vtfs_mkdir(
  struct mnt_idmap *id,
  struct inode *parent_inode,
  struct dentry *child_dentry,
  umode_t mode
) {
  struct vtfs_entry *parent_entry = parent_inode->i_private;
  struct vtfs_entry *entry, *new_entry;
  struct inode *inode;

  printk(KERN_INFO "vtfs_mkdir called for %s\n", child_dentry->d_name.name);

  if (!S_ISDIR(parent_inode->i_mode))
    return -ENOTDIR;

  if (!parent_entry)
    return -ENOENT;

  // Проверка на существование директории с таким же именем
  list_for_each_entry(entry, &parent_entry->children, list) {
    if (strcmp(entry->name, child_dentry->d_name.name) == 0)
      return -EEXIST;
  }

  new_entry = kzalloc(sizeof(*new_entry), GFP_KERNEL);
  if (!new_entry)
    return -ENOMEM;

  strncpy(new_entry->name, child_dentry->d_name.name, MAX_NAME_LEN - 1);
  new_entry->name[MAX_NAME_LEN - 1] = '\0';
  new_entry->mode = S_IFDIR | mode;
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
  inode->i_nlink = 2; // . и parent

  inc_nlink(parent_inode); // parent получает ссылку от .. в новой директории
  d_add(child_dentry, inode);

  return 0;
}

// Удаление пустой директории
int vtfs_rmdir(
  struct inode *parent_inode,
  struct dentry *child_dentry
) {
  struct vtfs_entry *parent_entry = parent_inode->i_private;
  struct vtfs_entry *entry;
  struct inode *inode = d_inode(child_dentry);
  const char *name = child_dentry->d_name.name;

  printk(KERN_INFO "vtfs_rmdir called for %s\n", name);

  if (!parent_entry)
    return -ENOENT;

  // Поиск директории в children
  list_for_each_entry(entry, &parent_entry->children, list) {
    if (strcmp(entry->name, name) == 0) {
      // Проверка, что директория пуста
      if (!list_empty(&entry->children))
        return -ENOTEMPTY;

      list_del(&entry->list);
      kfree(entry);

      if (inode) {
        drop_nlink(inode);
        drop_nlink(parent_inode); // удаляем ссылку от ..
        inode->i_ctime = parent_inode->i_ctime = current_time(inode);
      }

      printk(KERN_INFO "vtfs_rmdir: directory %s removed\n", name);
      return 0;
    }
  }

  return -ENOENT;
}

// Создание жесткой ссылки
int vtfs_link(
  struct dentry *old_dentry,
  struct inode *parent_inode,
  struct dentry *new_dentry
) {
  struct inode *inode = d_inode(old_dentry);
  struct vtfs_entry *orig_entry = inode->i_private;
  struct vtfs_entry *parent_entry = parent_inode->i_private;
  struct vtfs_entry *check_entry, *link_entry;

  printk(KERN_INFO "vtfs_link: creating link from %s to %s\n",
         old_dentry->d_name.name, new_dentry->d_name.name);

  // Жесткие ссылки только на файлы, не на директории
  if (S_ISDIR(inode->i_mode))
    return -EPERM;

  if (!orig_entry || !parent_entry)
    return -ENOENT;

  // Проверка на существование файла с таким же именем
  list_for_each_entry(check_entry, &parent_entry->children, list) {
    if (strcmp(check_entry->name, new_dentry->d_name.name) == 0)
      return -EEXIST;
  }

  // Создаем новую entry для hard link (легкая копия с новым именем)
  link_entry = kzalloc(sizeof(*link_entry), GFP_KERNEL);
  if (!link_entry)
    return -ENOMEM;

  strncpy(link_entry->name, new_dentry->d_name.name, MAX_NAME_LEN - 1);
  link_entry->name[MAX_NAME_LEN - 1] = '\0';
  link_entry->mode = orig_entry->mode;
  link_entry->ino = orig_entry->ino;  // Тот же ino!
  link_entry->parent = parent_entry;
  link_entry->data = orig_entry->data;  // Разделяем данные
  link_entry->size = orig_entry->size;
  INIT_LIST_HEAD(&link_entry->children);
  // refcount не копируем, используем общий из orig_entry

  // Увеличиваем refcount оригинальной entry
  atomic_inc(&orig_entry->refcount);
  
  // Добавляем link_entry в parent директорию
  list_add(&link_entry->list, &parent_entry->children);

  inc_nlink(inode);
  inode->i_ctime = current_time(inode);
  ihold(inode); // увеличиваем счетчик inode
  d_add(new_dentry, inode);

  printk(KERN_INFO "vtfs_link: link created, refcount=%d\n",
         atomic_read(&orig_entry->refcount));

  return 0;
}