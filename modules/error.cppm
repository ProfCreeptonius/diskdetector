module;

#include <format>
#include <stdexcept>
export module Error;

export struct Exception : std::runtime_error {
  using std::runtime_error::runtime_error;
};

export struct SyscallError : Exception {
  const char *syscall;
  int err_no;
  SyscallError(const char *_syscall = "unspecified", int _err_no = -1)
      : Exception(std::format("Syscall error: {} {}", _syscall, _err_no)),
        syscall(_syscall), err_no(_err_no) {}
};

export struct LimitsError : Exception {
  const char *exceeded_limit;
  int limit;
  int actual;
  LimitsError(const char *_exceeded_limit, int _limit, int _actual)
      : Exception("Limit exceeded"), exceeded_limit(_exceeded_limit),
        limit(_limit), actual(_actual) {}
};
