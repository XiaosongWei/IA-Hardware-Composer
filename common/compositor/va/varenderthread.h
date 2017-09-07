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

#ifndef COMMON_COMPOSITOR_VA_VARENDERTHREAD_H_
#define COMMON_COMPOSITOR_VA_VARENDERTHREAD_H_

#include <spinlock.h>
#include <platformdefines.h>

#include <memory>
#include <vector>

#include "varenderer.h"
#include "hwcthread.h"

#include "fdhandler.h"
#include "hwcevent.h"

namespace hwcomposer {

class NativeSurface;
struct OverlayLayer;

class VARenderThread : public HWCThread {
 public:
  VARenderThread();
  ~VARenderThread() override;

  void Initialize(uint32_t gpu_fd);

  void Draw(std::vector<OverlayLayer*>& layers,
              std::vector<NativeSurface*>& surfaces);

  void HandleRoutine() override;
  void ExitThread();

  void Wait();

 private:
  void HandleDrawRequest();

  SpinLock tasks_lock_;
  std::unique_ptr<VARenderer> renderer_;
  std::vector<OverlayLayer*> layers_;
  std::vector<NativeSurface*> surfaces_;
  FDHandler fd_chandler_;
  HWCEvent cevent_;
  uint32_t gpu_fd_;
};

}  // namespace hwcomposer
#endif  // COMMON_COMPOSITOR_VA_VARENDERTHREAD_H_
