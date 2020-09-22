#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/time.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ilyasov Idris ICS7-63B");
MODULE_DESCRIPTION("Tasklet");


static int irq_cnt = 0, irq = 1;
module_param(irq, int, S_IRUGO);
char tasklet_data[] = "tasklet func";
void tasklet_function(unsigned long data);
static struct timeval time;


DECLARE_TASKLET(my_tasklet, tasklet_function, (unsigned long)&tasklet_data);


void tasklet_function(unsigned long data)
{
    do_gettimeofday(&time);
	printk(KERN_INFO "+ TASKLET --- TIME: %.2lu:%.2lu:%.2lu\n", (time.tv_sec / 3600) % (24),
	                                                            (time.tv_sec / 60) % (60),
	                                                            time.tv_sec % 60);
							
	return;
}

static irqreturn_t my_interrupt(int irq, void *dev_id)
{
    if (irq == 1)
    {
	    irq_cnt++;
	    tasklet_schedule(&my_tasklet);
	    return IRQ_HANDLED;
	}
	else
	{
	    return IRQ_NONE;
	}
}

static int __init my_tasklet_init(void)
{
	if (request_irq(irq, my_interrupt, IRQF_SHARED, "my_interrupt", (void*)my_interrupt))
	{
		return -1;
	}
	
	printk("Successfully loading ISR handler on IRQ %d\n", irq);
	printk("Module is now loaded!\n");
	return 0;
}

static void __exit my_tasklet_exit(void)
{
	tasklet_kill(&my_tasklet);
	free_irq(irq, (void*)my_interrupt);
	printk("Successfully unloading, irq_cnt = %d\n", irq_cnt);
	printk("Module is now unloaded!\n");
	return;
}

module_init(my_tasklet_init);
module_exit(my_tasklet_exit);
