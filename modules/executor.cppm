module;

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <filesystem>

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
export module Executor;

import Error;
import Logging;

// This is what gets run whenever the disk gets mounted.
// argv[1] will be the path to the new mountpoint.
constexpr static const char *EXE_NAME = "onmount";
// The checker should return 0 if it is satisfied.
// argv[1] will be the path to the new mountpoint.
constexpr static const char *CHECKER_NAME = "keycheck";

namespace Executor {

export std::expected<bool, SyscallError>
run_script_on_mountpoint(const char *mountpoint) {
  auto script_file =
      std::format("{}/{}", std::filesystem::current_path().c_str(), EXE_NAME);
  if (access(script_file.c_str(), F_OK) != 0) {
    return std::unexpected{SyscallError("access", ENOENT)};
  }
  LOG::INFO("Running script");
  LOG::INFO(script_file.c_str());
  pid_t pid = vfork();
  // Child
  if (pid == 0) {
    execl(script_file.c_str(), script_file.c_str(), mountpoint);
    _exit(-1); // If exec failed
  }
  // Parent
  if (pid > 0) {
    int child_status = -1;
    int wait_status = waitpid(pid, &child_status, 0);
    if (wait_status < 0) {
      LOG::WARN("Failed to wait for script process. Reason:");
      LOG::WARN(strerror(wait_status));
      return false;
    } else {
      if (WIFEXITED(child_status)) {
        return true;
      }
      return false;
    }
    return {};
  }
  if (pid < 0) {
    return std::unexpected{SyscallError("vfork", errno)};
  }
  std::unreachable();
}

export std::expected<bool, SyscallError>
run_keyfile_check(const char *mountpoint) {
  std::string checker_file = std::format(
      "{}/{}", std::filesystem::current_path().c_str(), CHECKER_NAME);
  if (access(checker_file.c_str(), F_OK) != 0) {
    return std::unexpected{SyscallError("access", ENOENT)};
  }
  pid_t pid = vfork();
  // Child
  if (pid == 0) {
    execl(checker_file.c_str(), checker_file.c_str(), mountpoint);
    _exit(-1); // If exec failed
  }
  // Parent
  if (pid > 0) {
    int child_status = -1;
    int wait_status = waitpid(pid, &child_status, 0);
    if (wait_status < 0) {
      LOG::WARN("Failed to wait for keycheck process. Reason:");
      LOG::WARN(strerror(wait_status));
      return false;
    } else {
      if (WIFEXITED(child_status) && WEXITSTATUS(child_status) == 0) {
        return true;
      }
      return false;
    }
  }
  if (pid < 0) {
    return std::unexpected{SyscallError("vfork", errno)};
  }
  std::unreachable();
}
} // namespace Executor
