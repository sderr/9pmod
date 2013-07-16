
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <net/9p/9p.h>
#include <linux/proc_fs.h>
#include <net/9p/client.h>
#include <net/9p/transport.h>
#include "protocol.h"
#include "stats.h"



DEFINE_PER_CPU(unsigned long, reqtime);
DEFINE_PER_CPU(unsigned long, nr_req);

static struct proc_dir_entry *dir;

void p9stat_record(long delta_us)
{
	get_cpu_var(reqtime) += delta_us;
	put_cpu_var(reqtime);
	get_cpu_var(nr_req)++;
	put_cpu_var(nr_req);
}


static int stats_show(struct seq_file *m, void *v)
{
	int j;
	unsigned long total_reqtime = 0;
	unsigned long total_nr_req = 0;

	for_each_online_cpu(j) {
		total_reqtime += per_cpu(reqtime, j);
		total_nr_req += per_cpu(nr_req, j);
	}

	seq_printf(m, "Requests: %lu\n", total_nr_req);
	seq_printf(m, "Time (us): %lu\n", total_reqtime);
	return 0;
}

static int stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, stats_show, PDE(inode)->data);
}

static const struct file_operations stats_fops = {
	.open		= stats_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

void stats_init(void)
{
	/* create /proc/net/9p */
	dir = proc_mkdir("fs/9p", NULL);
	if (!dir) {
		printk("Could not create 9p directory.\n");
		return;
	}
	printk("9p directory OK.\n");

	proc_create("stats", 0600, dir, &stats_fops);
}

void stats_exit(void)
{
	remove_proc_entry("stats", dir);
	remove_proc_entry("fs/9p", NULL);
}

