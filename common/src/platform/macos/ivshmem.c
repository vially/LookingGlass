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
#include "common/array.h"
#include "common/debug.h"
#include "common/option.h"
#include "common/stringutils.h"
#include "common/sysinfo.h"

#include <IOKit/IOKitLib.h>
#include <IOKit/IOReturn.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct IVSHMEMInfo
{
  io_connect_t      connection;
  mach_vm_address_t address;
  mach_vm_size_t    size;
};

void ivshmemOptionsInit(void)
{
  // clang-format off
  struct Option options[] =
  {
    {
      .module         = "app",
      .name           = "dextIdentifier",
      .shortopt       = 'f',
      .description    = "The IOService name of the IVSHMEM macOS driver",
      .type           = OPTION_TYPE_STRING,
      .value.x_string = "IVSHMEMDriver"
    },
    {0}
  };
  // clang-format on

  option_register(options);
}

bool ivshmemInit(struct IVSHMEM * dev)
{
  DEBUG_ASSERT(dev);

  kern_return_t ret        = kIOReturnSuccess;
  io_iterator_t iterator   = IO_OBJECT_NULL;
  io_service_t  service    = IO_OBJECT_NULL;
  io_connect_t  connection = IO_OBJECT_NULL;

  dev->opaque = NULL;

  const char * dextIdentifier = option_get_string("app", "dextIdentifier");
  DEBUG_INFO("DEXT identifier  : %s", dextIdentifier);

  ret = IOServiceGetMatchingServices(
      kIOMainPortDefault, IOServiceNameMatching(dextIdentifier), &iterator);
  if (ret != kIOReturnSuccess)
  {
    DEBUG_ERROR("Unable to find IOService for identifier: %s", dextIdentifier);
    DEBUG_ERROR("%s", strerror(ret));
    return false;
  }

  while ((service = IOIteratorNext(iterator)) != IO_OBJECT_NULL)
  {
    ret = IOServiceOpen(service, mach_task_self_, kIOHIDServerConnectType,
                        &connection);
    IOObjectRelease(service);

    if (ret == kIOReturnSuccess)
      break;

    DEBUG_ERROR("Failed opening service: %s", dextIdentifier);
    DEBUG_ERROR("%s", strerror(ret));
  }
  IOObjectRelease(iterator);

  if (connection == IO_OBJECT_NULL)
  {
    DEBUG_ERROR("Failed opening service connection: %s", dextIdentifier);
    return false;
  }

  struct IVSHMEMInfo * info = malloc(sizeof(*info));
  info->connection          = connection;
  info->address             = 0;
  info->size                = 0;

  dev->opaque = info;

  return true;
}

bool ivshmemOpen(struct IVSHMEM * dev)
{
  return ivshmemOpenDev(dev, option_get_string("app", "dextIdentifier"));
}

bool ivshmemOpenDev(struct IVSHMEM * dev, const char * shmDevice)
{
  DEBUG_ASSERT(dev && dev->opaque);

  struct IVSHMEMInfo * info = (struct IVSHMEMInfo *)dev->opaque;

  kern_return_t ret = kIOReturnSuccess;

  ret = IOConnectMapMemory64(info->connection, 0, mach_task_self_,
                             &info->address, &info->size, kIOMapAnywhere);
  if (ret != kIOReturnSuccess)
  {
    DEBUG_ERROR("Failed to map the shared memory device: %s", shmDevice);
    DEBUG_ERROR("%s", strerror(errno));
    return false;
  }

  dev->size = info->size;
  dev->mem  = (void *)info->address;

  return true;
}

void ivshmemClose(struct IVSHMEM * dev)
{
  DEBUG_ASSERT(dev);

  if (!dev->opaque)
    return;

  struct IVSHMEMInfo * info = (struct IVSHMEMInfo *)dev->opaque;

  kern_return_t ret = kIOReturnSuccess;

  ret = IOConnectUnmapMemory64(info->connection, 0, mach_task_self_,
                               info->address);
  if (ret != kIOReturnSuccess)
  {
    DEBUG_ERROR("Failed to unmap the shared memory device");
    DEBUG_ERROR("%s", strerror(errno));
    return;
  }

  info->address = 0;
  info->size    = 0;

  dev->mem  = NULL;
  dev->size = 0;
}

void ivshmemFree(struct IVSHMEM * dev)
{
  DEBUG_ASSERT(dev);

  kern_return_t ret = kIOReturnSuccess;

  if (!dev->opaque)
    return;

  // FIXME: split code from ivshmemClose
  struct IVSHMEMInfo * info = (struct IVSHMEMInfo *)dev->opaque;

  ret = IOServiceClose(info->connection);
  if (ret != kIOReturnSuccess)
  {
    DEBUG_ERROR("Failed closing service connection");
  }

  free(info);
  dev->opaque = NULL;
}

bool ivshmemHasDMA(struct IVSHMEM * dev)
{
  return false;
}
