module;

#include <cstring>
#include <expected>
#include <print>
#include <utility>

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

export module Mount;
import Error;
import Logging;

namespace Mount {

std::expected<void, int> attempt_ntfs3g_mount(const char *device,
                                              const char *path) {
  if (access("/usr/bin/ntfs-3g", F_OK) != 0) {
    LOG::ERROR("Failed to use ntfs-3g to mount ntfs drive.");
    return std::unexpected{ENOENT};
  }
  int exit_status = 0;
  pid_t pid = vfork();
  // Child
  if (pid == 0) {
    exit_status = execl("/usr/bin/ntfs-3g", "/usr/bin/ntfs-3g", device, path);
    _exit(-1);
  }
  if (pid > 0) {
    if (exit_status != 0) {
      std::println("ntfs-3g failed to mount ntfs drive {}, reason: {} {}",
                   device, exit_status, strerror(exit_status));
    }
    return {};
  }
  if (pid < 0) {
    return std::unexpected{errno};
  }
  std::unreachable();
}

std::expected<void, int> try_alternate_mount(const char *device,
                                             const char *filesystem,
                                             const char *path) {
  if (std::strcmp(filesystem, "ntfs") == 0) {
    return attempt_ntfs3g_mount(device, path);
  }
  return std::unexpected{ENOTSUP};
}

export std::expected<void, SyscallError>
attempt_mount(const char *device, const char *mount_location,
              const char *filesystem) {
  if (access(device, F_OK) != 0) {
    std::println("No such file: {}", device);
    return std::unexpected{SyscallError("access", EACCES)};
  }

  std::string_view path_sv{device};
  size_t last_slash_idx = path_sv.rfind('/');
  if (last_slash_idx == std::string::npos) {
    return std::unexpected{SyscallError("device path", EINVAL)};
  }

  const mode_t MODE = 666;
  int mkdir_status = mkdir(mount_location, MODE);
  if (mkdir_status < 0) {
    if (errno == EEXIST) {
      std::println("Directory {} already exists... okay...", mount_location);
    } else {
      return std::unexpected{SyscallError("mkdir", errno)};
    }
  }

  std::println("Mounting: {} {} {}", device, mount_location, filesystem);

  if (std::strcmp(filesystem, "ntfs") == 0) {
    auto mount_ntfs_status =
        try_alternate_mount(device, filesystem, mount_location);
    if (mount_ntfs_status.has_value()) {
      std::println("Successfully mounted {} at {} with filesystem {}.", device,
                   mount_location, *filesystem);
      return {};
    } else {
      return std::unexpected{
          SyscallError("ntfs mount", mount_ntfs_status.error())};
    }
  }

  int mount_syscall_status =
      mount(device, mount_location, filesystem, 0, nullptr);
  if (mount_syscall_status < 0) {
    return std::unexpected{SyscallError("mount", errno)};
  }
  return {};
}

export std::expected<void, SyscallError>
attempt_unmount(const char *mountpoint) {
  sync();
  int status = umount2(mountpoint, MNT_DETACH);
  if (status == 0)
    return {};
  LOG::INFO("First unmount attempt failed, waiting a second...");
  sleep(1);
  status = umount2(mountpoint, MNT_DETACH);
  if (status != 0) {
    return std::unexpected{SyscallError("umount", errno)};
  }
  LOG::INFO("Second umount attempt is successful!");
  return {};
}

} // namespace Mount
