#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>
#include <linux/ktime.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Canty");
MODULE_DESCRIPTION("Timer Kernel Module");

#define ENTRY_NAME "my_timer"
#define PERMS 0644
#define PARENT NULL

static struct proc_dir_entry *timer_entry;
static ktime_t lastCall;
int firstCall = 1; 

static ssize_t timer_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    ktime_t ts_now;
    char buf[256];
    int len = 0;

    ts_now = ktime_get_real();
    ts_now = ktime_to_ns(ts_now);

    len = snprintf(buf, sizeof(buf), "current time: %lld.%09ld\n", (long long)(ts_now / NSEC_PER_SEC), (long)(ts_now % NSEC_PER_SEC));

    if (firstCall) { 
        firstCall = 0; 
    } else {
        ktime_t elapsed = ktime_sub(ts_now, lastCall);
        long long elapsed_ns = ktime_to_ns(elapsed);
        if (elapsed_ns >= 1000000) { 
            len += snprintf(buf + len, sizeof(buf) - len, "elapsed time: %lld.%09ld\n", (long long)(elapsed_ns / NSEC_PER_SEC), (long)(elapsed_ns % NSEC_PER_SEC));
        }
    }


    lastCall = ts_now;

    return simple_read_from_buffer(ubuf, count, ppos, buf, len);
}

static const struct proc_ops timer_fops = {
    .proc_read = timer_read,
};

static int __init timer_init(void)
{
    timer_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &timer_fops);
    if (!timer_entry) {
        return -ENOMEM;
    }

    return 0;
}

static void __exit timer_exit(void)
{
    proc_remove(timer_entry);
}

module_init(timer_init);
module_exit(timer_exit);