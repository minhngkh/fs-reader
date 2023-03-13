#pragma once

#include "Drive.hpp"

class IReader {
 public:
  virtual void read(Drive) = 0;
  virtual void refresh() = 0;
};
