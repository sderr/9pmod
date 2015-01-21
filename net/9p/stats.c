
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <net/9p/9p.h>
#include <linux/proc_fs.h>
#include <net/9p/client.h>
#include <net/9p/transport.h>
#include "protocol.h"
#include "stats.h"

/* We build two histograms:
 * - duration of 9p requests. This one has a logarithmic scale.
 * - number of active requests. This one has a linear scale.
 */

#define TSLOTS 10 /* slots for the time histogram */
#define TSHIFT 8  /* bit shift for time histogram i.e. size of first bucket */
#define ASLOTS 12 /* slots for the active requests histogram */
#define ASTEP 3   /* size of each bucket */
 
DEFINE_PER_CPU(unsigned long, reqtime);
DEFINE_PER_CPU(unsigned long, nr_req);
DEFINE_PER_CPU(unsigned long[TSLOTS], time_hist);
DEFINE_PER_CPU(unsigned long[ASLOTS], ar_hist);

atomic_t active_requests;

static struct proc_dir_entry *dir;

void p9stat_record(long delta_us, int active_req)
{
	int t_slot;
	int a_slot;
	
	get_cpu_var(reqtime) += delta_us;
	put_cpu_var(reqtime);
	get_cpu_var(nr_req)++;
	put_cpu_var(nr_req);

	t_slot = max(0, min_t(int, TSLOTS-1, fls(delta_us) - TSHIFT));
	a_slot = min(ASLOTS-1, active_req/ASTEP);

	get_cpu_var(time_hist)[t_slot]++;
	put_cpu_var(time_hist);

	get_cpu_var(ar_hist)[a_slot]++;
	put_cpu_var(ar_hist);
}


void p9stat_enter(ktime_t *t)
{
	*t = ktime_get();
	atomic_inc(&active_requests);
}

void p9stat_leave(ktime_t *t)
{
	int ar = atomic_read(&active_requests);
	ktime_t end = ktime_get();
	s64 delta = ktime_us_delta(end, *t);

	atomic_dec(&active_requests);
	p9stat_record(delta, ar);
}

static int stats_show(struct seq_file *m, void *v)
{
	int j;
	int i;
	unsigned long total_reqtime = 0;
	unsigned long total_nr_req = 0;
        char tmp[64];

	for_each_online_cpu(j) {
		total_reqtime += per_cpu(reqtime, j);
		total_nr_req += per_cpu(nr_req, j);
	}

	seq_printf(m, "Total requests: %lu\n", total_nr_req);
	seq_printf(m, "Total time (us): %lu\n", total_reqtime);
	seq_printf(m, "Request duration (us):\n");

	for(i = 0; i < TSLOTS; i++) {
		int total = 0;
		for_each_online_cpu(j)
			total += per_cpu(time_hist, j)[i];
		if (i < TSLOTS - 1)
			snprintf(tmp, 64, "%d-%d",
				 i ? 1<<(i+TSHIFT-1) : 0,
				 (1<<(i+TSHIFT))-1);
		else
			snprintf(tmp, 64, "%d-",
				 1<<(i+TSHIFT-1));
		seq_printf(m, "   %12s : %d\n", tmp, total);
	}

	seq_printf(m, "Active requests:\n");
	for(i = 0; i < ASLOTS; i++) {
		int total = 0;
		for_each_online_cpu(j)
			total += per_cpu(ar_hist, j)[i];
		if (i < ASLOTS - 1)
			snprintf(tmp, 64, "%d-%d",
				 i*ASTEP, (i+1) * ASTEP - 1);
		else
			snprintf(tmp, 64, "%d-", i*ASTEP);
		seq_printf(m, "   %12s : %d\n", tmp, total);
	}


	return 0;
}

static int stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, stats_show, PDE_DATA(inode));
}

static const struct file_operations stats_fops = {
	.open		= stats_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

void stats_init(void)
{
	/* create /proc/fs/9p */
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

