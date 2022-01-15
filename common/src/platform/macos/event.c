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

#include "common/event.h"
#include "common/debug.h"

#include <dispatch/dispatch.h>
#include <stdint.h>
#include <stdlib.h>

struct LGEvent
{
  dispatch_semaphore_t semaphore;
};

LGEvent * lgCreateEvent(bool autoReset, unsigned int msSpinTime)
{
  LGEvent * handle = calloc(1, sizeof(*handle));
  if (!handle)
  {
    DEBUG_ERROR("Failed to allocate memory");
    return NULL;
  }

  dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
  if (!semaphore)
  {
    DEBUG_ERROR("Failed to create semaphore");
    return NULL;
  }

  handle->semaphore = semaphore;
  return handle;
}

void lgFreeEvent(LGEvent * handle)
{
  DEBUG_ASSERT(handle);
  free(handle);
}

bool lgWaitEvent(LGEvent * handle, unsigned int timeout)
{
  DEBUG_ASSERT(handle);

  dispatch_time_t to =
      (timeout == TIMEOUT_INFINITE)
          ? DISPATCH_TIME_FOREVER
          : dispatch_time(DISPATCH_TIME_NOW, timeout * 1000000U);

  return dispatch_semaphore_wait(handle->semaphore, to) == 0;
}

bool lgSignalEvent(LGEvent * handle)
{
  DEBUG_ASSERT(handle);

  dispatch_semaphore_signal(handle->semaphore);

  return true;
}

bool lgResetEvent(LGEvent * handle)
{
  DEBUG_ASSERT(handle);

  dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
  if (!semaphore)
  {
    DEBUG_ERROR("Failed to recreate semaphore");
    return false;
  }

  handle->semaphore = semaphore;

  return true;
}
