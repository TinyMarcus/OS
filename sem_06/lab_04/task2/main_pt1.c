#include <build/include/linux/module.h> 
#include <build/include/linux/moduleparam.h> 
#include <linux/init.h>
#include <linux/kernel.h> 
#include <linux/proc_fs.h> 

#define BUFSIZE 100

MODULE_LICENSE("Dual BSD/GPL"); 
MODULE_AUTHOR("Ilyasov Idris");

static struct proc_dir_entry * ent;

static ssize_t mywrite(struct file *file, const char __user *ubuf,size_t count, loff_t *ppos) 
{ 
    printk(KERN_DEBUG "write handler\n");
    return -1;
}

static ssize_t myread(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{ 
    printk(KERN_DEBUG "read handler\n");
    return 0;
}

static struct file_operations myops = 
{
    .owner = THIS_MODULE, .read = myread,
    .write = mywrite
};

static int simple_init(void) 
{
    ent = proc_create("mydev", 0660, NULL, &myops); 
    ent = proc_symlink("link", NULL, "mydev");
    ent = proc_mkdir("mydir", NULL);
    return 0;
}

static void simple_cleanup(void) 
{ 
    proc_remove(ent);
}

module_init(simple_init); 
module_exit(simple_cleanup);