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

#include "interface/capture.h"
#include "interface/platform.h"
#include "common/debug.h"
#include "common/stringutils.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// forwards

static bool pipewire_deinit();

// implementation

static const char * pipewire_getName(void)
{
  return "PipeWire";
}

static bool pipewire_create(CaptureGetPointerBuffer getPointerBufferFn, CapturePostPointerBuffer postPointerBufferFn)
{
  // TODO(vially): implement me
  return true;
}

static bool pipewire_init(void)
{
  // TODO(vially): implement me
  return true;
}

static void pipewire_stop(void)
{
  // TODO(vially): implement me
}

static bool pipewire_deinit(void)
{
  // TODO(vially): implement me
  return true;
}

static void pipewire_free(void)
{
  // TODO(vially): implement me
}

static CaptureResult pipewire_capture(void)
{
  // TODO(vially): implement me
  return CAPTURE_RESULT_OK;
}

static CaptureResult pipewire_waitFrame(CaptureFrame * frame,
    const size_t maxFrameSize)
{
  // TODO(vially): implement me
  return CAPTURE_RESULT_OK;
}

static CaptureResult pipewire_getFrame(FrameBuffer * frame,
    const unsigned int height, int frameIndex)
{
  // TODO(vially): implement me
  return CAPTURE_RESULT_OK;
}

struct CaptureInterface Capture_CoreGraphics =
{
  .shortName       = "CoreGraphics",
  .asyncCapture    = false,
  .getName         = pipewire_getName,
  .create          = pipewire_create,
  .init            = pipewire_init,
  .stop            = pipewire_stop,
  .deinit          = pipewire_deinit,
  .free            = pipewire_free,
  .capture         = pipewire_capture,
  .waitFrame       = pipewire_waitFrame,
  .getFrame        = pipewire_getFrame
};
