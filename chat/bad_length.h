#pragma once
#include <exception>

class bad_length :
  public std::exception {
public:
  bad_length() = default;
  bad_length(int length);
  const char *what() const noexcept override;

private:
  int length_{ 0 };
};

