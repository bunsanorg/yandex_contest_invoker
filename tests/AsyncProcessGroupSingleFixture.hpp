#pragma once

#include "AsyncProcessGroupFixture.hpp"

struct AsyncProcessGroupSingleFixture : AsyncProcessGroupFixture {
  AsyncProcessGroupSingleFixture()
      : AsyncProcessGroupFixture(1), process(task.processes[0]) {}

  PG::Process &process;
};
