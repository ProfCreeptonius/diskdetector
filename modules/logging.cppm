module;

#include <print>
export module Logging;

namespace LOG {
export void INFO(const char* msg) {
  std::println("INFO: {}", msg);
}

export void WARN(const char* msg) {
  std::println("WARN: {}", msg);
}

export void ERROR(const char* msg) {
  std::println("ERROR: {}", msg);
}
} // namespace LOG
