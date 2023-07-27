#pragma once
#include <exception>

class bad_range :
  public std::exception {
public:
  bad_range() = default;
  bad_range(int range);
  const char *what() const noexcept override;
private:
  int range_{ 0 };
};

