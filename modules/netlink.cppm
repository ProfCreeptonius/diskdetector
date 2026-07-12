module;
#include <cstring>
#include <expected>
#include <memory>
#include <print>

#include <asm-generic/socket.h>
#include <linux/netlink.h>
#include <sys/socket.h>
#include <unistd.h>
export module Netlink;

import Error;

export struct NetlinkSocket {
  NetlinkSocket() {}
  std::expected<void, SyscallError> initialize();
  std::expected<std::string, std::unique_ptr<Exception>> next_message();

  static std::string find_added_device_in_message(std::string_view message);

private:
  int netlink_sockfd = -1;
  size_t rcvbuf_size = -1;
  std::unique_ptr<char[]> temp_buffer = nullptr;
  size_t temp_buffer_size = 0;
};

std::expected<void, SyscallError> NetlinkSocket::initialize() {
  if (netlink_sockfd > 0) {
    close(netlink_sockfd);
  }
  netlink_sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
  if (netlink_sockfd < 0)
    return std::unexpected{SyscallError("socket", errno)};

  struct sockaddr_nl netlink_addr;
  std::memset(&netlink_addr, 0, sizeof(netlink_addr));

  netlink_addr.nl_family = AF_NETLINK;
  netlink_addr.nl_pid = getpid();
  netlink_addr.nl_groups = (unsigned)-1; // Mask to listen to everything

  int status = bind(netlink_sockfd, (struct sockaddr *)&netlink_addr,
                    sizeof(netlink_addr));
  if (status < 0) {
    close(netlink_sockfd);
    netlink_sockfd = -1;
    return std::unexpected{SyscallError("bind", errno)};
  }

  rcvbuf_size = 0;
  socklen_t optlen = sizeof(rcvbuf_size);
  status =
      getsockopt(netlink_sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf_size, &optlen);
  if (status < 0) {
    rcvbuf_size = -1;
    close(netlink_sockfd);
    netlink_sockfd = -1;
    return std::unexpected{SyscallError("getsockopt", errno)};
  }

  return {};
}

std::expected<std::string, std::unique_ptr<Exception>> NetlinkSocket::next_message() {
  // Guard against allocation too large.
  if (rcvbuf_size > 1024 * 1024) {
    return std::unexpected{std::make_unique<LimitsError>("Buffer size", 1024 * 1024, rcvbuf_size)};
  }

  if (temp_buffer_size < rcvbuf_size + 1) {
    temp_buffer = std::make_unique<char[]>(rcvbuf_size + 1);
    temp_buffer_size = rcvbuf_size + 1;
  }
  std::memset(temp_buffer.get(), 0, rcvbuf_size + 1);

  ssize_t bytes_read = read(netlink_sockfd, temp_buffer.get(), rcvbuf_size);
  if (bytes_read < 0) {
    return std::unexpected{std::make_unique<SyscallError>("read", errno)};
  }

  return std::string{temp_buffer.get()};
}


std::string NetlinkSocket::find_added_device_in_message(std::string_view message) {
  std::string result = "/dev/";
  const char *block = "/block/";
  size_t previous_pos = message.find("add@");
  if (previous_pos == std::string::npos)
    return {};
  previous_pos = message.find(block, previous_pos + 1);
  if (previous_pos == std::string::npos)
    return {};
  size_t last_slash = message.rfind('/');
  if (last_slash == std::string::npos) {
    return {};
  }
  result.append(message.data() + last_slash + 1);
  return result;
}
