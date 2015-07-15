#pragma once

#include "AsyncProcessGroupFixture.hpp"

struct AsyncProcessGroupMultipleFixture : AsyncProcessGroupFixture {
  AsyncProcessGroupMultipleFixture()
      : AsyncProcessGroupFixture(2),
        p0(task.processes[0]),
        p1(task.processes[1]) {}

  PG::Process &p(const std::size_t id) {
    if (id >= task.processes.size()) {
      task.processes.resize(id + 1, defaultProcess());
    }
    return task.processes[id];
  }

  PG::Process &p0, &p1;
};
