// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * kernel-mmap-device.c - an example device driver that allows to mmap a single kernel accessible page
 */

#include <linux/mm_types.h>
#include <uapi/asm-generic/errno-base.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/mm.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(
	"An example device driver that allows to mmap a single kernel accessible page");

#define KMD_DEVICE_NAME "kernel-mmap-device"

static dev_t kmd_dev;
static struct cdev *kmd_cdev;
static struct class *kmd_class;
static struct device *kmd_device;
static struct page *kmd_page;

/*
 * kmd_open() - open the file only non-writable
 */
static int kmd_open(struct inode *inode, struct file *file)
{
	if (file->f_mode & FMODE_WRITE) {
		return -EACCES;
	}

	return 0;
}

/*
 * kmd_fault - handle the page faults to the mapped device
 *
 * The shared kernel page can only be mapped to the first page in the vma.
 * All other accesses to the vma result in VM_FAULT_SIGBUS.
 */
vm_fault_t kmd_fault(struct vm_fault *vmf)
{
	if (vmf->pgoff > 0)
		return VM_FAULT_SIGBUS;

	vmf->page = kmd_page;
	get_page(vmf->page);

	return 0;
}

/*
 * kmd_mmap - handle the mmap systemcall for this device
 */
static int kmd_mmap(struct file *file, struct vm_area_struct *vma)
{
	static struct vm_operations_struct vm_ops = { .fault = kmd_fault };

	// TODO: protect from private writable mappings
	// pr_info("kmd: vm_flags 0x%lx", vma->vm_flags);

	// WARN_ON(vma->vm_flags & VM_WRITE);

	// vma->vm_flags &= VM_DENYWRITE;
	vma->vm_ops = &vm_ops;
	return 0;
}

/*
 * kmd_init() - initialize this module
 * 
 * Add one "kernel-mmap-device" character device.
 */
static int __init kmd_init(void)
{
	int result;
	static struct file_operations fops = { .owner = THIS_MODULE,
					       .open = kmd_open,
					       .mmap = kmd_mmap };

	kmd_page = alloc_page(GFP_KERNEL & __GFP_ZERO);
	if (kmd_page == NULL) {
		pr_warn("kmd: can't allocate page");
		result = -ENOMEM;
		goto fail_alloc_page;
	}

	// Allocate one character device number with dynamic major number
	result = alloc_chrdev_region(&kmd_dev, 0, 1, KMD_DEVICE_NAME);
	if (result < 0) {
		pr_warn("kmd: can't allocate chrdev region");
		goto fail_chrdev;
	}

	// Allocate character device struct
	kmd_cdev = cdev_alloc();
	if (kmd_cdev == NULL) {
		pr_warn("kmd: can't allocate struct cdev");
		result = -ENOMEM;
		goto fail_cdev_alloc;
	}
	kmd_cdev->owner = THIS_MODULE;
	kmd_cdev->ops = &fops;

	// Add the character device to the kernel
	result = cdev_add(kmd_cdev, kmd_dev, 1);
	if (result < 0) {
		pr_warn("kmd: can't add character device");
		goto fail_cdev_add;
	}

	kmd_class = class_create(THIS_MODULE, KMD_DEVICE_NAME);
	if (IS_ERR_VALUE(kmd_class)) {
		pr_warn("kmd: can't create class");
		result = PTR_ERR(kmd_class);
		goto fail_class_create;
	}

	kmd_device =
		device_create(kmd_class, NULL, kmd_dev, NULL, KMD_DEVICE_NAME);
	if (IS_ERR_VALUE(kmd_device)) {
		pr_warn("kmd: can't create device");
		result = PTR_ERR(kmd_device);
		goto fail_device_create;
	}

	return 0;

	// Error handing
fail_device_create:
	class_destroy(kmd_class);
fail_class_create:
fail_cdev_add:
	cdev_del(kmd_cdev);
fail_cdev_alloc:
	unregister_chrdev_region(kmd_dev, 1);
fail_chrdev:
	__free_pages(kmd_page, 0);
fail_alloc_page:
	return result;
}

static void __exit kmd_exit(void)
{
	device_destroy(kmd_class, kmd_dev);
	class_destroy(kmd_class);
	cdev_del(kmd_cdev);
	unregister_chrdev_region(kmd_dev, 1);
	__free_pages(kmd_page, 0);
}

module_init(kmd_init);
module_exit(kmd_exit);
