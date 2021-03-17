// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * kernel-mmap-device.c - an example device driver that allows to mmap a single kernel accessible page
 */

#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(
	"An example device driver that allows to mmap a single kernel accessible page");

static int __init kmd_init(void)
{
	return 0;
}

static void __exit kmd_exit(void)
{
}

module_init(kmd_init);
module_exit(kmd_exit);
