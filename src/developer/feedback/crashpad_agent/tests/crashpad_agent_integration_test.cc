// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fuchsia/cobalt/cpp/fidl.h>
#include <fuchsia/cobalt/test/cpp/fidl.h>
#include <fuchsia/feedback/cpp/fidl.h>
#include <fuchsia/mem/cpp/fidl.h>
#include <lib/sys/cpp/service_directory.h>
#include <zircon/errors.h>

#include <vector>

#include "src/developer/feedback/crashpad_agent/metrics_registry.cb.h"
#include "src/lib/fsl/vmo/strings.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace feedback {
namespace {

using namespace fuchsia::cobalt::test;
using cobalt_registry::kCrashMetricId;
using cobalt_registry::kProjectId;
using testing::UnorderedElementsAreArray;

using CrashState = cobalt_registry::CrashMetricDimensionState;

class CrashpadAgentIntegrationTest : public testing::Test {
 public:
  void SetUp() override {
    environment_services_ = sys::ServiceDirectory::CreateFromNamespace();
    environment_services_->Connect(crash_reporter_.NewRequest());
    environment_services_->Connect(logger_querier_.NewRequest());
  }

  void TearDown() override {
    // Reset the logger so tests can be run repeatedly.
    logger_querier_->ResetLogger(kProjectId, LogMethod::LOG_EVENT);
  }

 protected:
  // Returns true if a response is received.
  void FileCrashReport() {
    fuchsia::feedback::CrashReport report;
    report.set_program_name("crashing_program");

    fuchsia::feedback::CrashReporter_File_Result out_result;
    ASSERT_EQ(crash_reporter_->File(std::move(report), &out_result), ZX_OK);
    EXPECT_TRUE(out_result.is_response());
  }

  // Returns the crash states the Cobalt logger has received.
  std::vector<CrashState> GetCobaltCrashStates() {
    LoggerQuerier_WatchLogs_Result result;
    std::vector<CrashState> crash_states;

    // We may need to run WatchLogs() multiple times to collect all of the events generated by
    // crashpad agent. This is due to the fact that we are communicating with both the
    // fuchsia.cobalt/Logger and fuchsia.cobalt.test/LoggerQuerier APIs and are provided no
    // guarantees regarding the order in which messages are received. Thus it's conceivable (and
    // will actually happen quite often) that the call to WatchLogs() (and maybe even ResetLogger())
    // will get to the component serving both APIs before either of the calls to LogEvent() arrive
    // and a response containing zero or one Cobalt events is received. So, if you wish to remove
    // this while loop it is a prerequisite that you have figured out a way to guarantee the
    // ordering of independent, asynchronous messages, made it so that crashpad agent only never
    // logs to Cobalt, or don't care about flakes in your tests.
    while (crash_states.size() < 2) {
      logger_querier_->WatchLogs(kProjectId, LogMethod::LOG_EVENT, &result);
      GetCrashStates(result, &crash_states);
    }

    return crash_states;
  }

 private:
  void GetCrashStates(const LoggerQuerier_WatchLogs_Result& result,
                      std::vector<CrashState>* crash_states) {
    ASSERT_TRUE(result.is_response());

    const auto& response = result.response();
    for (const auto& event : response.events) {
      ASSERT_EQ(event.metric_id, kCrashMetricId);
      for (const auto& event_code : event.event_codes) {
        crash_states->push_back(static_cast<CrashState>(event_code));
      }
    }
  }

  std::shared_ptr<sys::ServiceDirectory> environment_services_;
  fuchsia::feedback::CrashReporterSyncPtr crash_reporter_;
  fuchsia::cobalt::test::LoggerQuerierSyncPtr logger_querier_;
};

// Smoke-tests the actual service for fuchsia.feedback.CrashReporter, connecting through FIDL.
TEST_F(CrashpadAgentIntegrationTest, CrashReporter_SmokeTest) {
  FileCrashReport();
  EXPECT_THAT(GetCobaltCrashStates(), UnorderedElementsAreArray({
                                          CrashState::Filed,
                                          CrashState::Archived,
                                      }));
}

}  // namespace
}  // namespace feedback
