/*
 * Copyright 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __AIDLCOMPOSER_H__
#define __AIDLCOMPOSER_H__

#pragma once
#include <aidl/android/hardware/graphics/composer3/BnComposer.h>
#include <utils/Mutex.h>
#include "AidlComposerClient.h"
#include "DisplayConfigAIDL.h"

namespace aidl {
namespace vendor {
namespace qti {
namespace hardware {
namespace display {
namespace composer3 {
using aidl::android::hardware::graphics::composer3::BnComposer;
using aidl::android::hardware::graphics::composer3::Capability;
using aidl::android::hardware::graphics::composer3::IComposerClient;
using ndk::ScopedAStatus;

using aidl::vendor::qti::hardware::display::config::DisplayConfigAIDL;

class AidlComposer : public BnComposer {
 public:
  AidlComposer();
  virtual ~AidlComposer();

  binder_status_t dump(int fd, const char **args, uint32_t numArgs) override;

  // Composer3 API
  ScopedAStatus createClient(std::shared_ptr<IComposerClient> *aidl_return) override;
  ScopedAStatus getCapabilities(std::vector<Capability> *aidl_return) override;

 private:
  bool waitForClientDestroyedLocked(std::unique_lock<std::mutex> &lock);
  void onClientDestroyed();

  HWCSession *hwc_session_ = nullptr;
  DisplayConfigAIDL *display_config_aidl_ = nullptr;
  std::mutex mClientMutex;
  bool mClientAlive GUARDED_BY(mClientMutex) = false;
  std::condition_variable mClientDestroyedCondition;
};

}  // namespace composer3
}  // namespace display
}  // namespace hardware
}  // namespace qti
}  // namespace vendor
}  // namespace aidl

#endif  // __AIDLCOMPOSER_H__