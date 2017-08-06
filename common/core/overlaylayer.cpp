/*
// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "overlaylayer.h"

#include <drm_mode.h>
#include <hwctrace.h>

#include "hwcutils.h"

#include <hwclayer.h>
#include <nativebufferhandler.h>

namespace hwcomposer {

/* rotation property bits */
#ifndef DRM_ROTATE_0
#define DRM_ROTATE_0 0
#endif

#ifndef DRM_ROTATE_90
#define DRM_ROTATE_90 1
#endif

#ifndef DRM_ROTATE_180
#define DRM_ROTATE_180 2
#endif

#ifndef DRM_ROTATE_270
#define DRM_ROTATE_270 3
#endif

#ifndef DRM_ROTATE_X
#define DRM_REFLECT_X 4
#endif

#ifndef DRM_ROTATE_Y
#define DRM_REFLECT_Y 5
#endif

OverlayLayer::ImportedBuffer::~ImportedBuffer() {
  if (acquire_fence_ > 0) {
    close(acquire_fence_);
  }
}

OverlayLayer::ImportedBuffer::ImportedBuffer(OverlayBuffer* buffer,
                                             int32_t acquire_fence)
    : acquire_fence_(acquire_fence) {
  buffer_.reset(buffer);
}

void OverlayLayer::SetAcquireFence(int32_t acquire_fence) {
  // Release any existing fence.
  if (imported_buffer_->acquire_fence_ > 0) {
    close(imported_buffer_->acquire_fence_);
  }

  imported_buffer_->acquire_fence_ = acquire_fence;
}

int32_t OverlayLayer::GetAcquireFence() const {
  return imported_buffer_->acquire_fence_;
}

int32_t OverlayLayer::ReleaseAcquireFence() const {
  int32_t fence = imported_buffer_->acquire_fence_;
  imported_buffer_->acquire_fence_ = -1;
  return fence;
}

OverlayBuffer* OverlayLayer::GetBuffer() const {
  return imported_buffer_->buffer_.get();
}

void OverlayLayer::SetBuffer(NativeBufferHandler* buffer_handler,
                             HWCNativeHandle handle, int32_t acquire_fence) {
  OverlayBuffer* buffer = OverlayBuffer::CreateOverlayBuffer();
  buffer->InitializeFromNativeHandle(handle, buffer_handler);
  imported_buffer_.reset(new ImportedBuffer(buffer, acquire_fence));
}

void OverlayLayer::ResetBuffer() {
  imported_buffer_.reset(nullptr);
}

void OverlayLayer::SetZorder(uint32_t z_order) {
  z_order_ = z_order;
}

void OverlayLayer::SetLayerIndex(uint32_t layer_index) {
  layer_index_ = layer_index;
}

void OverlayLayer::SetTransform(int32_t transform) {
  transform_ = transform;
  rotation_ = 0;
  if (transform & kReflectX)
    rotation_ |= 1 << DRM_REFLECT_X;
  if (transform & kReflectY)
    rotation_ |= 1 << DRM_REFLECT_Y;
  if (transform & kRotate90)
    rotation_ |= 1 << DRM_ROTATE_90;
  else if (transform & kRotate180)
    rotation_ |= 1 << DRM_ROTATE_180;
  else if (transform & kRotate270)
    rotation_ |= 1 << DRM_ROTATE_270;
  else
    rotation_ |= 1 << DRM_ROTATE_0;
}

void OverlayLayer::SetAlpha(uint8_t alpha) {
  alpha_ = alpha;
}

void OverlayLayer::SetBlending(HWCBlending blending) {
  blending_ = blending;
}

void OverlayLayer::SetSourceCrop(const HwcRect<float>& source_crop) {
  source_crop_width_ =
      static_cast<int>(source_crop.right) - static_cast<int>(source_crop.left);
  source_crop_height_ =
      static_cast<int>(source_crop.bottom) - static_cast<int>(source_crop.top);
  source_crop_ = source_crop;
}

void OverlayLayer::SetDisplayFrame(const HwcRect<int>& display_frame) {
  display_frame_width_ = display_frame.right - display_frame.left;
  display_frame_height_ = display_frame.bottom - display_frame.top;
  display_frame_ = display_frame;
}

void OverlayLayer::ValidatePreviousFrameState(const OverlayLayer& rhs,
                                              HwcLayer* layer) {
  OverlayBuffer* buffer = imported_buffer_->buffer_.get();
  surface_damage_ = layer->GetSurfaceDamage();
  if (!prefer_separate_plane_)
    prefer_separate_plane_ = rhs.prefer_separate_plane_;

  if (buffer->GetFormat() != rhs.imported_buffer_->buffer_->GetFormat())
    return;

  bool content_changed = false;
  // We expect cursor plane to support alpha always.
  if (rhs.gpu_rendered_ || (buffer->GetUsage() & kLayerCursor)) {
    content_changed = alpha_ != rhs.alpha_ || layer->HasDisplayRectChanged() ||
                      layer->HasContentAttributesChanged() ||
                      layer->HasLayerAttributesChanged();
    gpu_rendered_ = true;
  } else {
    if (alpha_ != rhs.alpha_ || layer->HasDisplayRectChanged() ||
        layer->HasContentAttributesChanged() ||
        layer->HasLayerAttributesChanged())
      return;
  }

  state_ &= ~kLayerAttributesChanged;

  if (!layer->HasDisplayRectChanged()) {
    state_ &= ~kDimensionsChanged;
  }

  if (!layer->HasVisibleRegionChanged() &&
      !layer->HasSurfaceDamageRegionChanged() &&
      !layer->HasLayerContentChanged() && !content_changed) {
    state_ &= ~kLayerContentChanged;
  }
}

void OverlayLayer::ValidateForOverlayUsage() {
  prefer_separate_plane_ = imported_buffer_->buffer_->IsVideoBuffer();
}

void OverlayLayer::Dump() {
  DUMPTRACE("OverlayLayer Information Starts. -------------");
  switch (blending_) {
    case HWCBlending::kBlendingNone:
      DUMPTRACE("Blending: kBlendingNone.");
      break;
    case HWCBlending::kBlendingPremult:
      DUMPTRACE("Blending: kBlendingPremult.");
      break;
    case HWCBlending::kBlendingCoverage:
      DUMPTRACE("Blending: kBlendingCoverage.");
      break;
    default:
      break;
  }

  if (transform_ & kReflectX)
    DUMPTRACE("Transform: kReflectX.");
  if (transform_ & kReflectY)
    DUMPTRACE("Transform: kReflectY.");
  if (transform_ & kReflectY)
    DUMPTRACE("Transform: kReflectY.");
  else if (transform_ & kRotate180)
    DUMPTRACE("Transform: kRotate180.");
  else if (transform_ & kRotate270)
    DUMPTRACE("Transform: kRotate270.");
  else
    DUMPTRACE("Transform: kRotate0.");

  DUMPTRACE("Alpha: %u", alpha_);

  DUMPTRACE("SourceWidth: %d", source_crop_width_);
  DUMPTRACE("SourceHeight: %d", source_crop_height_);
  DUMPTRACE("DstWidth: %d", display_frame_width_);
  DUMPTRACE("DstHeight: %d", display_frame_height_);
  DUMPTRACE("AquireFence: %d", imported_buffer_->acquire_fence_);

  imported_buffer_->buffer_->Dump();
}

}  // namespace hwcomposer
