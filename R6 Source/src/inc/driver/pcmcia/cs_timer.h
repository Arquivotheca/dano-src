#define HZ		1000000
#define jiffies		(system_time())
#define RUN_AT(t)	(system_time()+(t))

typedef struct timer_list {
    struct timer_list *next, *prev;
    bigtime_t expires;
    u_long data;
    void (*function)(u_long data);
} timer_list;

void my_add_timer(struct timer_list *t);
void my_del_timer(struct timer_list *t);
