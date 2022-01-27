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

#include "common/debug.h"
#include "common/event.h"
#include "common/stringutils.h"
#include "interface/capture.h"
#include "interface/platform.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CGDisplayStream.h>
#import <IOSurface/IOSurface.h>

struct coregraphics
{
  NSScreen * screen;

  CGDisplayStreamRef stream;
  LGEvent *          streamStopEvent;
  IOSurfaceRef       current;

  // TODO(vially): Investigate if this mutex is needed?
  pthread_mutex_t mutex;

  bool stop;
  bool hasFormat;
  bool formatChanged;
  int  width, height;
};

static struct coregraphics * this = NULL;

// forwards

static bool coregraphics_deinit();

// implementation

static const char * coregraphics_getName(void)
{
  return "CoreGraphics";
}

static bool coregraphics_create(CaptureGetPointerBuffer  getPointerBufferFn,
                                CapturePostPointerBuffer postPointerBufferFn)
{
  DEBUG_ASSERT(!this);

  bool hasScreenCaptureAccess = CGPreflightScreenCaptureAccess();
  if (!hasScreenCaptureAccess)
  {
    DEBUG_ERROR("Screen capture access is not granted");
    CGRequestScreenCaptureAccess();
    return false;
  }

  this                  = calloc(1, sizeof(*this));
  this->streamStopEvent = lgCreateEvent(false, 0);

  if (!this->streamStopEvent)
  {
    DEBUG_ERROR("Failed to create the stream stop event");
    free(this);
    return false;
  }

  return true;
}

static inline void display_stream_update(CGDisplayStreamFrameStatus status,
                                         uint64_t                 display_time,
                                         IOSurfaceRef             frame_surface,
                                         CGDisplayStreamUpdateRef update_ref)
{
  if (status == kCGDisplayStreamFrameStatusStopped)
  {
    lgSignalEvent(this->streamStopEvent);
    return;
  }

  IOSurfaceRef prev_current = NULL;

  if (frame_surface && !pthread_mutex_lock(&this->mutex))
  {
    prev_current  = this->current;
    this->current = frame_surface;
    CFRetain(this->current);
    IOSurfaceIncrementUseCount(this->current);

    pthread_mutex_unlock(&this->mutex);
  }

  if (prev_current)
  {
    IOSurfaceDecrementUseCount(prev_current);
    CFRelease(prev_current);
  }

  size_t dropped_frames = CGDisplayStreamUpdateGetDropCount(update_ref);
  if (dropped_frames > 0)
    DEBUG_INFO("CoreGraphics: Dropped %zu frames", dropped_frames);
}

static bool coregraphics_init(void)
{
  DEBUG_ASSERT(this);

  pthread_mutex_init(&this->mutex, NULL);

  lgResetEvent(this->streamStopEvent);

  if ([NSScreen screens].count == 0)
    return false;

  // TODO(vially): Improve display selection logic
  unsigned display = 0;
  this->screen     = [[NSScreen screens][display] retain];

  NSRect frame = [this->screen convertRectToBacking:this->screen.frame];
  this->width  = frame.size.width;
  this->height = frame.size.height;

  NSNumber * screen_num     = this->screen.deviceDescription[@"NSScreenNumber"];
  CGDirectDisplayID disp_id = (CGDirectDisplayID)screen_num.intValue;

  NSDictionary * rect_dict =
      CFBridgingRelease(CGRectCreateDictionaryRepresentation(
          CGRectMake(0, 0, this->screen.frame.size.width,
                     this->screen.frame.size.height)));

  CFBooleanRef show_cursor_cf = kCFBooleanTrue;

  NSDictionary * dict = @{
    (__bridge NSString *)kCGDisplayStreamSourceRect : rect_dict,
    (__bridge NSString *)kCGDisplayStreamQueueDepth : @5,
    (__bridge NSString *)kCGDisplayStreamShowCursor : (id)show_cursor_cf,
  };

  this->stream = CGDisplayStreamCreateWithDispatchQueue(
      disp_id, this->width, this->height, 'BGRA',
      (__bridge CFDictionaryRef)dict, dispatch_queue_create(NULL, NULL),
      ^(CGDisplayStreamFrameStatus status, uint64_t displayTime,
        IOSurfaceRef frameSurface, CGDisplayStreamUpdateRef updateRef) {
        display_stream_update(status, displayTime, frameSurface, updateRef);
      });

  if (!this->stream) {
    DEBUG_ERROR("Unable to create display stream. Make sure the screen recording permission has been granted");
    return false;
  }

  return !CGDisplayStreamStart(this->stream);
}

static void coregraphics_stop(void)
{
  // TODO(vially): implement me
  this->stop = true;
  CGDisplayStreamStop(this->stream);
}

static bool coregraphics_deinit(void)
{
  if (this->stream)
  {
    CGDisplayStreamStop(this->stream);
    lgWaitEvent(this->streamStopEvent, 1000);
  }

  if (this->current)
  {
    IOSurfaceDecrementUseCount(this->current);
    CFRelease(this->current);
    this->current = NULL;
  }

  if (this->stream)
  {
    CFRelease(this->stream);
    this->stream = NULL;
  }

  if (this->screen)
  {
    [this->screen release];
    this->screen = nil;
  }

  // TODO(vially): implement me
  pthread_mutex_destroy(&this->mutex);
  return true;
}

static void coregraphics_free(void)
{
  DEBUG_ASSERT(this);
  lgFreeEvent(this->streamStopEvent);
  free(this);
  this = NULL;
}

static CaptureResult coregraphics_capture(void)
{
  DEBUG_ASSERT(this);

  if (this->stop)
    return CAPTURE_RESULT_REINIT;

  // TODO(vially): implement me
  return CAPTURE_RESULT_OK;
}

static CaptureResult coregraphics_waitFrame(CaptureFrame * frame,
                                            const size_t   maxFrameSize)
{
  if (this->stop)
    return CAPTURE_RESULT_REINIT;

  const int          bpp       = 4;
  const unsigned int maxHeight = maxFrameSize / (this->width * bpp);

  frame->width      = this->width;
  frame->height     = maxHeight > this->height ? this->height : maxHeight;
  frame->realHeight = this->height;
  frame->pitch      = this->width * bpp;
  frame->stride     = this->width;
  frame->format     = CAPTURE_FMT_BGRA;
  frame->rotation   = CAPTURE_ROT_0;

  // TODO: implement damage.
  frame->damageRectsCount = 0;

  return CAPTURE_RESULT_OK;
}

static CaptureResult coregraphics_getFrame(FrameBuffer *      frame,
                                           const unsigned int height,
                                           int                frameIndex)
{
  if (this->stop || !this->current)
    return CAPTURE_RESULT_REINIT;

  const int bpp       = 4;
  uint8_t * frameData = (uint8_t *)IOSurfaceGetBaseAddress(this->current);
  framebuffer_write(frame, frameData, height * this->width * bpp);

  return CAPTURE_RESULT_OK;
}

// clang-format off
struct CaptureInterface Capture_CoreGraphics =
{
  .shortName    = "CoreGraphics",
  .asyncCapture = false,
  .getName      = coregraphics_getName,
  .create       = coregraphics_create,
  .init         = coregraphics_init,
  .stop         = coregraphics_stop,
  .deinit       = coregraphics_deinit,
  .free         = coregraphics_free,
  .capture      = coregraphics_capture,
  .waitFrame    = coregraphics_waitFrame,
  .getFrame     = coregraphics_getFrame
};
// clang-format on
