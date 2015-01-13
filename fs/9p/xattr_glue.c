#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <net/9p/9p.h>
#include <net/9p/client.h>

#include "fid.h"
#include "xattr.h"


static const char *
strcmp_prefix(const char *a, const char *a_prefix)
{
	while (*a_prefix && *a == *a_prefix) {
		a++;
		a_prefix++;
	}
	return *a_prefix ? NULL : a;
}

/*
 * In order to implement different sets of xattr operations for each xattr
 * prefix with the v9fs_vfs xattr API, a filesystem should create a
 * null-terminated array of struct xattr_handler (one for each prefix) and
 * hang a pointer to it off of the s_xattr field of the superblock.
 *
 * The v9fs_vfs_fooxattr() functions will use this list to dispatch xattr
 * operations to the correct xattr_handler.
 */
#define for_each_xattr_handler(handlers, handler)		\
		for ((handler) = *(handlers)++;			\
			(handler) != NULL;			\
			(handler) = *(handlers)++)

/*
 * Find the xattr_handler with the matching prefix.
 */
static struct compat_xattr_handler *
xattr_resolve_name(const char **name)
{
	struct compat_xattr_handler **handlers = v9fs_xattr_handlers;
	struct compat_xattr_handler *handler;

	if (!*name)
		return NULL;

	for_each_xattr_handler(handlers, handler) {
		const char *n = strcmp_prefix(*name, handler->prefix);
		if (n) {
			*name = n;
			break;
		}
	}
	return handler;
}

/*
 * Find the handler for the prefix and dispatch its get() operation.
 */
ssize_t
v9fs_vfs_getxattr(struct dentry *dentry, const char *name, void *buffer, size_t size)
{
	struct compat_xattr_handler *handler;

	handler = xattr_resolve_name(&name);
	if (!handler)
		return -EOPNOTSUPP;
	return handler->get(dentry, name, buffer, size, handler->flags);
}

/*
 * Combine the results of the list() operation from every xattr_handler in the
 * list.
 */
ssize_t
v9fs_vfs_listxattr(struct dentry *dentry, char *buffer, size_t buffer_size)
{
	struct compat_xattr_handler *handler;
	struct compat_xattr_handler **handlers = v9fs_xattr_handlers;
	unsigned int size = 0;

	if (!buffer) {
		for_each_xattr_handler(handlers, handler)
			size += handler->list(dentry, NULL, 0, NULL, 0, handler->flags);
	} else {
		char *buf = buffer;

		for_each_xattr_handler(handlers, handler) {
			size = handler->list(dentry, buf, buffer_size, NULL, 0, handler->flags);
			if (size > buffer_size)
				return -ERANGE;
			buf += size;
			buffer_size -= size;
		}
		size = buf - buffer;
	}
	return size;
}

/*
 * Find the handler for the prefix and dispatch its set() operation.
 */
int
v9fs_vfs_setxattr(struct dentry *dentry, const char *name, const void *value, size_t size, int flags)
{
	struct compat_xattr_handler *handler;

	if (size == 0)
		value = "";  /* empty EA, do not remove */
	handler = xattr_resolve_name(&name);
	if (!handler)
		return -EOPNOTSUPP;
	return handler->set(dentry, name, value, size, flags, handler->flags);
}

/*
 * Find the handler for the prefix and dispatch its set() operation to remove
 * any associated extended attribute.
 */
int
v9fs_vfs_removexattr(struct dentry *dentry, const char *name)
{
	struct compat_xattr_handler *handler;

	handler = xattr_resolve_name(&name);
	if (!handler)
		return -EOPNOTSUPP;
	return handler->set(dentry, name, NULL, 0, XATTR_REPLACE, handler->flags);
}


