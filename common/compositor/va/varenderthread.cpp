/*
// Copyright (c) 2017 Intel Corporation
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

#include "varenderthread.h"

#include "hwcutils.h"
#include "hwctrace.h"
#include "overlaylayer.h"
#include "renderer.h"
#include "nativesurface.h"

namespace hwcomposer {

VARenderThread::VARenderThread() : HWCThread(-8, "VARenderThread") {
  if (!cevent_.Initialize())
    return;

  fd_chandler_.AddFd(cevent_.get_fd());
}

VARenderThread::~VARenderThread() {
}

void VARenderThread::Initialize(uint32_t gpu_fd) {
  gpu_fd_ = gpu_fd;
  if (!InitWorker()) {
    ETRACE("Failed to initalize VARenderThread. %s", PRINTERROR());
  }
}

void VARenderThread::Wait() {
  if (fd_chandler_.Poll(-1) <= 0) {
    ETRACE("Poll Failed in DisplayManager %s", PRINTERROR());
    return;
  }

  if (fd_chandler_.IsReady(cevent_.get_fd())) {
    // If eventfd_ is ready, we need to wait on it (using read()) to clean
    // the flag that says it is ready.
    cevent_.Wait();
  }
}

void VARenderThread::Draw(  std::vector<OverlayLayer*> &layers,
                            std::vector<NativeSurface*>& surfaces) {
  layers_.swap(layers);
  surfaces_.swap(surfaces);
  Resume();
}

void VARenderThread::ExitThread() {
  HWCThread::Exit();
  renderer_.reset(nullptr);
  std::vector<OverlayLayer*>().swap(layers_);
  std::vector<NativeSurface*>().swap(surfaces_);
}

void VARenderThread::HandleRoutine() {
  HandleDrawRequest();
  cevent_.Signal();
}

void VARenderThread::HandleDrawRequest() {
  if (!renderer_) {
    renderer_.reset(new VARenderer);
    if (!renderer_->Init(gpu_fd_)) {
      ETRACE("Failed to initialize VARenderer %s", PRINTERROR());
      renderer_.reset(nullptr);
      return;
    }
  }

  size_t size = layers_.size();
  for (size_t i = 0; i < size; i++) {
    if (!renderer_->Draw(layers_[i], surfaces_[i])) {
      ETRACE(
          "Failed to render the frame by VA, "
          "error: %s",
          PRINTERROR());
      break;
    }
  }

}

}  // namespace hwcomposer
