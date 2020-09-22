#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ilyasov Idris ICS7-63B");
MODULE_DESCRIPTION("Workqueue");


static int irq_cnt = 0, irq = 1;
module_param(irq, int, S_IRUGO);
struct workqueue_struct *wq;
void hardwork_function(struct work_struct *work);
static struct timeval time;


DECLARE_WORK(hardwork, hardwork_function);


void hardwork_function(struct work_struct *work)
{
    do_gettimeofday(&time);
	printk(KERN_INFO "+ WORKQUEUE --- TIME: %.2lu:%.2lu:%.2lu\n", (time.tv_sec / 3600) % (24),
	                                                              (time.tv_sec / 60) % (60),
	                                                              time.tv_sec % 60);
							
	return;
}

static irqreturn_t my_interrupt(int irq, void *dev_id) 
{
    if (irq == 1)
    {
        irq_cnt++;
        queue_work(wq, &hardwork);
        return IRQ_HANDLED;
    }
    else
	{
	    return IRQ_NONE;
	}
}

static int __init my_workqueue_init(void)
{
	if (request_irq(irq, my_interrupt, IRQF_SHARED, "my_interrupt", (void*)my_interrupt))
	{
		return -1;
	}
	
	printk(KERN_INFO "Successfully loading ISR handler on IRQ %d\n", irq);
    wq = create_workqueue("workqueue");
    
    if (wq)
    {
        printk(KERN_INFO "Workqueue created!\n");
    }

	printk(KERN_INFO "Module is now loaded!\n");
	return 0;
}

static void __exit my_workqueue_exit(void)
{
    flush_workqueue(wq);
    destroy_workqueue(wq);
    free_irq(irq, (void*)my_interrupt);
	printk("Successfully unloading, irq_cnt = %d\n", irq_cnt);
	printk("Module is now unloaded!\n");
	return;
}

module_init(my_workqueue_init);
module_exit(my_workqueue_exit);
