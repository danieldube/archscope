#pragma once

struct SharedPort {
  virtual ~SharedPort() = default;
  virtual void run() = 0;
};
