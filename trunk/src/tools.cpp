#include <algorithm>
#include "tools.h"

bool isPrefix(const std::string& prefix, const std::string& str) {
  if (prefix.length() > str.length())
    return false;
  return std::mismatch(prefix.begin(), prefix.end(), str.begin()).first == prefix.end();
}