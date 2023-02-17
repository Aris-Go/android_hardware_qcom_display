/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <thread>
#include <chrono>
#include <errno.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <hidl/LegacySupport.h>
#include <cutils/properties.h>

#include "composer_test_service.h"
#include "comp_test_bnd_service.h"
#include "sdm_comp_debugger.h"

#define __CLASS__ "ComposerTestService"

using namespace android;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

namespace sdm {

ComposerTestService::~ComposerTestService() {
  if(cwb_test_intf_) {
    destroy_cwb_test_intf_(cwb_test_intf_);
  }
}

int ComposerTestService::Init() {
  if (cwb_test_lib_.Open(CWB_TEST_LIB_NAME)) {
    if (!cwb_test_lib_.Sym("CreateCWBTestInterface",
                           reinterpret_cast<void **>(&create_cwb_test_intf_)) ||
        !cwb_test_lib_.Sym("DestroyCWBTestInterface",
                           reinterpret_cast<void **>(&destroy_cwb_test_intf_))) {
      DLOGE("Unable to load symbols, error = %s", cwb_test_lib_.Error());
      return -1;
    }

    DLOGI("Loaded %s and symbols", CWB_TEST_LIB_NAME);

    int error = create_cwb_test_intf_(&buffer_allocator_, &cwb_test_intf_);
    if (error) {
      DLOGE("Unable to create CWB Test Interface, err: %d", error);
      return error;
    }
  } else {
    DLOGW("Unable to load = %s, error = %s", CWB_TEST_LIB_NAME, cwb_test_lib_.Error());
  }
  return 0;
}

int ComposerTestService::CommandHandler(uint32_t command, const android::Parcel *input_parcel,
                     android::Parcel *output_parcel) {
  int status = 0;
  switch (command) {
    case ICompTestBndService::CONFIGURE_CWB_TEST:
      if (!input_parcel) {
        DLOGE("QService command = %d: input_parcel needed.", command);
        break;
      }
      status = ConfigureCWBTest(input_parcel);
      break;
    case ICompTestBndService::STOP_CWB_TEST:
      status = StopCWBTest();
      break;
    default:
      status = -EINVAL;
      DLOGE("QService command = %d is not supported.", command);
      break;
  }
  return status;
}

int ComposerTestService::ConfigureCWBTest(const android::Parcel *input_parcel) {
  if (!cwb_test_intf_) {
    return -EINVAL;
  }

  struct CWBConfig cwb_config;
  cwb_config.disp = static_cast<DisplayConfig::DisplayType>(input_parcel->readInt32());
  cwb_config.post_processed = input_parcel->readInt32();
  cwb_config.format = static_cast<LayerBufferFormat>(input_parcel->readInt32());
  cwb_config.trigger_frequency = input_parcel->readInt64();
  cwb_config.cwb_roi.left = input_parcel->readInt32();
  cwb_config.cwb_roi.top = input_parcel->readInt32();
  cwb_config.cwb_roi.right = input_parcel->readInt32();
  cwb_config.cwb_roi.bottom = input_parcel->readInt32();
  DLOGI("disp %d, post_processed %d, format %d, frequency %" PRIu64 "msec, "
        "ROI: LTRB [%d %d %d %d]", cwb_config.disp, cwb_config.post_processed,
        cwb_config.format, cwb_config.trigger_frequency, cwb_config.cwb_roi.left,
        cwb_config.cwb_roi.top, cwb_config.cwb_roi.right, cwb_config.cwb_roi.bottom);

  return cwb_test_intf_->ConfigureCWB(cwb_config);
}

int ComposerTestService::StopCWBTest() {
  if (!cwb_test_intf_) {
    return -EINVAL;
  }

  return cwb_test_intf_->StopTest();
}

}  // namespace sdm

int main(int, char **) {
  ProcessState::initWithDriver("/dev/vndbinder");
  sp<ProcessState> ps(ProcessState::self());
  ps->setThreadPoolMaxThreadCount(4);
  ps->startThreadPool();

  sdm::ComposerTestService composer_test_service;
  composer_test_service.Init();
  configureRpcThreadpool(4, true /*callerWillJoin*/);
  CompTestBndService::Init(&composer_test_service);

  joinRpcThreadpool();

  return 0;
}
