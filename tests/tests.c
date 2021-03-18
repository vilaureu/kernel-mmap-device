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
#include <sys/wait.h>
#include <stdlib.h>

#define DEVICE "/dev/kernel-mmap-device"
#define PAGE_SIZE ((size_t)sysconf(_SC_PAGE_SIZE))

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
 * mmap_checked - map a memory region with the given number of pages
 */
void *mmap_checked(unsigned long pages, int prot, int flags, int fd,
		   unsigned long offp)
{
	void *addr = mmap(NULL, pages * PAGE_SIZE, prot, flags, fd,
			  offp * PAGE_SIZE);
	if (addr == MAP_FAILED) {
		printf("mmap failed\n");
	}
	return addr;
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
 * check_page - check that the page is filled with zeros
 */
bool check_page(char *addr)
{
	for (size_t i = 0; i < PAGE_SIZE; i++) {
		if (addr[i] != 'X') {
			printf("byte %ld is not 'X'\n", i);
			return false;
		}
	}
	return true;
}

/*
 * read_fault - expects a read at the address to result in an error
 */
bool read_fault(char *addr)
{
	pid_t pid = fork();
	if (pid < 0) {
		printf("fork failed");
		return false;
	} else if (pid == 0) {
		char c;
		*((volatile char *)&c) = *addr;
		exit(0);
	} else {
		int wstatus;
		waitpid(pid, &wstatus, 0);
		if (wstatus != 0)
			return true;
		printf("read did not result in an error");
		return false;
	}
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
 * test_mmap - test mmap with different PROT_* and MAP_* flags
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

	return result;
}

/*
 * test_read - test reading the mapped memory region
 */
bool test_read(void)
{
	int result = true;

	int fd = open_rdonly();
	if (fd < 0)
		return false;

	char *addr;
	addr = mmap_checked(2, PROT_READ, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		result = false;
		goto unmap;
	}
	if (!check_page(addr))
		result = false;
	if (!read_fault(&addr[PAGE_SIZE]))
		result = false;
unmap:
	if (!unmap(addr, 2))
		result = false;

	addr = mmap_checked(1, PROT_READ, MAP_SHARED, fd, 1);
	if (addr == MAP_FAILED) {
		result = false;
	} else if (!read_fault(addr)) {
		result = false;
	}
	if (!unmap(addr, 1))
		result = false;

	return result;
}

/*
 * test_write - check security of private writable mappings
 */
bool test_write(void)
{
	int result = true;

	int fd = open_rdonly();
	if (fd < 0)
		return false;

	char *addr;
	addr = mmap_checked(1, PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (addr == MAP_FAILED) {
		result = false;
	} else {
		addr[0] = 'Z';
	}
	if (!unmap(addr, 1))
		result = false;

	addr = mmap_checked(1, PROT_READ, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		result = false;
	} else if (!check_page(addr)) {
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
	test_fn(&test_read, "test_read", &result);
	test_fn(&test_write, "test_write", &result);
	return result ? 0 : 1;
}
