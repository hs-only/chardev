# chardev — A Simple Linux Character Device Driver

A minimal Linux kernel module that registers a character device at `/dev/chardev`.
Each time the device is opened and read, it returns a "Hello World" message along
with a counter tracking how many times it has been opened. The device enforces
**exclusive access**: only one process may have it open at a time.

This is a classic teaching example based on *The Linux Kernel Module Programming Guide*.

---

## Features

- Registers a character device with a dynamically allocated major number.
- Automatically creates the device node under `/dev/chardev` via the device class API.
- Returns an incrementing "I already told you N times Hello World!" message on read.
- Enforces single-open access using an atomic compare-and-exchange (`atomic_cmpxchg`).
- Handles kernel-version differences in `class_create()` (kernel ≥ 6.4.0 vs. older).
- Write operations are intentionally unsupported and return `-EINVAL`.

---

## Files

| File         | Purpose                                    |
|--------------|--------------------------------------------|
| `chardev.c`  | The character device driver source.        |
| `Makefile`   | Kbuild-based build script for the module.  |
| `README.md`  | This file.                                 |

---

## Prerequisites

You need a toolchain and the kernel headers matching your running kernel.

**Debian / Ubuntu**
```bash
sudo apt install build-essential linux-headers-$(uname -r)
```

**Fedora / RHEL**
```bash
sudo dnf install kernel-devel kernel-headers gcc make
```

> Root privileges (`sudo`) are required to load, unload, and access the device.

---

## Building

From the directory containing the source and `Makefile`:

```bash
make
```

This produces `chardev.ko`, the loadable kernel module. To clean build artifacts:

```bash
make clean
```

---

## Usage

### 1. Load the module
```bash
sudo insmod chardev.ko
```

### 2. Find the assigned major number
The driver logs the major number it was assigned at load time:
```bash
dmesg | tail
```
You should see lines such as:
```
I was assigned major number 240.
Device created on /dev/chardev
```

### 3. Read from the device
```bash
cat /dev/chardev
```
Example output:
```
I already told you 0 times Hello World!
```
The counter increments on each successful open.

### 4. Writing is not supported
```bash
echo "hi" > /dev/chardev
```
This fails with `Invalid argument` (`-EINVAL`), and a message is logged to the
kernel log:
```
Soory, this operation is not supported.
```

### 5. Unload the module
```bash
sudo rmmod chardev
```
This destroys the device node, removes the class, and unregisters the driver.

---

## How It Works

- **`chardev_init()`** — Registers the driver with `register_chrdev()`, creates a
  device class, and creates the `/dev/chardev` node so it appears automatically
  without a manual `mknod`.
- **`device_open()`** — Uses `atomic_cmpxchg()` to atomically claim the device.
  If it is already open, the call returns `-EBUSY`. Otherwise it formats the
  greeting message with the current counter value.
- **`device_read()`** — Copies the message to user space with `put_user()`,
  tracking the read offset so repeated reads eventually signal end-of-file
  (returns `0`), then resets for the next open.
- **`device_release()`** — Resets the atomic flag so the next process can open
  the device.
- **`device_write()`** — Always returns `-EINVAL`; the device is read-only.
- **`chardev_exit()`** — Tears everything down in reverse order.

---

## Exclusive Access Behavior

Because the driver enforces single-open access, a second process attempting to
open the device while it is already in use will receive `EBUSY`:

```bash
# Terminal 1 (holds the device open)
sleep 60 < /dev/chardev &

# Terminal 2
cat /dev/chardev
# cat: /dev/chardev: Device or resource busy
```

---

## Troubleshooting

- **`missing separator` when running `make`** — Makefile recipe lines must start
  with a real tab character, not spaces.
- **Compilation error around `msg[BUF_LEN+1]`** — Ensure `BUF_LEN` is defined
  with a numeric value in `chardev.c`, e.g. `#define BUF_LEN 80`. An empty
  definition will not compile.
- **`insmod: ERROR: could not insert module`** — Confirm the kernel headers match
  your running kernel (`uname -r`) and rebuild.
- **No `/dev/chardev` node** — Check `dmesg` for errors during class or device
  creation.

---

## Notes

- The dynamically assigned major number may differ each time the module is loaded.
- This driver is intended for learning and experimentation, not production use.

---

## License

Licensed under the **GPL** (as declared by `MODULE_LICENSE("GPL")` in the source).
