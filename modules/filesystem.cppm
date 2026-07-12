module;
#include <blkid/blkid.h>
#include <cstring>
#include <expected>
#include <memory>
#include <print>

export module Filesystem;

import Error;
import Logging;

namespace Filesystem {
export std::expected<const char *, SyscallError>
detect_filesystem(const char *device_file_path) {
  blkid_probe probe = blkid_new_probe_from_filename(device_file_path);
  std::unique_ptr<blkid_probe, decltype([](blkid_probe *probe) {
                    blkid_free_probe(*probe);
                  })>
      probe_cleanup(&probe);
  if (!probe) {
    return std::unexpected{
        SyscallError("blkid_new_probe_from_filename", errno)};
  }
  int superblock_status = blkid_probe_enable_superblocks(probe, true);
  if (superblock_status != 0) {
      return std::unexpected{SyscallError("blkid_probe_enable_superblocks", errno)};
  }

  int status = blkid_do_fullprobe(probe);
  if (status < 0) {
    return std::unexpected{SyscallError("blkid_do_fullprobe", errno)};
  }
  if (status == 1) {
      LOG::WARN("Blkid detected nothing!");
  }
  // This is by far longer than the longest filesystem supported by linux
  // kernel, so this should be safe.
  static char filesystem_buf[256];
  std::memset(filesystem_buf, 0, 256);

  const char *filesystem = nullptr;
  size_t return_len = 0;
  status = blkid_probe_lookup_value(probe, "TYPE", &filesystem, &return_len);
  if (status < 0 || filesystem == nullptr) {
    return std::unexpected{SyscallError("blkid_probe_lookup_value", errno)};
  }
  // For some bizzarre reason, cleaning up the probe also deletes the filesystem string, so we have to copy it over...
  std::strcpy(filesystem_buf, filesystem);
  return filesystem_buf;
}
} // namespace Filesystem
