#include "ACTFW/Utilities/Paths.hpp"
#include <cstdio>

std::string
FW::joinPaths(const std::string& dir, const std::string& name)
{
  if (dir.empty()) {
    return name;
  } else {
    return dir + '/' + name;
  }
}

std::string
FW::perEventFilepath(const std::string& dir,
                     const std::string& name,
                     size_t             event)
{
  char prefix[1024];

  snprintf(prefix, sizeof(prefix), "event%09zu-", event);

  if (dir.empty()) {
    return prefix + name;
  } else {
    return dir + '/' + prefix + name;
  }
}