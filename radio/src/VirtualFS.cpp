/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <stdarg.h>
#include <string.h>

#include "strhelpers.h"
#if !defined(BOOT)
#include "opentx.h"
#endif
#include "VirtualFS.h"
#include "board.h"
#include "audio.h"
#include "disk_cache.h"
#include "debug.h"

#if defined(LIBOPENUI) && 0
  #include "libopenui.h"
#endif

VirtualFS* VirtualFS::_instance = nullptr;;

#if defined(SDCARD)
static FATFS sdFatFs __DMA;    // initialized in boardInit()

#if defined(LOG_TELEMETRY)
VfsFile g_telemetryFile = {};
#endif

#if defined(LOG_BLUETOOTH)
VfsFile g_bluetoothFile = {};
#endif
#endif

#if defined(SPI_FLASH) && !defined(USE_LITTLEFS)
static FATFS spiFatFs __DMA;
#endif

#if defined(USE_LITTLEFS)
size_t flashSpiRead(size_t address, uint8_t* data, size_t size);
size_t flashSpiWrite(size_t address, const uint8_t* data, size_t size);
uint16_t flashSpiGetPageSize();
uint16_t flashSpiGetSectorSize();
uint16_t flashSpiGetSectorCount();

int flashSpiErase(size_t address);
int flashSpiBlockErase(size_t address);
void flashSpiEraseAll();

void flashSpiSync();


extern "C"
{

#ifdef LFS_THREADSAFE
// Lock the underlying block device. Negative error codes
// are propogated to the user.
int (*lock)(const struct lfs_config *c);

// Unlock the underlying block device. Negative error codes
// are propogated to the user.
int (*unlock)(const struct lfs_config *c);
#endif

int flashRead(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
  flashSpiRead((block * c->block_size) + off, (uint8_t*)buffer, size);
  return LFS_ERR_OK;
}

int flashWrite(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size)
{
  flashSpiWrite((block * c->block_size) + off, (uint8_t*)buffer, size);
  return LFS_ERR_OK;
}

int flashErase(const struct lfs_config *c, lfs_block_t block)
{
    flashSpiErase(block * c->block_size);
    return LFS_ERR_OK;
}

int flashSync(const struct lfs_config *c)
{
  flashSpiSync();
  return LFS_ERR_OK;
}
}

static VfsError convertResult(lfs_error err)
{
  switch(err)
  {
  case LFS_ERR_OK:          return VfsError::OK;
  case LFS_ERR_IO:          return VfsError::IO;
  case LFS_ERR_CORRUPT:     return VfsError::CORRUPT;
  case LFS_ERR_NOENT:       return VfsError::NOENT;
  case LFS_ERR_EXIST:       return VfsError::EXIST;
  case LFS_ERR_NOTDIR:      return VfsError::NOTDIR;
  case LFS_ERR_ISDIR:       return VfsError::ISDIR;
  case LFS_ERR_NOTEMPTY:    return VfsError::NOTEMPTY;
  case LFS_ERR_BADF:        return VfsError::BADF;
  case LFS_ERR_FBIG:        return VfsError::FBIG;
  case LFS_ERR_INVAL:       return VfsError::INVAL;
  case LFS_ERR_NOSPC:       return VfsError::NOSPC;
  case LFS_ERR_NOMEM:       return VfsError::NOMEM;
  case LFS_ERR_NOATTR:      return VfsError::NOATTR;
  case LFS_ERR_NAMETOOLONG: return VfsError::NAMETOOLONG;
  }
  return VfsError::INVAL;
}

static int convertOpenFlagsToLfs(VfsOpenFlags flags)
{
  int lfsFlags = 0;

  if((flags & VfsOpenFlags::READ) != VfsOpenFlags::NONE && (flags & VfsOpenFlags::WRITE) != VfsOpenFlags::NONE)
    lfsFlags |= LFS_O_RDWR;
  else if ((flags & VfsOpenFlags::READ) != VfsOpenFlags::NONE)
    lfsFlags |= LFS_O_RDONLY;
  else if ((flags & VfsOpenFlags::WRITE) != VfsOpenFlags::NONE)
    lfsFlags |= LFS_O_WRONLY;

  if((flags & VfsOpenFlags::CREATE_NEW) != VfsOpenFlags::NONE)
    lfsFlags |= LFS_O_CREAT | LFS_O_EXCL;
  else if ((flags & VfsOpenFlags::CREATE_ALWAYS) != VfsOpenFlags::NONE)
    lfsFlags |= LFS_O_CREAT | LFS_O_TRUNC;
  else if((flags & VfsOpenFlags::OPEN_ALWAYS) != VfsOpenFlags::NONE)
    lfsFlags |= LFS_O_CREAT;
  else if((flags & VfsOpenFlags::OPEN_APPEND) != VfsOpenFlags::NONE)
    lfsFlags |= LFS_O_CREAT | LFS_O_APPEND;

  return lfsFlags;
}
#endif

#if defined (USE_FATFS)
static VfsError convertResult(FRESULT err)
{
  switch(err)
  {
  case FR_OK:                  return VfsError::OK;
  case FR_DISK_ERR:            return VfsError::IO;
  case FR_INT_ERR:             return VfsError::INVAL;
  case FR_NOT_READY:           return VfsError::NOT_READY;
  case FR_NO_FILE:             return VfsError::NOENT;
  case FR_NO_PATH:             return VfsError::NOENT;
  case FR_INVALID_NAME:        return VfsError::INVAL;
  case FR_DENIED:              return VfsError::INVAL;
  case FR_EXIST:               return VfsError::EXIST;
  case FR_INVALID_OBJECT:      return VfsError::BADF;
  case FR_WRITE_PROTECTED:     return VfsError::INVAL;
  case FR_INVALID_DRIVE:       return VfsError::INVAL;
  case FR_NOT_ENABLED:         return VfsError::INVAL;
  case FR_NO_FILESYSTEM:       return VfsError::INVAL;
  case FR_MKFS_ABORTED:        return VfsError::INVAL;
  case FR_TIMEOUT:             return VfsError::INVAL;
  case FR_LOCKED:              return VfsError::INVAL;
  case FR_NOT_ENOUGH_CORE:     return VfsError::INVAL;
  case FR_TOO_MANY_OPEN_FILES: return VfsError::INVAL;
  case FR_INVALID_PARAMETER:   return VfsError::INVAL;
  }
  return VfsError::INVAL;
}

static int convertOpenFlagsToFat(VfsOpenFlags flags)
{
  return (int)flags;
}
#endif

VfsOpenFlags operator|(VfsOpenFlags lhs,VfsOpenFlags rhs)
{
  typedef typename
    std::underlying_type<VfsOpenFlags>::type underlying;
  return static_cast<VfsOpenFlags>(
    static_cast<underlying>(lhs)
  | static_cast<underlying>(rhs));
}
VfsOpenFlags operator|=(VfsOpenFlags lhs,VfsOpenFlags rhs)
{
  lhs = lhs|rhs;
  return lhs;
}
VfsOpenFlags operator&(VfsOpenFlags lhs,VfsOpenFlags rhs)
{
  typedef typename
    std::underlying_type<VfsOpenFlags>::type underlying;
  return static_cast<VfsOpenFlags>(
    static_cast<underlying>(lhs)
  & static_cast<underlying>(rhs));
}

VfsFileAttributes operator|(VfsFileAttributes lhs,VfsFileAttributes rhs)
{
  typedef typename
    std::underlying_type<VfsFileAttributes>::type underlying;
  return static_cast<VfsFileAttributes>(
    static_cast<underlying>(lhs)
  | static_cast<underlying>(rhs));
}
VfsFileAttributes operator&(VfsFileAttributes lhs,VfsFileAttributes rhs)
{
  typedef typename
    std::underlying_type<VfsFileAttributes>::type underlying;
  return static_cast<VfsFileAttributes>(
    static_cast<underlying>(lhs)
  & static_cast<underlying>(rhs));
}

const char* VfsFileInfo::getName() const
{
  switch(type)
  {
  case VfsFileType::ROOT: return name;
#if defined (USE_FATFS)
  case VfsFileType::FAT:  return(name != nullptr)?name:fatInfo.fname;
#endif
#if defined(USE_LITTLEFS)
  case VfsFileType::LFS:  return lfsInfo.name;
#endif
  }
  return "";
};

size_t VfsFileInfo::getSize() const
{
  switch(type)
  {
  case VfsFileType::ROOT: return 0;
#if defined (USE_FATFS)
  case VfsFileType::FAT:  return fatInfo.fsize;
#endif
#if defined(USE_LITTLEFS)
  case VfsFileType::LFS:  return lfsInfo.size;
#endif
  }
  return 0;
}

VfsType VfsFileInfo::getType() const
{
  switch(type)
  {
  case VfsFileType::ROOT:
    return VfsType::DIR;
#if defined (USE_FATFS)
  case VfsFileType::FAT:
    if (name != nullptr)
      return VfsType::DIR;
    if(fatInfo.fattrib & AM_DIR)
      return VfsType::DIR;
    else
      return VfsType::FILE;
#endif
#if defined(USE_LITTLEFS)
  case VfsFileType::LFS:

    if(lfsInfo.type == LFS_TYPE_DIR)
      return VfsType::DIR;
    else
      return VfsType::FILE;
#endif
  }
  return VfsType::UNKOWN;
};

VfsFileAttributes VfsFileInfo::getAttrib()
{
  switch(type)
  {
  case VfsFileType::ROOT:
    return VfsFileAttributes::DIR;
#if defined (USE_FATFS)
  case VfsFileType::FAT:
    return (VfsFileAttributes)fatInfo.fattrib;
#endif
#if defined(USE_LITTLEFS)
  case VfsFileType::LFS:
    if(lfsInfo.type == LFS_TYPE_DIR)
      return VfsFileAttributes::DIR;
    return VfsFileAttributes::NONE;
#endif
  }
  return VfsFileAttributes::NONE;
}

int VfsFileInfo::getDate(){
  switch(type)
  {
  case VfsFileType::ROOT:
    return 0;
#if defined (USE_FATFS)
  case VfsFileType::FAT:
    return fatInfo.fdate;
#endif
#if defined(USE_LITTLEFS)
  case VfsFileType::LFS:
    return 0;
#endif
  }
  return 0;
}

int VfsFileInfo::getTime()
{
  switch(type)
  {
  case VfsFileType::ROOT:
    return 0;
#if defined (USE_FATFS)
  case VfsFileType::FAT:
    return fatInfo.ftime;
#endif
#if defined(USE_LITTLEFS)
  case VfsFileType::LFS:
    return 0;
#endif
  }
  return 0;
}

void VfsFileInfo::clear() {
  type = VfsFileType::UNKNOWN;
#if defined(USE_LITTLEFS)
  lfsInfo = {0};
#endif
  fatInfo = {0};
  name = nullptr;
}


void VfsDir::clear()
{
  type = DIR_UNKNOWN;
#if defined(USE_LITTLEFS)
  lfs.dir = {0};
  lfs.handle = nullptr;
#endif
  fat.dir = {0};

  readIdx = 0;
}

VfsError VfsDir::read(VfsFileInfo& info)
{
  info.clear();
  switch(type)
  {
  case VfsDir::DIR_ROOT:
    info.type = VfsFileType::ROOT;
#if defined(SPI_FLASH)
    if(readIdx == 0)
      info.name = "INTERNAL";
#if defined(SDCARD)
    else if(readIdx == 1)
      info.name = "SDCARD";
#endif
    else
      info.name = "";
#elif defined(SDCARD) // SPI_FLASH
    if(readIdx == 0)
      info.name = "SDCARD";
    else
      info.name = "";
#endif // SDCARD
    readIdx++;
    return VfsError::OK;
#if defined (USE_FATFS)
  case VfsDir::DIR_FAT:
  {
    info.type = VfsFileType::FAT;
    if(readIdx == 0) // emulate ".." entry
    {
      readIdx++;
      info.name = "..";
      return VfsError::OK;
    }
    VfsError ret = convertResult(f_readdir(&fat.dir, &info.fatInfo));
    return ret;
  }
#endif
#if defined(USE_LITTLEFS)
  case VfsDir::DIR_LFS:
    {
      info.type = VfsFileType::LFS;
      int res = lfs_dir_read(lfs.handle, &lfs.dir, &info.lfsInfo);
      if(res >= 0)
        return VfsError::OK;
      return convertResult((lfs_error)res);
    }
#endif
  }
  return VfsError::INVAL;
}

VfsError VfsDir::close()
{
  VfsError ret = VfsError::INVAL;
  switch(type)
  {
  case VfsDir::DIR_ROOT:
    ret = VfsError::OK;
    break;
#if defined (USE_FATFS)
  case VfsDir::DIR_FAT:
    ret = convertResult(f_closedir(&fat.dir));
    break;
#endif
#if defined(USE_LITTLEFS)
  case VfsDir::DIR_LFS:
    ret = convertResult((lfs_error)lfs_dir_close(lfs.handle, &lfs.dir));
    break;
#endif
  }
  clear();
  return ret;
}

VfsError VfsDir::rewind()
{
  readIdx = 0;
  switch(type)
  {
  case VfsDir::DIR_ROOT:
    return VfsError::OK;
#if defined (USE_FATFS)
  case VfsDir::DIR_FAT:
  {
    return convertResult(f_readdir(&fat.dir, nullptr));
  }
#endif
#if defined(USE_LITTLEFS)
  case VfsDir::DIR_LFS:
    {
      info.type = VfsFileType::LFS;
      int res = lfs_dir_read(lfs.handle, &lfs.dir, nullptr);
      if(res >= 0)
        return VfsError::OK;
      return convertResult((lfs_error)res);
    }
#endif
  }
  return VfsError::INVAL;
}

void VfsFile::clear() {
  type = VfsFileType::UNKNOWN;
#if defined(USE_LITTLEFS)
  lfs.file = {0};
  lfs.handle = nullptr;
#endif
  fat.file = {0};
}

VfsError VfsFile::close()
{
  VfsError ret = VfsError::INVAL;
  switch(type)
  {
#if defined (USE_FATFS)
  case VfsFileType::FAT:
    ret = convertResult(f_close(&fat.file));
    break;
#endif
#if defined (USE_LITTLEFS)
  case VfsFileType::LFS:
    ret = convertResult((lfs_error)lfs_file_close(lfs.handle, &lfs.file));
    break;
#endif
  }

  clear();
  return ret;
}

int VfsFile::size()
{
  switch(type)
  {
#if defined (USE_FATFS)
  case VfsFileType::FAT:
    return f_size(&fat.file);
    break;
#endif
#if defined (USE_LITTLEFS)
  case VfsFileType::LFS:
    {
      int res = lfs_file_size(lfs.handle, &lfs.file);
      if(res < 0)
        return (int)convertResult((lfs_error)res);
      return res;
    }
#endif
  }

  return -1;
}

VfsError VfsFile::read(void* buf, size_t size, size_t& readSize)
{
  switch(type)
  {
#if defined (USE_FATFS)
  case VfsFileType::FAT: {
    UINT rd = 0;
    VfsError res = convertResult(f_read(&fat.file, buf, size, &rd));
    readSize = rd;
    return res;
  }
#endif
#if defined (USE_LITTLEFS)
  case VfsFileType::LFS:
    {
      int ret = lfs_file_read(lfs.handle, &lfs.file, buf, size);
      if(ret >= 0)
      {
        readSize = ret;
        return VfsError::OK;
      } else {
        readSize = 0;
        return convertResult((lfs_error)ret);
      }
      break;
    }
#endif
  }

  return VfsError::INVAL;
}

char* VfsFile::gets(char* buf, size_t maxLen)
{
  switch(type)
  {
#if defined (USE_FATFS)
  case VfsFileType::FAT:
    return f_gets(buf, maxLen, &fat.file);
#endif
#if defined (USE_LITTLEFS)
  case VfsFileType::LFS:
    {
      size_t nc = 0;
      char* p = buf;
      char s[4];
      size_t rc;
      char dc;
      maxLen -= 1;   /* Make a room for the terminator */
      while (nc < maxLen) {
          read(s, 1, rc);
          if (rc != 1) break;
          dc = s[0];
          *p++ = dc; nc++;
          if (dc == '\n') break;
      }
      *p = 0;     /* Terminate the string */
      return nc ? buf : 0;   /* When no data read due to EOF or error, return with error. */
    }
#endif
  }

  return 0;
}

#if !defined(BOOT)
VfsError VfsFile::write(const void* buf, size_t size, size_t& written)
{
  switch(type)
  {
#if defined (USE_FATFS)
  case VfsFileType::FAT: {
    UINT wrt = 0;
    VfsError res = convertResult(f_write(&fat.file, buf, size, &wrt));
    written = wrt;
    return res;
  }
#endif
#if defined (USE_LITTLEFS)
  case VfsFileType::LFS:
    {
      int ret = lfs_file_write(lfs.handle, &lfs.file, buf, size);
      if(ret >= 0)
      {
        written = ret;
        return VfsError::OK;
      } else {
        written = 0;
        return convertResult((lfs_error)ret);
      }
      break;
    }
#endif
  }

  return VfsError::INVAL;
}

VfsError VfsFile::puts(const std::string& str)
{
  size_t written;
  return this->write(str.data(), str.length(), written);
}

VfsError VfsFile::putc(char c)
{
  size_t written;
  return this->write(&c, 1, written);
}

int VfsFile::fprintf(const char* str, ...)
{
  switch(type)
  {
#if defined (USE_FATFS)
  case VfsFileType::FAT:
  {
      va_list args;
      va_start(args, str);

      int ret = f_printf(&fat.file, str, args);
      va_end(args);
      return ret;
  }
#endif
#if defined(USE_LITTLEFS)
  case VfsFileType::LFS:
  {
#if 0
      va_list arp;
         putbuff pb;
         BYTE f, r;
         UINT i, j, w;
         DWORD v;
         TCHAR c, d, str[32], *p;


         putc_init(&pb, fp);

         va_start(arp, fmt);

         for (;;) {
             c = *fmt++;
             if (c == 0) break;          /* End of string */
             if (c != '%') {             /* Non escape character */
                 putc_bfd(&pb, c);
                 continue;
             }
             w = f = 0;
             c = *fmt++;
             if (c == '0') {             /* Flag: '0' padding */
                 f = 1; c = *fmt++;
             } else {
                 if (c == '-') {         /* Flag: left justified */
                     f = 2; c = *fmt++;
                 }
             }
             if (c == '*') {             /* Minimum width by argument */
                 w = va_arg(arp, int);
                 c = *fmt++;
             } else {
                 while (IsDigit(c)) {    /* Minimum width */
                     w = w * 10 + c - '0';
                     c = *fmt++;
                 }
             }
             if (c == 'l' || c == 'L') { /* Type prefix: Size is long int */
                 f |= 4; c = *fmt++;
             }
             if (c == 0) break;
             d = c;
             if (IsLower(d)) d -= 0x20;
             switch (d) {                /* Atgument type is... */
             case 'S' :                  /* String */
                 p = va_arg(arp, TCHAR*);
                 for (j = 0; p[j]; j++) ;
                 if (!(f & 2)) {                     /* Right padded */
                     while (j++ < w) putc_bfd(&pb, ' ') ;
                 }
                 while (*p) putc_bfd(&pb, *p++) ;        /* String body */
                 while (j++ < w) putc_bfd(&pb, ' ') ;    /* Left padded */
                 continue;

             case 'C' :                  /* Character */
                 putc_bfd(&pb, (TCHAR)va_arg(arp, int)); continue;

             case 'B' :                  /* Unsigned binary */
                 r = 2; break;

             case 'O' :                  /* Unsigned octal */
                 r = 8; break;

             case 'D' :                  /* Signed decimal */
             case 'U' :                  /* Unsigned decimal */
                 r = 10; break;

             case 'X' :                  /* Unsigned hexdecimal */
                 r = 16; break;

             default:                    /* Unknown type (pass-through) */
                 putc_bfd(&pb, c); continue;
             }

             /* Get an argument and put it in numeral */
             v = (f & 4) ? (DWORD)va_arg(arp, long) : ((d == 'D') ? (DWORD)(long)va_arg(arp, int) : (DWORD)va_arg(arp, unsigned int));
             if (d == 'D' && (v & 0x80000000)) {
                 v = 0 - v;
                 f |= 8;
             }
             i = 0;
             do {
                 d = (TCHAR)(v % r); v /= r;
                 if (d > 9) d += (c == 'x') ? 0x27 : 0x07;
                 str[i++] = d + '0';
             } while (v && i < sizeof str / sizeof *str);
             if (f & 8) str[i++] = '-';
             j = i; d = (f & 1) ? '0' : ' ';
             if (!(f & 2)) {
                 while (j++ < w) putc_bfd(&pb, d);   /* Right pad */
             }
             do {
                 putc_bfd(&pb, str[--i]);            /* Number body */
             } while (i);
             while (j++ < w) putc_bfd(&pb, d);       /* Left pad */
         }

         va_end(arp);

         return putc_flush(&pb);
#endif
  }
#endif
  }

  return (int)VfsError::INVAL;
}
#endif

size_t VfsFile::tell()
{
    switch(type)
    {
#if defined (USE_FATFS)
    case VfsFileType::FAT:
      return f_tell(&fat.file);
  #endif
  #if defined (USE_LITTLEFS)
    case VfsFileType::LFS:
      {
        int ret = lfs_file_tell(lfs.handle, &lfs.file);
        if(ret < 0)
          return (size_t)convertResult((lfs_error)ret);
        else
          return ret;
      }
  #endif
    }

    return (size_t)VfsError::INVAL;
}

VfsError VfsFile::lseek(size_t offset)
{
  switch(type)
  {
#if defined (USE_FATFS)
  case VfsFileType::FAT:
    return convertResult(f_lseek(&fat.file, offset));
    break;
#endif
#if defined (USE_LITTLEFS)
  case VfsFileType::LFS:
    {
      int ret = lfs_file_seek(lfs.handle, &lfs.file, offset, LFS_SEEK_SET);
      if(ret < 0)
        return convertResult((lfs_error)ret);
      else
        return VfsError::OK;
    }
#endif
  }

  return VfsError::INVAL;
}

int VfsFile::eof()
{
  switch(type)
  {
#if defined (USE_FATFS)
  case VfsFileType::FAT:
    return f_eof(&fat.file);
#endif
#if defined (USE_LITTLEFS)
  case VfsFileType::LFS:
    return lfs_file_tell(lfs.handle, &lfs.file) == lfs_file_size(lfs.handle, &lfs.file);
#endif
  }

  return 0;
}

const char * VirtualFS::getBasename(const char * path)
{
  for (int8_t i = strlen(path) - 1; i >= 0; i--) {
    if (path[i] == '/') {
      return &path[i + 1];
    }
  }
  return path;
}

VirtualFS::VirtualFS()
{
#if defined (SPI_FLASH)
#if defined (USE_LITTLEFS)
  // configuration of the filesystem is provided by this struct
  lfsCfg.read  = flashRead;
  lfsCfg.prog  = flashWrite;
  lfsCfg.erase = flashErase;
  lfsCfg.sync  = flashSync;

  // block device configuration
  lfsCfg.read_size = 256;
  lfsCfg.prog_size = flashSpiGetPageSize();
  lfsCfg.block_size = flashSpiGetSectorSize();
  lfsCfg.block_count = flashSpiGetSectorCount();
  lfsCfg.block_cycles = 500;
  lfsCfg.cache_size = flashSpiGetPageSize();
  lfsCfg.lookahead_size = 256;
#else
  spiFatFs = {0};
#endif // USE_LITTLEFS
#endif // SPI_FLASH
#if defined (SDCARD)
  sdFatFs = {0};
#endif

  restart();
}

VirtualFS::~VirtualFS()
{
#if defined (SPI_FLASH)
#if defined (USE_LITTLEFS)
  lfs_unmount(&lfs);
#else // USE_LITTLEFS
  f_unmount("1:");
#endif // USE_LITTLEFS
#endif // SPI_FLASH
}

void VirtualFS::stop()
{
  stopLogs();
  audioQueue.stopSD();
#if defined (SDCARD)
  if (sdCardMounted()) {
    f_mount(nullptr, "", 0); // unmount SD
  }
#endif

#if defined (SPI_FLASH)
#if defined (USE_LITTLEFS)
  lfs_unmount(&lfs);
#else // USE_LITTLEFS
  f_unmount("1:");
#endif // USE_LITTLEFS
#endif // SPI_FLASH
}
void VirtualFS::restart()
{
  TRACE("VirtualFS::restart()");
#if defined (SDCARD)
  mountSd();
#endif
#if defined (SPI_FLASH)
#if defined(USE_LITTLEFS)
  //  flashSpiEraseAll();
  lfsMounted = true;
  int err = lfs_mount(&lfs, &lfsCfg);
  if(err) {
      flashSpiEraseAll();
      err = lfs_format(&lfs, &lfsCfg);
      if(err == LFS_ERR_OK)
        err = lfs_mount(&lfs, &lfsCfg);
      if(err != LFS_ERR_OK)
      {
        lfsMounted = false;
#if !defined(BOOT)
        POPUP_WARNING(STR_SDCARD_ERROR);
#endif
      }
  }
  lfsCfg.context = this;
#else // USE_LITTLEFS
#if !defined(BOOT)
  diskCache[1].clear();
#endif
  if(f_mount(&spiFatFs, "1:", 1) != FR_OK)
  {
#if !defined(BOOT)
    BYTE work[FF_MAX_SS];
    FRESULT res = f_mkfs("1:", FM_ANY, 0, work, sizeof(work));
#if !defined(BOOT)
    switch(res) {
      case FR_OK :
        break;
      case FR_DISK_ERR:
        POPUP_WARNING("Format error");
        break;
      case FR_NOT_READY:
        POPUP_WARNING("Flash not ready");
        break;
      case FR_WRITE_PROTECTED:
        POPUP_WARNING("Flash write protected");
        break;
      case FR_INVALID_PARAMETER:
        POPUP_WARNING("Format param invalid");
        break;
      case FR_INVALID_DRIVE:
        POPUP_WARNING("Invalid drive");
        break;
      case FR_MKFS_ABORTED:
        POPUP_WARNING("Format aborted");
        break;
      default:
        POPUP_WARNING(STR_SDCARD_ERROR);
        break;
    }
#endif
    if(f_mount(&spiFatFs, "1:", 1) != FR_OK)
    {
#if !defined(BOOT)
      POPUP_WARNING(STR_SDCARD_ERROR);
#endif
    }
#endif
  }
#endif // USE_LITLEFS
#endif // SPI_FLASH
#if !defined(BOOT)
  checkAndCreateDirectory("/DEFAULT/RADIO");
  checkAndCreateDirectory("/DEFAULT/MODELS");
  checkAndCreateDirectory("/DEFAULT/LOGS");
  checkAndCreateDirectory("/DEFAULT/SCREENSHOTS");
  checkAndCreateDirectory("/DEFAULT/BACKUP");
#endif
  startLogs();
}

void VirtualFS::mountSd()
{
  TRACE("VirtualFS::mountSd");
#if defined(SDCARD)
  if(sdCardMounted())
    return;

#if defined(DISK_CACHE) && !defined(BOOT)
  diskCache[0].clear();
#endif

  if (f_mount(&sdFatFs, "", 1) == FR_OK) {
#if(!defined(BOOT))
    // call sdGetFreeSectors() now because f_getfree() takes a long time first time it's called
    sdGetFreeSectors();
#endif
  }
  else {
    TRACE("SD Card f_mount() failed");
  }
#endif
}

bool VirtualFS::defaultStorageAvailable()
{
#if defined (SIMU)
  return true;
#endif
#if (DEFAULT_STORAGE == INTERNAL)
#if defined (USE_LITTLEFS)
    return lfsMounted
#else // USE_LITTLEFS
    return spiFatFs.fs_type != 0;
#endif // USE_LITTLEFS
#elif (DEFAULT_STORAGE == SDCARD) // DEFAULT_STORAGE
    return sdFatFs.fs_type != 0;
#endif

}
#if !defined(BOOT)
bool VirtualFS::format()
{
// TODO format
#if defined (USE_LITTLEFS)

  flashSpiEraseAll();
  lfs_format(&lfs, &lfsCfg);
  return true;
#else
  return false;
#endif
//  BYTE work[FF_MAX_SS];
//  FRESULT res = f_mkfs("", FM_FAT32, 0, work, sizeof(work));
//  switch(res) {
//    case FR_OK :
//      return true;
//    case FR_DISK_ERR:
//      POPUP_WARNING("Format error");
//      return false;
//    case FR_NOT_READY:
//      POPUP_WARNING("SDCard not ready");
//      return false;
//    case FR_WRITE_PROTECTED:
//      POPUP_WARNING("SDCard write protected");
//      return false;
//    case FR_INVALID_PARAMETER:
//      POPUP_WARNING("Format param invalid");
//      return false;
//    case FR_INVALID_DRIVE:
//      POPUP_WARNING("Invalid drive");
//      return false;
//    case FR_MKFS_ABORTED:
//      POPUP_WARNING("Format aborted");
//      return false;
//    default:
//      POPUP_WARNING(STR_SDCARD_ERROR);
//      return false;
//  }
}
#endif

VfsDir::DirType VirtualFS::getDirTypeAndPath(std::string& path)
{
  if(path == "/")
  {
    return VfsDir::DIR_ROOT;
#if defined (SPI_FLASH)
  } else if(path.substr(0, 9) == "/INTERNAL")
  {
#if defined (USE_LITTLEFS)
      path = path.substr(6);
    return VfsDir::DIR_LFS;
#else // USE_LITTLEFS
    path = "1:" + path.substr(9);
    if(path == "1:")
      path = "1:/";
    else if (path == "")
      path = "/";
    return VfsDir::DIR_FAT;
#endif // USE_LITTLEFS
#endif  // SPI_FLASH
#if defined (SDCARD)
  } else if(path.substr(0, 7) == "/SDCARD") {
    path = path.substr(7);
    if (path == "")
      path = "/";
    return VfsDir::DIR_FAT;
#endif
  } else if(path.substr(0, 8) == "/DEFAULT") {
#if (DEFAULT_STORAGE == INTERNAL)
#if defined (USE_LITTLEFS)
    path = path.substr(8);
    return VfsDir::DIR_LFS;
#else // USE_LITTLEFS
    path = "1:" + path.substr(8);
    if(path == "1:")
      path = "1:/";
    return VfsDir::DIR_FAT;
#endif // USE_LITTLEFS
#elif (DEFAULT_STORAGE == SDCARD) // DEFAULT_STORAGE
    path = path.substr(8);
    return VfsDir::DIR_FAT;
#else  // DEFAULT_STORAGE
  #error No valid default storage selectd
#endif
  }
  return VfsDir::DIR_UNKNOWN;
}

void VirtualFS::normalizePath(std::string& path)
{
  std::vector<std::string> tokens;
  size_t oldpos = 0;

  if(path[0] != '/')
    path = curWorkDir + PATH_SEPARATOR + path;

  while (1)
  {
      size_t newpos = path.find_first_of(PATH_SEPARATOR, oldpos);
      if(newpos == std::string::npos)
      {
        std::string elem = path.substr(oldpos);
        if(elem == "..")
          tokens.pop_back();
        else if (elem == ".")
          ;
        else
          tokens.push_back(path.substr(oldpos));
        break;
      }
      tokens.push_back(path.substr(oldpos,newpos-oldpos));
      oldpos=newpos+1;
  }

  if(tokens.empty())
    return;

  path = "";
  for(auto token: tokens)
  {
    if(token.length() == 0)
      continue;
    path += PATH_SEPARATOR;
    path += token;
  }
  if(path.length() == 0)
    path = "/";
}
#if !defined(BOOT)
VfsError VirtualFS::unlink(const std::string& path)
{
  std::string p = path;
  normalizePath(p);
  VfsDir::DirType type = getDirTypeAndPath(p);

  switch(type)
  {
  case VfsDir::DIR_ROOT:
    return VfsError::INVAL;
#if defined (USE_FATFS)
  case VfsDir::DIR_FAT:
    return convertResult(f_unlink(p.c_str()));
#endif
#if defined (USE_LITTLEFS)
  case VfsDir::DIR_LFS:
    return convertResult((lfs_error)lfs_remove(&lfs, p.c_str()));
#endif
  }

  return VfsError::INVAL;
}
#endif
VfsError VirtualFS::changeDirectory(const std::string& path)
{
  if(path.length() == 0)
    return VfsError::INVAL;

  std::string newWorkDir = path;
  normalizePath(newWorkDir);
  curWorkDir = newWorkDir;

  return VfsError::OK;
}

VfsError VirtualFS::openDirectory(VfsDir& dir, const char * path)
{
  dir.clear();
  if(path == nullptr)
    return VfsError::INVAL;

  if(path[0] == 0)
    return VfsError::INVAL;

  std::string dirPath(path);

  normalizePath(dirPath);

  VfsDir::DirType type = getDirTypeAndPath(dirPath);
  dir.type = type;
  switch(type)
  {
  case VfsDir::DIR_ROOT:
    return VfsError::OK;
#if defined (USE_LITTLEFS)
  case VfsDir::DIR_LFS:
      dir.lfs.handle = &lfs;
    return convertResult((lfs_error)lfs_dir_open(&lfs, &dir.lfs.dir, dirPath.c_str()));
#endif
#if defined (USE_FATFS)
  case VfsDir::DIR_FAT:
    return convertResult(f_opendir(&dir.fat.dir, dirPath.c_str()));
#endif
  }

  return VfsError::INVAL;
}
#if !defined(BOOT)
VfsError VirtualFS::makeDirectory(const std::string& path)
{
  std::string normPath(path);
  normalizePath(normPath);
  VfsDir::DirType dirType = getDirTypeAndPath(normPath);

  switch(dirType)
  {
  case VfsDir::DIR_ROOT:
    return VfsError::INVAL;
    break;
#if defined (USE_FATFS)
  case VfsDir::DIR_FAT:
  {
    DIR dir;
    FRESULT result = f_opendir(&dir, normPath.c_str());
    if (result == FR_OK) {
      f_closedir(&dir);
      return VfsError::OK;
    } else {
      if (result == FR_NO_PATH)
        result = f_mkdir(normPath.c_str());
      if (result != FR_OK)
        return convertResult(result);
    }
    break;
  }
#endif
#if defined (USE_LITTLEFS)
  case VfsDir::DIR_LFS:
  {
    lfs_dir_t dir;
    lfs_error res = (lfs_error)lfs_dir_open(&lfs, &dir, normPath.c_str());
    if(res == LFS_ERR_OK)
    {
      lfs_dir_close(&lfs, &dir);
      return VfsError::OK;
    } else {
      if(res == LFS_ERR_NOENT)
        res = (lfs_error)lfs_mkdir(&lfs, normPath.c_str());
      if(res != LFS_ERR_OK)
      {
        return convertResult(res);
      }
    }
break;
  }
#endif
  }
  return VfsError::INVAL;
}
VfsError VirtualFS::rename(const char* oldPath, const char* newPath)
{
  std::string oldP = oldPath;
  std::string newP = newPath;

  normalizePath(oldP);
  normalizePath(newP);

  VfsDir::DirType oldType = getDirTypeAndPath(oldP);
  VfsDir::DirType newType = getDirTypeAndPath(newP);

  if(oldType == newType)
  {
    switch(oldType)
    {
    case VfsDir::DIR_ROOT:
      return VfsError::INVAL;
#if defined (USE_FATFS)
    case VfsDir::DIR_FAT:
      return convertResult(f_rename(oldP.c_str(), newP.c_str()));
#endif
#if defined (USE_LITTLEFS)
    case VfsDir::DIR_LFS:
      return convertResult((lfs_error)lfs_rename(&lfs, oldP.c_str(), newP.c_str()));
#endif
    }
  } else {
    VfsError err = copyFile(oldPath, newPath);
    if(err == VfsError::OK)
      return unlink(oldPath);
    return err;
  }
  return VfsError::INVAL;
}

VfsError VirtualFS::copyFile(const std::string& source, const std::string& destination)
{
  VfsFile src;
  VfsFile dest;

  VfsError err = openFile(src, source, VfsOpenFlags::READ);
  if(err != VfsError::OK)
    return err;

  err = openFile(dest, destination, VfsOpenFlags::CREATE_NEW|VfsOpenFlags::WRITE);
  if(err != VfsError::OK)
  {
    src.close();
    return err;
  }

  err = VfsError::OK;

  char buf[256] = {0};
  size_t readBytes = 0;
  size_t written = 0;

  err = src.read(buf, sizeof(buf), readBytes);
  if(err != VfsError::OK)
    goto cleanup;

  while(readBytes)
  {
    err = dest.write(buf, readBytes, written);
    if(err != VfsError::OK)
      goto cleanup;
    err = src.read(buf, sizeof(buf), readBytes);
    if(err != VfsError::OK)
      goto cleanup;
  }

cleanup:
  src.close();
  dest.close();
  return err;
}

VfsError VirtualFS::copyFile(const std::string& srcFile, const std::string& srcDir,
           const std::string& destDir, const std::string& destFile)
{
  return copyFile(srcDir +"/" + srcFile, destDir + "/" + destFile);
}

// Will overwrite if destination exists
const char * VirtualFS::moveFile(const std::string& srcPath, const std::string& destPath)
{

  auto res = copyFile(srcPath, destPath);
  if(res != VfsError::OK) {
    return STORAGE_ERROR(res);
  }

  res = unlink(srcPath);
  if(res != VfsError::OK) {
    return STORAGE_ERROR(res);
  }
  return nullptr;
}

// Will overwrite if destination exists
const char * VirtualFS::moveFile(const std::string& srcFilename, const std::string& srcDir, const std::string& destFilename, const std::string& destDir)
{
  auto res = copyFile(srcFilename, srcDir, destFilename, destDir);
  if(res != VfsError::OK) {
    return STORAGE_ERROR(res);
  }

  char srcPath[2*CLIPBOARD_PATH_LEN+1] = { 0 };
  char * tmp = strAppend(srcPath, srcDir.c_str(), CLIPBOARD_PATH_LEN);
  *tmp++ = '/';
  strAppend(tmp, srcFilename.c_str(), CLIPBOARD_PATH_LEN);
  res = unlink(srcPath);
  if(res != VfsError::OK) {
    return STORAGE_ERROR(res);
  }
  return nullptr;
}
#endif // !BOOT
#if !defined(SIMU) || defined(SIMU_DISKIO)
uint32_t sdGetNoSectors()
{
  static DWORD noSectors = 0;
  if (noSectors == 0 ) {
    disk_ioctl(0, GET_SECTOR_COUNT, &noSectors);
  }
  return noSectors;
}

#endif
VfsError VirtualFS::fstat(const std::string& path, VfsFileInfo& fileInfo)
{
  std::string normPath(path);
  normalizePath(normPath);
  VfsDir::DirType dirType = getDirTypeAndPath(normPath);

  switch(dirType)
  {
  case VfsDir::DIR_ROOT:
    return VfsError::INVAL;
#if defined (USE_FATFS)
  case VfsDir::DIR_FAT:
    fileInfo.type = VfsFileType::FAT;
    return convertResult(f_stat(normPath.c_str(), &fileInfo.fatInfo));
#endif
#if defined (USE_LITTLEFS)
  case VfsDir::DIR_LFS:
    fileInfo.type = VfsFileType::LFS;
    return convertResult((lfs_error)lfs_stat(&lfs, normPath.c_str(), fileInfo.lfsInfo));
#endif
  }
  return VfsError::INVAL;
}
#if !defined(BOOT)
VfsError VirtualFS::utime(const std::string& path, const VfsFileInfo& fileInfo)
{
  std::string normPath(path);
  normalizePath(normPath);
  VfsDir::DirType dirType = getDirTypeAndPath(normPath);

  switch(dirType)
  {
  case VfsDir::DIR_ROOT:
    return VfsError::INVAL;
#if defined (USE_FATFS)
  case VfsDir::DIR_FAT:
    return convertResult(f_utime(normPath.c_str(), &fileInfo.fatInfo));
#endif
#if defined (USE_LITTLEFS)
  case VfsDir::DIR_LFS:
    return VfsError::OK;
#endif
  }
  return VfsError::INVAL;
}
#endif
VfsError VirtualFS::openFile(VfsFile& file, const std::string& path, VfsOpenFlags flags)
{
  file.clear();
  std::string normPath(path);
  normalizePath(normPath);
  VfsDir::DirType dirType = getDirTypeAndPath(normPath);

  VfsError ret = VfsError::INVAL;
  switch(dirType)
  {
  case VfsDir::DIR_ROOT:
    return VfsError::INVAL;
    break;
#if defined (USE_FATFS)
  case VfsDir::DIR_FAT:
  {
    file.type = VfsFileType::FAT;
    ret = convertResult(f_open(&file.fat.file, normPath.c_str(),
                               convertOpenFlagsToFat(flags)));
    break;
  }
#endif
#if defined (USE_LITTLEFS)
  case VfsDir::DIR_LFS:
  {
    file.type = VfsFileType::LFS;
    file.lfs.handle = &lfs;
    ret = convertResult((lfs_error)lfs_file_open(&lfs, &file.lfs.file, normPath.c_str(),
                                                 convertOpenFlagsToLfs(flags)));
    break;
  }
#endif
  }

  return ret;
}
#if !defined(BOOT)
const char* VirtualFS::checkAndCreateDirectory(const char * path)
{
  VfsError res = makeDirectory(path);

  if(res == VfsError::OK)
    return nullptr;
#if !defined(BOOT)
  return STORAGE_ERROR(res);
#else
  return "could not create directory";
#endif
}

bool VirtualFS::isFileAvailable(const char * path, bool exclDir)
{
  std::string p = path;
  VfsDir::DirType dirType = getDirTypeAndPath(p);

  switch(dirType)
  {
  case VfsDir::DIR_ROOT:
    return false;
    break;
#if defined (USE_FATFS)
  case VfsDir::DIR_FAT:
    {
      if (exclDir) {
        FILINFO fno;
        return (f_stat(p.c_str(), &fno) == FR_OK && !(fno.fattrib & AM_DIR));
      }
      return f_stat(p.c_str(), nullptr) == FR_OK;
    }
#endif
#if defined (USE_LITTLEFS)
  case VfsDir::DIR_LFS:
    {
      lfs_file_t file;
      int res = lfs_file_open(&lfs, &file, p.c_str(), LFS_O_RDONLY);
      if(res != LFS_ERR_OK)
      {
        if(res == LFS_ERR_ISDIR)
          return(!exclDir);
        return false;
      } else {
        lfs_file_close(&lfs, &file);
        return true;
      }
    }
#endif
  }

  return false;
}

const char * VirtualFS::getFileExtension(const char * filename, uint8_t size, uint8_t extMaxLen, uint8_t * fnlen, uint8_t * extlen)
{
  int len = size;
  if (!size) {
    len = strlen(filename);
  }
  if (!extMaxLen) {
    extMaxLen = LEN_FILE_EXTENSION_MAX;
  }
  if (fnlen != nullptr) {
    *fnlen = (uint8_t)len;
  }
  for (int i=len-1; i >= 0 && len-i <= extMaxLen; --i) {
    if (filename[i] == '.') {
      if (extlen) {
        *extlen = len-i;
      }
      return &filename[i];
    }
  }
  if (extlen != nullptr) {
    *extlen = 0;
  }
  return nullptr;
}

/**
  Check if given extension exists in a list of extensions.
  @param extension The extension to search for, including leading period.
  @param pattern One or more file extensions concatenated together, including the periods.
    The list is searched backwards and the first match, if any, is returned.
    eg: ".gif.jpg.jpeg.png"
  @param match Optional container to hold the matched file extension (wide enough to hold LEN_FILE_EXTENSION_MAX + 1).
  @retval true if a extension was found in the lost, false otherwise.
*/
bool VirtualFS::isFileExtensionMatching(const char * extension, const char * pattern, char * match)
{
  const char *ext;
  uint8_t extlen, fnlen;
  int plen;

  ext = getFileExtension(pattern, 0, 0, &fnlen, &extlen);
  plen = (int)fnlen;
  while (plen > 0 && ext) {
    if (!strncasecmp(extension, ext, extlen)) {
      if (match != nullptr) strncat(&(match[0]='\0'), ext, extlen);
      return true;
    }
    plen -= extlen;
    if (plen > 0) {
      ext = getFileExtension(pattern, plen, 0, nullptr, &extlen);
    }
  }
  return false;
}


/**
  Search file system path for a file. Can optionally take a list of file extensions to search through.
  Eg. find "splash.bmp", or the first occurrence of one of "splash.[bmp|jpeg|jpg|gif]".

  @param path String with path name, no trailing slash. eg; "/BITMAPS"
  @param file String containing file name to search for, with or w/out an extension.
    eg; "splash.bmp" or "splash"
  @param pattern Optional list of one or more file extensions concatenated together, including the period(s).
    The list is searched backwards and the first match, if any, is returned.  If null, then only the actual filename
    passed will be searched for.
    eg: ".gif.jpg.jpeg.bmp"
  @param exclDir true/false whether to allow directory matches (default true, excludes directories)
  @param match Optional container to hold the matched file extension (wide enough to hold LEN_FILE_EXTENSION_MAX + 1).
  @retval true if a file was found, false otherwise.
*/
bool VirtualFS::isFilePatternAvailable(const char * path, const char * file, const char * pattern, bool exclDir, char * match)
{
  uint8_t fplen;
  char fqfp[LEN_FILE_PATH_MAX + FF_MAX_LFN + 1] = "\0";

  fplen = strlen(path);
  if (fplen > LEN_FILE_PATH_MAX) {
    //TRACE_ERROR("isFilePatternAvailable(%s) = error: path too long.\n", path);
    return false;
  }

  strcpy(fqfp, path);
  strcpy(fqfp + fplen, "/");
  strncat(fqfp + (++fplen), file, FF_MAX_LFN);

  if (pattern == nullptr) {
    // no extensions list, just check the filename as-is
    return isFileAvailable(fqfp, exclDir);
  }
  else {
    // extensions list search
    const char *ext;
    uint16_t len;
    uint8_t extlen, fnlen;
    int plen;

    getFileExtension(file, 0, 0, &fnlen, &extlen);
    len = fplen + fnlen - extlen;
    fqfp[len] = '\0';
    ext = getFileExtension(pattern, 0, 0, &fnlen, &extlen);
    plen = (int)fnlen;
    while (plen > 0 && ext) {
      strncat(fqfp + len, ext, extlen);
      if (isFileAvailable(fqfp, exclDir)) {
        if (match != nullptr) strncat(&(match[0]='\0'), ext, extlen);
        return true;
      }
      plen -= extlen;
      if (plen > 0) {
        fqfp[len] = '\0';
        ext = getFileExtension(pattern, plen, 0, nullptr, &extlen);
      }
    }
  }
  return false;
}

char* VirtualFS::getFileIndex(char * filename, unsigned int & value)
{
  value = 0;
  char * pos = (char *)getFileExtension(filename);
  if (!pos || pos == filename)
    return nullptr;
  int multiplier = 1;
  while (pos > filename) {
    pos--;
    char c = *pos;
    if (c >= '0' && c <= '9') {
      value += multiplier * (c - '0');
      multiplier *= 10;
    }
    else {
      return pos+1;
    }
  }
  return filename;
}

static uint8_t _getDigitsCount(unsigned int value)
{
  uint8_t count = 1;
  while (value >= 10) {
    value /= 10;
    ++count;
  }
  return count;
}

unsigned int VirtualFS::findNextFileIndex(char * filename, uint8_t size, const char * directory)
{
  unsigned int index = 0;
  uint8_t extlen;
  char * indexPos = getFileIndex(filename, index);
  char extension[LEN_FILE_EXTENSION_MAX+1] = "\0";
  char * p = (char *)getFileExtension(filename, 0, 0, nullptr, &extlen);
  if (p) strncat(extension, p, sizeof(extension)-1);
  while (true) {
    index++;
    if ((indexPos - filename) + _getDigitsCount(index) + extlen > size) {
      return 0;
    }
    char * pos = strAppendUnsigned(indexPos, index);
    strAppend(pos, extension);
    if (!isFilePatternAvailable(directory, filename, nullptr, false)) {
      return index;
    }
  }
  return 0;
}
#endif
#if !defined(LIBOPENUI) && !defined(BOOT)
bool VirtualFS::listFiles(const char * path, const char * extension, const uint8_t maxlen, const char * selection, uint8_t flags)
{
  static uint16_t lastpopupMenuOffset = 0;
  VfsFileInfo fno;
  VfsDir dir;
  const char * fnExt;
  uint8_t fnLen, extLen;
  char tmpExt[LEN_FILE_EXTENSION_MAX+1] = "\0";

  popupMenuOffsetType = MENU_OFFSET_EXTERNAL;

  static uint8_t s_last_flags;

  if (selection) {
    s_last_flags = flags;
    if (!isFilePatternAvailable(path, selection, ((flags & LIST_SD_FILE_EXT) ? nullptr : extension))) selection = nullptr;
  }
  else {
    flags = s_last_flags;
  }

  if (popupMenuOffset == 0) {
    lastpopupMenuOffset = 0;
    memset(reusableBuffer.modelsel.menu_bss, 0, sizeof(reusableBuffer.modelsel.menu_bss));
  }
  else if (popupMenuOffset == popupMenuItemsCount - MENU_MAX_DISPLAY_LINES) {
    lastpopupMenuOffset = 0xffff;
    memset(reusableBuffer.modelsel.menu_bss, 0, sizeof(reusableBuffer.modelsel.menu_bss));
  }
  else if (popupMenuOffset == lastpopupMenuOffset) {
    // should not happen, only there because of Murphy's law
    return true;
  }
  else if (popupMenuOffset > lastpopupMenuOffset) {
    memmove(reusableBuffer.modelsel.menu_bss[0], reusableBuffer.modelsel.menu_bss[1], (MENU_MAX_DISPLAY_LINES-1)*MENU_LINE_LENGTH);
    memset(reusableBuffer.modelsel.menu_bss[MENU_MAX_DISPLAY_LINES-1], 0xff, MENU_LINE_LENGTH);
  }
  else {
    memmove(reusableBuffer.modelsel.menu_bss[1], reusableBuffer.modelsel.menu_bss[0], (MENU_MAX_DISPLAY_LINES-1)*MENU_LINE_LENGTH);
    memset(reusableBuffer.modelsel.menu_bss[0], 0, MENU_LINE_LENGTH);
  }

  popupMenuItemsCount = 0;

  VfsError res = openDirectory(dir, path);
  if (res == VfsError::OK) {

    if (flags & LIST_NONE_SD_FILE) {
      popupMenuItemsCount++;
      if (selection) {
        lastpopupMenuOffset++;
      }
      else if (popupMenuOffset==0 || popupMenuOffset < lastpopupMenuOffset) {
        char * line = reusableBuffer.modelsel.menu_bss[0];
        memset(line, 0, MENU_LINE_LENGTH);
        strcpy(line, "---");
        popupMenuItems[0] = line;
      }
    }

    for (;;) {
      res = dir.read(fno);                   /* Read a directory item */
      if (res != VfsError::OK || strlen(fno.getName()) == 0) break;  /* Break on error or end of dir */
      if (fno.getType() == VfsType::DIR) continue;            /* Skip subfolders */
      if ((int)(fno.getAttrib() & VfsFileAttributes::HID) != 0) continue;     /* Skip hidden files */
      if ((int)(fno.getAttrib() & VfsFileAttributes::SYS) != 0) continue;     /* Skip system files */

      fnExt = getFileExtension(fno.getName(), 0, 0, &fnLen, &extLen);
      fnLen -= extLen;

//      TRACE_DEBUG("listSdFiles(%s, %s, %u, %s, %u): fn='%s'; fnExt='%s'; match=%d\n",
//           path, extension, maxlen, (selection ? selection : "nul"), flags, fname.c_str(), (fnExt ? fnExt : "nul"), (fnExt && isExtensionMatching(fnExt, extension)));
      // file validation checks
      if (!fnLen || fnLen > maxlen || (                                              // wrong size
            fnExt && extension && (                                                  // extension-based checks follow...
              !isFileExtensionMatching(fnExt, extension) || (                            // wrong extension
                !(flags & LIST_SD_FILE_EXT) &&                                       // only if we want unique file names...
                strcasecmp(fnExt, getFileExtension(extension)) &&                    // possible duplicate file name...
                isFilePatternAvailable(path, fno.getName(), extension, true, tmpExt) &&  // find the first file from extensions list...
                strncasecmp(fnExt, tmpExt, LEN_FILE_EXTENSION_MAX)                   // found file doesn't match, this is a duplicate
              )
            )
          ))
      {
        continue;
      }

      popupMenuItemsCount++;

      std::string fname = fno.getName();
      if (!(flags & LIST_SD_FILE_EXT)) {
        fname = fname.substr(0,fnLen);  // strip extension
      }

      if (popupMenuOffset == 0) {
        if (selection && strncasecmp(fname.c_str(), selection, maxlen) < 0) {
          lastpopupMenuOffset++;
        }
        else {
          for (uint8_t i=0; i<MENU_MAX_DISPLAY_LINES; i++) {
            char * line = reusableBuffer.modelsel.menu_bss[i];
            if (line[0] == '\0' || strcasecmp(fname.c_str(), line) < 0) {
              if (i < MENU_MAX_DISPLAY_LINES-1) memmove(reusableBuffer.modelsel.menu_bss[i+1], line, sizeof(reusableBuffer.modelsel.menu_bss[i]) * (MENU_MAX_DISPLAY_LINES-1-i));
              memset(line, 0, MENU_LINE_LENGTH);
              strcpy(line, fname.c_str());
              break;
            }
          }
        }
        for (uint8_t i=0; i<min(popupMenuItemsCount, (uint16_t)MENU_MAX_DISPLAY_LINES); i++) {
          popupMenuItems[i] = reusableBuffer.modelsel.menu_bss[i];
        }
      }
      else if (lastpopupMenuOffset == 0xffff) {
        for (int i=MENU_MAX_DISPLAY_LINES-1; i>=0; i--) {
          char * line = reusableBuffer.modelsel.menu_bss[i];
          if (line[0] == '\0' || strcasecmp(fname.c_str(), line) > 0) {
            if (i > 0) memmove(reusableBuffer.modelsel.menu_bss[0], reusableBuffer.modelsel.menu_bss[1], sizeof(reusableBuffer.modelsel.menu_bss[i]) * i);
            memset(line, 0, MENU_LINE_LENGTH);
            strcpy(line, fname.c_str());
            break;
          }
        }
        for (uint8_t i=0; i<min(popupMenuItemsCount, (uint16_t)MENU_MAX_DISPLAY_LINES); i++) {
          popupMenuItems[i] = reusableBuffer.modelsel.menu_bss[i];
        }
      }
      else if (popupMenuOffset > lastpopupMenuOffset) {
        if (strcasecmp(fname.c_str(), reusableBuffer.modelsel.menu_bss[MENU_MAX_DISPLAY_LINES-2]) > 0 && strcasecmp(fname.c_str(), reusableBuffer.modelsel.menu_bss[MENU_MAX_DISPLAY_LINES-1]) < 0) {
          memset(reusableBuffer.modelsel.menu_bss[MENU_MAX_DISPLAY_LINES-1], 0, MENU_LINE_LENGTH);
          strcpy(reusableBuffer.modelsel.menu_bss[MENU_MAX_DISPLAY_LINES-1], fname.c_str());
        }
      }
      else {
        if (strcasecmp(fname.c_str(), reusableBuffer.modelsel.menu_bss[1]) < 0 && strcasecmp(fname.c_str(), reusableBuffer.modelsel.menu_bss[0]) > 0) {
          memset(reusableBuffer.modelsel.menu_bss[0], 0, MENU_LINE_LENGTH);
          strcpy(reusableBuffer.modelsel.menu_bss[0], fname.c_str());
        }
      }
    }
    dir.close();
  }

  if (popupMenuOffset > 0)
    lastpopupMenuOffset = popupMenuOffset;
  else
    popupMenuOffset = lastpopupMenuOffset;

  return popupMenuItemsCount;
	return 0;
}

#endif // !LIBOPENUI

bool VirtualFS::sdCardMounted()
{
#if defined (SDCARD)
  return sdFatFs.fs_type != 0;
#else
  return false;
#endif
}
size_t VirtualFS::sdGetFreeSectors()
{
#if defined (SDCARD)
  return ::sdGetFreeSectors();
#else
  return 0;
#endif
}

void VirtualFS::startLogs()
{
  if(!sdCardMounted())
    return;
#if defined(SDCARD)
#if defined(LOG_TELEMETRY)
  openFile(g_telemetryFile, LOGS_PATH "/telemetry.log", VfsOpenFlags::OPEN_ALWAYS | VfsOpenFlagsWRITE);
  if (g_telemetryFile.size() > 0) {
    g_telemetryFile.lseek(g_telemetryFile.size()); // append
  }
#endif

#if defined(LOG_BLUETOOTH)
  openFile(g_bluetoothFile, LOGS_PATH "/bluetooth.log", VfsOpenFlags::OPEN_ALWAYS | VfsOpenFlagsWRITE);
  if (&g_bluetoothFile.size() > 0) {
    g_bluetoothFile.lseek(g_bluetoothFile.size()); // append
  }
#endif
#endif
}

void VirtualFS::stopLogs()
{
#if defined(SDCARD)
#if defined(LOG_TELEMETRY)
  g_telemetryFile.close();
#endif

#if defined(LOG_BLUETOOTH)
  g_bluetoothFile.close();
#endif
#endif
}
