/*
 *  linux/fs/9p/vfs_addr.c
 *
 * This file contians vfs address (mmap) ops for 9P2000.
 *
 *  Copyright (C) 2005 by Eric Van Hensbergen <ericvh@gmail.com>
 *  Copyright (C) 2002 by Ron Minnich <rminnich@lanl.gov>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to:
 *  Free Software Foundation
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02111-1301  USA
 *
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/inet.h>
#include <linux/pagemap.h>
#include <linux/idr.h>
#include <linux/sched.h>
#include <net/9p/9p.h>
#include <net/9p/client.h>

#include "v9fs.h"
#include "v9fs_vfs.h"
#include "cache.h"

/*
static void v9fs_vfs_readpage_async_cb(struct page *page, char *data, int len)
{
	struct inode *inode = page->mapping->host;
	char *buffer;

	if (len < 0) {
		v9fs_uncache_page(inode, page);
		SetPageError(page);
		goto done;
	}

	buffer = kmap(page);
	memcpy(buffer, data, len);
	memset(buffer + len, 0, PAGE_CACHE_SIZE - len);
	kunmap(page);

	flush_dcache_page(page);
	SetPageUptodate(page);

	v9fs_readpage_to_fscache(inode, page);
done:
	unlock_page(page);
}
*/


static int v9fs_vfs_readpage_async(struct file *filp, struct page *page)
{
	return -ENOSYS;
/*

	int retval;
	loff_t offset;
	struct inode *inode;
	struct p9_fid *fid;

	inode = page->mapping->host;
	fid = filp->private_data;

	offset = page_offset(page);

	retval = p9_client_readpage(fid, offset, page, PAGE_CACHE_SIZE,
				    v9fs_vfs_readpage_async_cb);
	if (retval < 0) {
		v9fs_uncache_page(inode, page);
		unlock_page(page);
	}

	return retval;
	*/
}

/**
 * v9fs_fid_readn - read from a fid
 * @fid: fid to read
 * @data: data buffer to read data into
 * @udata: user data buffer to read data into
 * @count: size of buffer
 * @offset: offset at which to read data
 *
 */
ssize_t
v9fs_fid_readn(struct p9_fid *fid, char *data, char __user *udata, u32 count,
	       u64 offset)
{
	int n, total, size;

	p9_debug(P9_DEBUG_VFS, "fid %d offset %llu count %d\n",
		 fid->fid, (long long unsigned)offset, count);
	n = 0;
	total = 0;
	size = fid->iounit ? fid->iounit : fid->clnt->msize - P9_IOHDRSZ;
	do {
		n = p9_client_read(fid, data, udata, offset, count);
		if (n <= 0)
			break;

		if (data)
			data += n;
		if (udata)
			udata += n;

		offset += n;
		count -= n;
		total += n;
	} while (count > 0 && n == size);

	if (n < 0)
		total = n;

	return total;
}

/**
 * v9fs_fid_writen - write from a fid
 * @fid: fid to write
 * @data: data buffer to write data from
 * @udata: user data buffer to write data from
 * @count: size of buffer
 * @offset: offset at which to write data
 *
 */
ssize_t
v9fs_fid_writen(struct p9_fid *fid, const char *data, const char __user *udata, u32 count,
	       u64 offset)
{
	int n, total, size;

	p9_debug(P9_DEBUG_VFS, "fid %d offset %llu count %d\n",
		 fid->fid, (long long unsigned)offset, count);
	n = 0;
	total = 0;
	size = fid->iounit ? fid->iounit : fid->clnt->msize - P9_IOHDRSZ;
	do {
		n = p9_client_write(fid, (char*) data, (char*)udata, offset, count);
		if (n <= 0)
			break;

		if (data)
			data += n;
		if (udata)
			udata += n;

		offset += n;
		count -= n;
		total += n;
	} while (count > 0 && n == size);

	if (n < 0)
		total = n;

	return total;
}

static int v9fs_vfs_readpage_sync(struct file *filp, struct page *page)
{
	int retval;
	loff_t offset;
	char *buffer;
	struct inode *inode;
	struct p9_fid *fid;

	inode = page->mapping->host;
	fid = filp->private_data;

	buffer = kmap(page);
	offset = page_offset(page);

	retval = v9fs_fid_readn(fid, buffer, NULL, PAGE_CACHE_SIZE, offset);
	if (retval < 0) {
		v9fs_uncache_page(inode, page);
		goto done;
	}

	memset(buffer + retval, 0, PAGE_CACHE_SIZE - retval);
	flush_dcache_page(page);
	SetPageUptodate(page);

	v9fs_readpage_to_fscache(inode, page);
	retval = 0;

done:
	kunmap(page);
	unlock_page(page);
	return retval;
}

/**
 * v9fs_vfs_readpage - read an entire page in from 9P
 *
 * @filp: file being read
 * @page: structure to page
 *
 */

static int v9fs_vfs_readpage(struct file *filp, struct page *page)
{
	int retval;
	struct inode *inode;
	struct p9_fid *fid;
	struct v9fs_session_info *v9ses;

	inode = page->mapping->host;
	P9_DPRINTK(P9_DEBUG_VFS, "\n");

	BUG_ON(!PageLocked(page));

	retval = v9fs_readpage_from_fscache(inode, page);
	if (retval == 0)
		return retval;

	v9ses = v9fs_inode2v9ses(inode);
	fid = filp->private_data;
	if (!v9ses->asyncreadpage
			|| fid->clnt->msize - P9_IOHDRSZ < PAGE_CACHE_SIZE
			|| (fid->iounit > 0 && fid->iounit < PAGE_CACHE_SIZE))
		retval = v9fs_vfs_readpage_sync(filp, page);
	else
		retval = v9fs_vfs_readpage_async(filp, page);

	return retval;
}

/**
 * v9fs_vfs_readpages - read a set of pages from 9P
 *
 * @filp: file being read
 * @mapping: the address space
 * @pages: list of pages to read
 * @nr_pages: count of pages to read
 *
 */

static int v9fs_vfs_readpages(struct file *filp, struct address_space *mapping,
			     struct list_head *pages, unsigned nr_pages)
{
	int ret = 0;
	struct inode *inode;

	inode = mapping->host;
	P9_DPRINTK(P9_DEBUG_VFS, "inode: %p file: %p\n", inode, filp);

	ret = v9fs_readpages_from_fscache(inode, mapping, pages, &nr_pages);
	if (ret == 0)
		return ret;

	ret = read_cache_pages(mapping, pages, (void *)v9fs_vfs_readpage, filp);
	P9_DPRINTK(P9_DEBUG_VFS, "  = %d\n", ret);
	return ret;
}

/**
 * v9fs_release_page - release the private state associated with a page
 *
 * Returns 1 if the page can be released, false otherwise.
 */

static int v9fs_release_page(struct page *page, gfp_t gfp)
{
	if (PagePrivate(page))
		return 0;

	return v9fs_fscache_release_page(page, gfp);
}

/**
 * v9fs_invalidate_page - Invalidate a page completely or partially
 *
 * @page: structure to page
 * @offset: offset in the page
 */

static void v9fs_invalidate_page(struct page *page, unsigned long offset)
{
	if (offset == 0)
		v9fs_fscache_invalidate_page(page);
}

/**
 * v9fs_launder_page - Writeback a dirty page
 * Since the writes go directly to the server, we simply return a 0
 * here to indicate success.
 *
 * Returns 0 on success.
 */

static int v9fs_launder_page(struct page *page)
{
	return 0;
}

/**
 * v9fs_direct_IO - 9P address space operation for direct I/O
 * @rw: direction (read or write)
 * @iocb: target I/O control block
 * @iov: array of vectors that define I/O buffer
 * @pos: offset in file to begin the operation
 * @nr_segs: size of iovec array
 *
 * The presence of v9fs_direct_IO() in the address space ops vector
 * allowes open() O_DIRECT flags which would have failed otherwise.
 *
 * In the non-cached mode, we shunt off direct read and write requests before
 * the VFS gets them, so this method should never be called.
 *
 * Direct IO is not 'yet' supported in the cached mode. Hence when
 * this routine is called through generic_file_aio_read(), the read/write fails
 * with an error.
 *
 */
ssize_t v9fs_direct_IO(int rw, struct kiocb *iocb, const struct iovec *iov,
		loff_t pos, unsigned long nr_segs)
{
	P9_DPRINTK(P9_DEBUG_VFS, "v9fs_direct_IO: v9fs_direct_IO (%s) "
			"off/no(%lld/%lu) EINVAL\n",
			iocb->ki_filp->f_path.dentry->d_name.name,
			(long long) pos, nr_segs);

	return -EINVAL;
}
const struct address_space_operations v9fs_addr_operations = {
      .readpage = v9fs_vfs_readpage,
      .readpages = v9fs_vfs_readpages,
      .releasepage = v9fs_release_page,
      .invalidatepage = v9fs_invalidate_page,
      .launder_page = v9fs_launder_page,
      .direct_IO = v9fs_direct_IO,
};
