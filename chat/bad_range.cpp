#include "bad_range.h"

bad_range::bad_range(int range) :
  range_{ range } {}

const char *bad_range::what() const noexcept {
  return "Bad range";
}