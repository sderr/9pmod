

void p9stat_record(long delta_us);

static inline void p9stat_enter(ktime_t *t)
{
	*t = ktime_get();
}

static inline void p9stat_leave(ktime_t *t)
{
	ktime_t end = ktime_get();
	s64 delta = ktime_us_delta(end, *t);

	p9stat_record(delta);
}

void stats_init(void);
void stats_exit(void);

