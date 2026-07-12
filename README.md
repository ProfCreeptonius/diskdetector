# Diskdetector

Diskdetector is a Linux daemon which:
- Detects when a disk is plugged into a device.
- Mounts the disk to `/run/media/mount_{disk}`.
- Calls the executable `keycheck` with the mount path as the first argument.
- If `keycheck` returns `0`, then runs the executable `onmount` with the mount path as first argument.
- Unmounts the disk.


Customize the behavior of diskdetector by providing your own `keycheck` and `onmount`.

## Dependencies:

None, notably also not systemd.

## Notes:
- The executables `keycheck` and `onmount` should be in the same directory as diskdetector.
- If the executables finish too quickly, the umount system call might fail with `EINVAL`, so diskdetector will retry after one second.

## Known limitations:

Only capable of handling one disk at a time.

## Build:

Requires `libblkid-static`, which depends on `libeconf-static`, and a C++23 capable compiler with C++ modules support. 
*Note:* On Fedora 44, `clang` and `Ninja` are sufficiently up-to-date. However, `dnf` does not provide `libblkid-static`. You may opt to either not build statically, or get the package from the OpenSuse repositories, and tell `ld` where to find the libraries.




