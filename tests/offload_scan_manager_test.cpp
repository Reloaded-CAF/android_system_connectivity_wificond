/*
 * Copyright (C) 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <functional>
#include <memory>
#include <vector>
#include <string>

#include <gtest/gtest.h>
#include <android/hardware/wifi/offload/1.0/IOffload.h>

#include "wificond/tests/mock_offload.h"
#include "wificond/tests/mock_offload_service_utils.h"
#include "wificond/tests/offload_test_utils.h"

#include "wificond/scanning/scan_result.h"
#include "wificond/scanning/offload/offload_callback.h"
#include "wificond/scanning/offload/offload_scan_manager.h"

using android::hardware::wifi::offload::V1_0::ScanResult;
using android::wificond::OnOffloadScanResultsReadyHandler;
using com::android::server::wifi::wificond::NativeScanResult;
using testing::NiceMock;
using testing::_;
using testing::Invoke;
using android::sp;
using std::unique_ptr;
using std::vector;

namespace android {
namespace wificond {

sp<OffloadCallback> CaptureReturnValue(OnOffloadScanResultsReadyHandler handler,
    sp<OffloadCallback>* offload_callback) {
  *offload_callback = sp<OffloadCallback>(new OffloadCallback(handler));
  return *offload_callback;
}

class OffloadScanManagerTest: public ::testing::Test {
  protected:
    virtual void SetUp() {
      ON_CALL(*mock_offload_service_utils_, GetOffloadCallback(_))
          .WillByDefault(Invoke(bind(CaptureReturnValue,
              std::placeholders::_1, &offload_callback_)));
    }

    void TearDown() override {
      offload_callback_.clear();
    }

    sp<NiceMock<MockOffload>> mock_offload_{new NiceMock<MockOffload>()};
    sp<OffloadCallback> offload_callback_;
    unique_ptr<NiceMock<MockOffloadServiceUtils>> mock_offload_service_utils_{
        new NiceMock<MockOffloadServiceUtils>()};
    unique_ptr<OffloadScanManager> dut_;
};

/**
 * Testing OffloadScanManager with OffloadServiceUtils null argument
 */
TEST_F(OffloadScanManagerTest, ServiceUtilsNotAvailableTest) {
  dut_.reset(new OffloadScanManager(nullptr, nullptr));
  EXPECT_EQ(false, dut_->isServiceAvailable());
}

/**
 * Testing OffloadScanManager with no handle on Offloal HAL service
 * and no registered handler for Offload Scan results
 */
TEST_F(OffloadScanManagerTest, ServiceNotAvailableTest) {
  ON_CALL(*mock_offload_service_utils_, GetOffloadService())
      .WillByDefault(testing::Return(nullptr));
  dut_.reset(new OffloadScanManager(mock_offload_service_utils_.get(),
      nullptr));
  EXPECT_EQ(false, dut_->isServiceAvailable());
}

/**
 * Testing OffloadScanManager when service is available and valid handler
 * registered for Offload Scan results
 */
TEST_F(OffloadScanManagerTest, ServiceAvailableTest) {
  EXPECT_CALL(*mock_offload_service_utils_, GetOffloadService());
  ON_CALL(*mock_offload_service_utils_, GetOffloadService())
      .WillByDefault(testing::Return(mock_offload_));
  dut_ .reset(new OffloadScanManager(mock_offload_service_utils_.get(),
    [] (vector<NativeScanResult> scanResult) -> void {}));
  EXPECT_EQ(true, dut_->isServiceAvailable());
}

/**
 * Testing OffloadScanManager when service is available and valid handler
 * is registered, test to ensure that registered handler is invoked when
 * scan results are available
 */
TEST_F(OffloadScanManagerTest, CallbackInvokedTest) {
  bool callback_invoked = false;
  EXPECT_CALL(*mock_offload_service_utils_, GetOffloadService());
  ON_CALL(*mock_offload_service_utils_, GetOffloadService())
      .WillByDefault(testing::Return(mock_offload_));
  dut_.reset(new OffloadScanManager(mock_offload_service_utils_.get(),
    [&callback_invoked] (vector<NativeScanResult> scanResult) -> void {
        callback_invoked = true;
    }));
  vector<ScanResult> dummy_scan_results_ = OffloadTestUtils::createOffloadScanResults();
  offload_callback_->onScanResult(dummy_scan_results_);
  EXPECT_EQ(true, callback_invoked);
}

} // wificond
} //android