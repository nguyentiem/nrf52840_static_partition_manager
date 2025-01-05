#ifndef _FILE_SYSTEM_H_
#define _FILE_SYSTEM_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include <stddef.h>
#include <zephyr/fs/fs_interface.h>

#define  DISK_MOUNT_PT "/lfs"

#define SPI_FLASH_FS_REGION_OFFSET 0x00000
#define SPI_FLASH_FS_SECTOR_SIZE 0x170000

/* User file system config */
#define  MAX_USER_SUPORT 256
#define  USER_INFOR_FILE_NAME "userinfo.dat"
#define  USER_INFOR_FULL_PATH  DISK_MOUNT_PT "/"  USER_INFOR_FILE_NAME
#define USER_FILE_SIZE 35328


  enum file_status
  {
    FILE_ERROR = 0x00,
    FILE_EXIST,
    FILE_NOT_EXIST,
  };

  bool  fs_create_file(const char *full_path_file, size_t size_file);

  int  fs_write_file(const char *disk, const char *full_path_file, void *buff, size_t len);

  int  fs_read_file(const char *disk, const char *full_path_file, void *buff, size_t len);

  int  fs_write_file_index(const char *disk, const char *full_path_file, void *buff, size_t len, size_t index);

  int  fs_read_file_index(const char *disk, const char *full_path_file, void *buff, size_t len, size_t index);

  int format_disk_and_create_file();

  bool  flash_earse_region(off_t region_offset, size_t sector_size);

  // int lsdir(const char *disk, const char *path);


  bool  file_system_init();

  // int  disk_init();
  int  fs_clear_file(const char *full_path_file, int size);

 #ifdef __cplusplus
}
#endif
#endif /*_FILE_SYSTEM_H_*/