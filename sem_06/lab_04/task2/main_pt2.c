#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>

#define MAX_COOKIE_LENGTH PAGE_SIZE

MODULE_LICENSE("Dual BSD/GPL"); 
MODULE_AUTHOR("Ilyasov Idris");

static struct proc_dir_entry *proc_entry; 
char *cookie_pot;
int cookie_index;
int next_fortune;
char buf[256];

static ssize_t fortune_write(struct file *file, const char __user *ubuf,size_t count, loff_t *ppos) 
{ 
    if (count > MAX_COOKIE_LENGTH - cookie_index + 1) 
    {
        printk(KERN_DEBUG "Big count\n");
        return -ENOSPC; 
    }

    if (copy_from_user(cookie_pot + cookie_index, ubuf, count)) 
    { 
        printk(KERN_DEBUG "Copy_from_user error\n");
        return -EFAULT;
    }

    cookie_index += count; 
    cookie_pot[cookie_index - 1] = 0;
    printk(KERN_DEBUG "Good write\n");
    
    return count; 
}

static ssize_t fortune_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{ 
    int len = 0;
    
    if (*ppos > 0)
        return 0;
    
    if (next_fortune >= cookie_index) 
    { 
        next_fortune = 0;
    }

    if (cookie_index > 0) 
    {
        len = sprintf(buf, "%s\n", cookie_pot + next_fortune); 
        copy_to_user(ubuf, buf, len);
        next_fortune += len;
        ubuf += len;
        printk(KERN_DEBUG "Len: %d\n", len);
        *ppos += len; 
    }

    return len; 
}

static struct file_operations myops = 
{
    .owner = THIS_MODULE, 
    .read = fortune_read,
    .write = fortune_write 
};

static int simple_init(void) 
{
    cookie_pot = (char *)vmalloc(MAX_COOKIE_LENGTH); 
    
    if (!cookie_pot)
        return -ENOMEM;
    
    memset(cookie_pot, 0, MAX_COOKIE_LENGTH);
    proc_entry = proc_create("myfortune", 0666, NULL, &myops);
    
    if (proc_entry == NULL) 
    {
        vfree(cookie_pot);
        printk(KERN_DEBUG "fortune: Couldn't create proc entry\n"); 
        return -ENOMEM;
    }
    
    cookie_index = 0; 
    next_fortune = 0;
    printk(KERN_DEBUG "fortune: Init\n");

    return 0; 
}

static void simple_cleanup(void) 
{ 
    proc_remove(proc_entry); 
    vfree(cookie_pot);
    printk(KERN_DEBUG "fortune: Clean\n");
}

module_init(simple_init); 
module_exit(simple_cleanup);