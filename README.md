# kernel-mmap-device
An example device driver that allows to mmap a single kernel accessible page

## Build
Tested with Linux v5.11.

```bash
make -C /path/to/kernel/source M=`pwd` modules
```

## Install
```bash
insmod kernel-mmap-device.c
```

## Usage
A character device is created at `/dev/kernel-mmap-device`

*TODO*

## Licence
GPLv2 or later
