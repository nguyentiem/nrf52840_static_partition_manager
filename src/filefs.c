/********************************************************************
 * COPYRIGHT (C) 2024 Assa Abloy. All rights reserved.
 *
 * This file is part of Havasu and is proprietary to Assa Abloy.
 *
 * No part of this file may be copied, reproduced, modified, published,
 * uploaded, posted, transmitted, or distributed in any way, without
 * the prior written permission of Assa Abloy.
 *
 * Created by: FPT Software. Date: Feb 01, 2024
 ********************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include "filefs.h"
#include <zephyr/drivers/flash.h>
#include <zephyr/devicetree.h>
#include <stdio.h>
#include <string.h>

#define FILE_ACCESS_MUTEX_TIMEOUT_MS 1000

LOG_MODULE_DECLARE(app, 4);

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t lfs_storage_mnt = {
    .type = FS_LITTLEFS,
    .fs_data = &storage,
    .storage_dev = (void *)FIXED_PARTITION_ID(littlefs_storage),
    .mnt_point =  DISK_MOUNT_PT,
};
struct fs_mount_t *mountpoint = &lfs_storage_mnt;

static int littlefs_flash_erase(unsigned int id);
static bool  fs_create_all_file();
static int get_file_infor(const char *fileName);
static int  disk_deinit();
static int  disk_init();

static int littlefs_mount(struct fs_mount_t *mp)
{
  int rc;

  rc = littlefs_flash_erase((uintptr_t)mp->storage_dev);
  if (rc < 0)
  {
    return rc;
  }

  rc = fs_mount(mp);
  if (rc < 0)
  {
    LOG_PRINTK("FAIL: mount id %" PRIuPTR " at %s: %d\n",
               (uintptr_t)mp->storage_dev, mp->mnt_point, rc);
    return rc;
  }
  LOG_PRINTK("%s mount: %d\n", mp->mnt_point, rc);

  return 0;
}

static int lsdir(const char *path)
{
  int res;
  struct fs_dir_t dirp;
  static struct fs_dirent entry;

  fs_dir_t_init(&dirp);

  /* Verify fs_opendir() */
  res = fs_opendir(&dirp, path);
  if (res)
  {
    LOG_ERR("Error opening dir %s [%d]\n", path, res);
    return res;
  }

  LOG_PRINTK("\nListing dir %s ...\n", path);
  for (;;)
  {
    /* Verify fs_readdir() */
    res = fs_readdir(&dirp, &entry);

    /* entry.name[0] == 0 means end-of-dir */
    if (res || entry.name[0] == 0)
    {
      if (res < 0)
      {
        LOG_ERR("Error reading dir [%d]\n", res);
      }
      break;
    }

    if (entry.type == FS_DIR_ENTRY_DIR)
    {
      LOG_PRINTK("[DIR ] %s\n", entry.name);
    }
    else
    {
      LOG_PRINTK("[FILE] %s (size = %zu)\n", entry.name, entry.size);
    }
  }

  /* Verify fs_closedir() */
  fs_closedir(&dirp);

  return res;
}

static int littlefs_flash_erase(unsigned int id)
{
  const struct flash_area *pfa;
  int rc;

  rc = flash_area_open(id, &pfa);
  if (rc < 0)
  {
    LOG_ERR("FAIL: unable to find flash area %u: %d\n", id, rc);
    return rc;
  }

  LOG_PRINTK("<FS>Area %u at 0x%x on %s for %u bytes\n",
             id,
             (unsigned int)pfa->fa_off,
             pfa->fa_dev->name,
             (unsigned int)pfa->fa_size);

  /* Optional wipe flash contents */
  if (IS_ENABLED(CONFIG_APP_WIPE_STORAGE))
  {
    rc = flash_area_erase(pfa, 0, pfa->fa_size);
    LOG_ERR("Erasing flash area ... %d", rc);
  }

  flash_area_close(pfa);
  return rc;
}

static uint8_t  fs_file_exist(const char *full_path_file);

bool  flash_earse_region(off_t region_offset, size_t sector_size)
{
  const struct device *flash_dev = DEVICE_DT_GET(DT_ALIAS(spi_flash0));

  if (!device_is_ready(flash_dev))
  {
    LOG_ERR("%s: device not ready.\n", flash_dev->name);
    return false;
  }
  int rc = flash_erase(flash_dev, region_offset, sector_size);
  if (rc != 0)
  {
    return false;
  }
  else
  {
  }
  return true;
}

static bool  fs_create_all_file()
{
  return  fs_create_file( USER_INFOR_FULL_PATH, USER_FILE_SIZE);
}

int format_disk_and_create_file()
{
   disk_deinit();
  if (! flash_earse_region(SPI_FLASH_FS_REGION_OFFSET, SPI_FLASH_FS_SECTOR_SIZE))
  {
    LOG_ERR("FS-INIT:Error format flash");
    
    return -1;
  }
   disk_init();
  LOG_INF("<FS> Format disk successfully");
  bool flag =  fs_create_all_file();
  if (flag != true)
  {
    return -3;
  }
  return 0;
}

static int  disk_deinit()
{
  return fs_unmount(mountpoint);
}

static int  disk_init()
{
  struct fs_statvfs sbuf;
  int rc;
  rc = littlefs_mount(mountpoint);
  if (rc < 0)
  {
    return -1;
  }
  rc = fs_statvfs(mountpoint->mnt_point, &sbuf);
  if (rc < 0)
  {
    LOG_INF("<FS> FAIL: statvfs: %d\n", rc);
    return 0;
  }

  LOG_INF("<FS> Littlefs infor: %s: bsize = %lu ; frsize = %lu ;"
          " blocks = %lu ; bfree = %lu\n",
          mountpoint->mnt_point,
          sbuf.f_bsize,
          sbuf.f_frsize,
          sbuf.f_blocks,
          sbuf.f_bfree);
  LOG_INF("<FS> Listing file in littlefs Paritition %s", mountpoint->mnt_point);
  rc = lsdir(mountpoint->mnt_point);
  if (rc < 0)
  {
    LOG_INF("FAIL: lsdir %s: %d\n", mountpoint->mnt_point, rc);
    return 0;
  }
  return 0;
}

bool  fs_create_file(const char *full_path_file, size_t size_file)
{
  bool res = false;
  LOG_INF("<FS> %s File %s size %d", __func__, full_path_file, size_file);

 

  struct fs_file_t file;
  fs_file_t_init(&file);
  if (fs_open(&file, full_path_file, FS_O_CREATE | FS_O_RDWR) != 0)
  {
    LOG_ERR("FS-Create File-ERR: create file %s", full_path_file);
   
    return res;
  }

  if (fs_truncate(&file, 0) != 0)
  {
    LOG_ERR("Failed to shirk file");
    res = -1;
    goto out;
  }

  if (fs_truncate(&file, size_file) != 0)
  {
    LOG_ERR("Failed to extend file to: %d bytes", size_file);
    res = -1;
    goto out;
  }
  res = 0;
out:
  fs_close(&file);
  return res == 0;
}

int  fs_read_file(const char *disk, const char *full_path_file, void *buff, size_t len)
{
  int res = 0;
  struct fs_file_t file;

  fs_file_t_init(&file);
  res = fs_open(&file, full_path_file, FS_O_READ);
  if (res != 0)
  {
    LOG_ERR("Failed to open file %s Err %d", full_path_file, res);
    return res;
  }
  res = fs_read(&file, buff, len);
  if (res < 0)
  {
    LOG_ERR("Error read file %s", full_path_file);
  }
  if (res != len)
  {
    LOG_ERR("Error len file %d bytes, you read %d bytes", res, len);
    res = 0;
  }
  fs_close(&file);
  return res;
}

int  fs_write_file(const char *disk, const char *full_path_file, void *buff, size_t len)
{
  int res = 0;
  struct fs_file_t file;
  
  fs_file_t_init(&file);
  res = fs_open(&file, full_path_file, FS_O_APPEND | FS_O_WRITE);
  if (res != 0)
  {
    LOG_ERR("Failed to open file %s err %d\n", full_path_file, res);
   
    return res;
  }

  res = fs_write(&file, buff, len);
  if (res < 0 || res != len)
  {
    LOG_ERR("Error write file %s , ret: %d\n", full_path_file, res);
    res = -1;
  }
  fs_close(&file);
  return res;
}

int  fs_write_file_index(const char *disk, const char *full_path_file, void *buff, size_t len, size_t index)
{
  int res = 0;
  struct fs_file_t file;
  fs_file_t_init(&file);
  res = fs_open(&file, full_path_file, FS_O_WRITE);
  if (res != 0)
  {
    LOG_ERR("Failed to open file %s err %d\n", full_path_file, res);
    return res;
  }
  res = fs_seek(&file, index, FS_SEEK_SET);
  if (res != 0)
  {
    LOG_ERR("Failed to seek file %s", full_path_file);
    res = -1;
    goto out;
  }
  res = fs_write(&file, buff, len);
  if (res < 0 || res != len)
  {
    LOG_ERR("Error write file %s", full_path_file);
    res = -1;
    goto out;
  }
  fs_sync(&file);
out:
  fs_close(&file);
 
  return res;
}

int  fs_read_file_index(const char *disk, const char *full_path_file, void *buff, size_t len, size_t index)
{
  int res = 0;
  struct fs_file_t file;
  fs_file_t_init(&file);
  res = fs_open(&file, full_path_file, FS_O_READ);
  if (res != 0)
  {
    LOG_ERR("Failed to open file %s error %d\n", full_path_file, res);
    return res;
  }
  res = fs_seek(&file, index, FS_SEEK_SET);
  if (res != 0)
  {
    LOG_ERR("Failed to seek file %s \n", full_path_file);
    res = -1;
    goto out;
  }
  res = fs_read(&file, buff, len);
  if (res < 0)
  {
    LOG_ERR("Error read file %s\n", full_path_file);
    res = -1;
    goto out;
  }

  if (res != len)
  {
    LOG_ERR("Error len file %d bytes, you read %d bytes, index %d\n", res, len, index);
    res = -1;
    goto out;
  }
out:
  fs_close(&file);
  return res;
}

static uint8_t  fs_file_exist(const char *full_path_file)
{
  // LOG_INF("<FS> %s %s", __func__, full_path_file);
  int res = 0;
  struct fs_file_t file;
  uint8_t rs = 0;

  fs_file_t_init(&file);
  res = fs_open(&file, full_path_file, FS_O_READ);
  if (res == 0)
  {
    fs_close(&file);
    LOG_INF("<FS> File %s exist", full_path_file);
    rs = FILE_EXIST;
    goto exit;
  }

  if (res == -ENOENT)
  {
    rs = FILE_NOT_EXIST;
    LOG_INF("<FS> File %s not exist", full_path_file);
    goto exit;
  }
  rs = FILE_ERROR;
exit:
  return rs;
}

static bool  fs_create_file_if_invalid()
{
  if ( fs_file_exist( USER_INFOR_FULL_PATH) == FILE_NOT_EXIST ||
      get_file_infor( USER_INFOR_FULL_PATH) != USER_FILE_SIZE )
  {
    return format_disk_and_create_file() == 0;
  }
  return true;
}

/*return file size*/
static int get_file_infor(const char *fileName)
{
  struct fs_file_t file;
  struct fs_dirent entry;
  int size = 0;
  fs_file_t_init(&file);
  int res = fs_open(&file, fileName, FS_O_READ);
  if (res != 0)
  {
    LOG_ERR("Failed to open file %s error %d\n", fileName, res);
    return -1;
  }
  if (fs_stat(fileName, &entry) != 0)
  {
    LOG_ERR("Fail to get file infor %s", fileName);
    fs_close(&file);
    return -1;
  }
  size = entry.size;
  LOG_INF("<FS> Check File %s size %d", fileName, entry.size);
  fs_close(&file);
  return size;
}

int  fs_clear_file(const char *full_path_file, int size)
{
  int res = 0;
  uint8_t buff[128];
  struct fs_file_t file;
  memset(buff, 0, sizeof(buff));
  fs_file_t_init(&file);
  res = fs_open(&file, full_path_file, FS_O_WRITE);
  if (res != 0)
  {
    LOG_ERR("Failed to open file %s error %d\n", full_path_file, res);
    return res;
  }
  res = fs_seek(&file, 0, FS_SEEK_SET);
  if (res != 0)
  {
    LOG_ERR("Failed to seek file %s \n", full_path_file);
    res = -1;
    goto out;
  }
  int write_size = 0;
  while (size > 0)
  {
    write_size = sizeof(buff) > size ? size : sizeof(buff);
    res = fs_write(&file, buff, write_size);
    if (res < 0 || res != write_size)
    {
      LOG_ERR("Error read file %s\n", full_path_file);
      res = -1;
      goto out;
    }
    size -= write_size;
  }

out:
  fs_close(&file);
  return res;
}

bool  file_system_init()
{
  /*check file*/
   disk_init();
  LOG_INF("Init file");
  return  fs_create_file_if_invalid();
}

