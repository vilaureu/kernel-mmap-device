// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * tests.c - perform some simple tests on /dev/kernel-mmap-device
 */

#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#define DEVICE "/dev/kernel-mmap-device"

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
 * test_open - test different O_* flags
 */
bool test_open()
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

int main()
{
	bool result = true;
	test_fn(&test_open, "test_open", &result);
	return result ? 0 : 1;
}
