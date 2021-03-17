// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * tests.c - perform some simple tests on /dev/kernel-mmap-device
 */

#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>

#define DEVICE "/dev/kernel-mmap-device"
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)

/*
 * test_fn - small wrapper function for test functions
 */
void test_fn(bool (*fn)(void), const char *name, bool *result)
{
	printf("----- testing %s\n", name);
	if (fn()) {
		printf("-ok-- %s\n", name);
	} else {
		printf("-err- %s\n", name);
		*result = false;
	}
}

/*
 * open_rdonly - try to open the device read-only
 */
int open_rdonly(void)
{
	int fd = open(DEVICE, O_RDONLY);
	if (fd < 0)
		printf("open failed\n");
	return fd;
}

/*
 * unmap - unmap a memory region with the given number of pages
 */
bool unmap(void *addr, unsigned long pages)
{
	if (addr == MAP_FAILED)
		return true;

	if (munmap(addr, pages * PAGE_SIZE) != 0) {
		printf("can't munmap\n");
		return false;
	}
	return true;
}

/*
 * test_open - test different O_* flags
 */
bool test_open(void)
{
	int fd;
	bool result = true;

	fd = open(DEVICE, O_RDONLY);
	if (fd < 0) {
		printf("open with O_RDONLY fails\n");
		result = false;
	}

	fd = open(DEVICE, O_WRONLY);
	if (fd >= 0) {
		printf("open with O_WRONLY succeeds\n");
		result = false;
	} else if (errno != EACCES) {
		printf("open with O_WRONLY did not result in EACCES\n");
		result = false;
	}

	fd = open(DEVICE, O_RDWR);
	if (fd >= 0) {
		printf("open with O_RDWR succeeds\n");
		result = false;
	} else if (errno != EACCES) {
		printf("open with O_RDWR did not result in EACCES\n");
		result = false;
	}

	return result;
}

/*
 * test_mmap test mmap with different PROT_* and MAP_* flags
 */
bool test_mmap(void)
{
	int result = true;

	int fd = open_rdonly();
	if (fd < 0)
		return false;

	void *addr;
	addr = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		printf("mmap with PROT_READ failed\n");
		result = false;
	}
	if (!unmap(addr, 1))
		result = false;

	addr = mmap(NULL, PAGE_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr != MAP_FAILED) {
		printf("mmap with PROT_WRITE and MAP_SHARED succeeded\n");
		result = false;
	}
	if (!unmap(addr, 1))
		result = false;

	// TODO: is this a problem?
	addr = mmap(NULL, PAGE_SIZE, PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (addr != MAP_FAILED) {
		printf("mmap with PROT_WRITE and MAP_PRIVATE succeeded\n");
		result = false;
	}
	if (!unmap(addr, 1))
		result = false;

	return result;
}

int main(void)
{
	bool result = true;
	test_fn(&test_open, "test_open", &result);
	test_fn(&test_mmap, "test_mmap", &result);
	return result ? 0 : 1;
}
