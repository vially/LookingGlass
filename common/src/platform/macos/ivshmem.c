/**
 * Looking Glass
 * Copyright © 2017-2022 The Looking Glass Authors
 * https://looking-glass.io
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "common/ivshmem.h"

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "common/array.h"
#include "common/debug.h"
#include "common/option.h"
#include "common/sysinfo.h"
#include "common/stringutils.h"

struct IVSHMEMInfo
{
  int  devFd;
  int  size;
  bool hasDMA;
};

static bool ivshmemDeviceValidator(struct Option * opt, const char ** error)
{
  // if it's not a kvmfr device, it must be a file on disk
  if (strlen(opt->value.x_string) > 3 && memcmp(opt->value.x_string, "kvmfr", 5) != 0)
  {
    struct stat st;
    if (stat(opt->value.x_string, &st) != 0)
    {
      *error = "Invalid path to the ivshmem file specified";
      return false;
    }
    return true;
  }

  return true;
}

static StringList ivshmemDeviceGetValues(struct Option * option)
{
  StringList sl = stringlist_new(true);

  DIR * d = opendir("/sys/class/kvmfr");
  if (!d)
    return sl;

  struct dirent * dir;
  while((dir = readdir(d)) != NULL)
  {
    if (dir->d_name[0] == '.')
      continue;

    char * devName;
    alloc_sprintf(&devName, "/dev/%s", dir->d_name);
    stringlist_push(sl, devName);
  }

  closedir(d);
  return sl;
}

void ivshmemOptionsInit(void)
{
  struct Option options[] =
  {
    {
      .module         = "app",
      .name           = "shmFile",
      .shortopt       = 'f',
      .description    = "The path to the shared memory file, or the name of the kvmfr device to use, e.g. kvmfr0",
      .type           = OPTION_TYPE_STRING,
      .value.x_string = "/dev/shm/looking-glass",
      .validator      = ivshmemDeviceValidator,
      .getValues      = ivshmemDeviceGetValues
    },
    {0}
  };

  option_register(options);
}

bool ivshmemInit(struct IVSHMEM * dev)
{
  // FIXME: split code from ivshmemOpen
  return true;
}

bool ivshmemOpen(struct IVSHMEM * dev)
{
  return ivshmemOpenDev(dev, option_get_string("app", "shmFile"));
}

bool ivshmemOpenDev(struct IVSHMEM * dev, const char * shmDevice)
{
  DEBUG_ASSERT(dev);

  unsigned int devSize;
  int devFd = -1;
  bool hasDMA = false;

  dev->opaque = NULL;

  DEBUG_INFO("KVMFR Device     : %s", shmDevice);

  // TODO(vially): Implement memory mapping
  void *map = NULL;
  devSize = 0;

  struct IVSHMEMInfo * info = malloc(sizeof(*info));
  info->size   = devSize;
  info->devFd  = devFd;
  info->hasDMA = hasDMA;

  dev->opaque = info;
  dev->size   = devSize;
  dev->mem    = map;
  return true;
}

void ivshmemClose(struct IVSHMEM * dev)
{
  DEBUG_ASSERT(dev);

  if (!dev->opaque)
    return;

  struct IVSHMEMInfo * info =
    (struct IVSHMEMInfo *)dev->opaque;

  munmap(dev->mem, info->size);
  close(info->devFd);

  free(info);
  dev->mem    = NULL;
  dev->size   = 0;
  dev->opaque = NULL;
}

void ivshmemFree(struct IVSHMEM * dev)
{
  // FIXME: split code from ivshmemClose
}

bool ivshmemHasDMA(struct IVSHMEM * dev)
{
  DEBUG_ASSERT(dev && dev->opaque);

  struct IVSHMEMInfo * info =
    (struct IVSHMEMInfo *)dev->opaque;

  return info->hasDMA;
}
