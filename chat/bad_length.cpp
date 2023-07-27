#include "bad_length.h"

bad_length::bad_length(int length) :
  length_{ length } {}

const char *bad_length::what() const noexcept {
  return "Bad length";
}
