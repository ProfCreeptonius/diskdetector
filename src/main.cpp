#include <expected>
#include <format>

import Netlink;
import Logging;
import Mount;
import Executor;
import Filesystem;

static std::string currently_mounted_path{};

std::string_view get_device_name_from_path(std::string_view path) {
  size_t last_slash_idx = path.rfind('/');
  if (last_slash_idx == std::string::npos) {
    return "";
  }
  return std::string_view(path.data() + last_slash_idx + 1);
}

void handle_added_device(std::string_view device_path) {
  // If there's still something mounted
  if (!currently_mounted_path.empty()) {
    if (auto umount_status =
            Mount::attempt_unmount(currently_mounted_path.c_str());
        !umount_status.has_value()) {
      LOG::WARN(
          std::format("Failed to unmount device: {}", currently_mounted_path)
              .c_str());
      LOG::WARN(umount_status.error().what());
      currently_mounted_path.clear();
    }
  }

  auto filesystem = Filesystem::detect_filesystem(device_path.data());
  if (!filesystem.has_value()) {
    LOG::WARN(
        std::format("Unable to detect filesystem on device: {}", device_path)
            .c_str());
    LOG::WARN(filesystem.error().what());
    return;
  }

  std::string to_mount_path = std::format(
      "/run/media/mount_{}", get_device_name_from_path(device_path));
  auto mount_status = Mount::attempt_mount(device_path.data(),
                                           to_mount_path.data(), *filesystem);
  if (!mount_status.has_value()) {
    LOG::WARN("Failed to mount device");
    LOG::WARN(mount_status.error().what());
    return;
  }
  currently_mounted_path = to_mount_path;

  auto key_check_status =
      Executor::run_keyfile_check(currently_mounted_path.c_str());
  if (!key_check_status.has_value()) {
    LOG::WARN("Failed to execute key check script! Reason:");
    LOG::WARN(key_check_status.error().what());
    return;
  }

  if (!*key_check_status) {
    LOG::INFO(std::format("Refusing to run script on device {}, keycheck "
                          "returned non-zero exit code.",
                          currently_mounted_path)
                  .c_str());
    LOG::INFO(device_path.data());
    return;
  }

  LOG::INFO("Running script.");
  auto script_status =
      Executor::run_script_on_mountpoint(currently_mounted_path.c_str());
  if (!key_check_status.has_value()) {
    LOG::WARN("Script failed.");
    LOG::WARN(key_check_status.error().what());
  }

  LOG::INFO(std::format("Successfully ran script on device, unmounting: {}",
                        currently_mounted_path)
                .c_str());
  auto umount_status = Mount::attempt_unmount(currently_mounted_path.c_str());
  if (!umount_status.has_value()) {
    LOG::WARN("Failed to unmount device.");
    LOG::WARN(umount_status.error().what());
    return;
  }
  currently_mounted_path.clear();
}

int main() {
  NetlinkSocket netlink_sock{};
  auto init_status = netlink_sock.initialize();
  if (!init_status.has_value()) {
    LOG::ERROR("Failed to initialize netlink socket!");
    return -1;
  }
  LOG::INFO("Listening for devices...");
  while (true) {
    std::expected message = netlink_sock.next_message();
    if (!message.has_value()) {
      LOG::WARN("Failed to receive netlink message!");
      LOG::WARN(message.error()->what());
      continue;
    }
    std::string dev = NetlinkSocket::find_added_device_in_message(*message);
    if (dev.empty()) {
      continue;
    }
    LOG::INFO("Detected device:");
    LOG::INFO(dev.c_str());
    handle_added_device(dev);
  }
}
