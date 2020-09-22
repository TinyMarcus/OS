#include <stdio.h> 
#include <string.h> 
#include <dirent.h>
#include <unistd.h>

#define BUF_SIZE 0x100
#define SUCCESS 0 
#define FILE_ERROR -1 
#define FREAD_ERROR -2 
#define SPRINTF_ERROR -3 
#define READLINK_ERROR -4 
#define OPEN_DIR_ERROR -5 
#define CLOSE_DIR_ERROR -6

char *key[] = {"pid", "comm", "state", "ppid", "pgrp", "session", "tty_nr", "tpgid", "flags", "minflt", "cminflt", 
               "majflt", "cmajflt", "utime", "stime", "cutime", "cstime", "priority", "nice", "num_threads", "itrealvalue", 
               "starttime", "vsize", "rss", "rsslim", "startcode", "endcode", "startstack", "kstkesp", "kstkeip", "signal", 
               "blocked", "sigignore", "sigcatch", "wchan", "nswap", "cnswap", "exit_signal", "processor", "rt_priority", 
               "policy", "delayacct_blkio_ticks", "guest_time", "cguest_time", "start_data", "end_data", "start_brk", 
               "arg_start", "arg_end", "env_start", "env_end", "exit_code"};

int print_file(char *name) 
{ 
    char buf[BUF_SIZE]; 
    int len = 0;
    FILE *f = NULL;
    f = fopen(name, "r"); 
    
    if (!f)
        return FILE_ERROR; 
        
    printf("\n\n====================%s====================\n\n", name);

    while ((len = fread(buf, 1, BUF_SIZE, f)) > 0) { for (int i = 0; i < len; i++) 
    {
        if (buf[i] == 0) 
        { 
            buf[i] = 10;
        } 
    }
    
    buf[len - 1] = 0;
    printf("%s", buf); }
    
    if (fclose(f) != 0)
        return FILE_ERROR;
    
    printf("\n");
    return SUCCESS; 
}

int print_stat() 
{
    char buf[BUF_SIZE]; 
    FILE *f = NULL; 
    char *pch = NULL; 
    int i = 0;
    
    f = fopen("/proc/self/stat", "r"); 
    if (!f)
        return FILE_ERROR;
    if (fread(buf, 1, BUF_SIZE, f) <= 0)
        return FREAD_ERROR;
    
    printf("\n\n====================STAT====================\n\n");
    
    pch = strtok(buf, " "); 

    while (pch != NULL) 
    {
        printf("%d) %s: %s\n", i + 1, key[i], pch); 
        pch = strtok(NULL, " ");
        i++;
    }
    
    if (fclose(f) != 0)
        return FILE_ERROR; 
        
    return SUCCESS;
}

int print_fd() 
{
    struct dirent *dirp = NULL; 
    DIR *dp = NULL;
    char str[BUF_SIZE];
    char path[BUF_SIZE];
    
    dp = opendir("/proc/self/fd"); 
    
    if (!dp)
        return OPEN_DIR_ERROR;
    
    printf("\n\n====================FD====================\n\n");
    
    while ((dirp = readdir(dp)) != NULL) 
    {
        if ((strcmp(dirp->d_name, ".") != 0) && (strcmp(dirp->d_name, ".") != 0)) 
        {
            if (sprintf(path, "%s%s", "/proc/self/fd/", dirp->d_name) < 0)
                return SPRINTF_ERROR;
            
            readlink(path, str, BUF_SIZE);
            printf("%s -> %s\n", dirp->d_name, str); 
        }
    }

    if (closedir(dp) < 0)
        return CLOSE_DIR_ERROR; 
        
    return SUCCESS;
}

int main(int argc, char **argv) 
{
    int err = 0;
    
    if ((err = print_file("/proc/self/environ")))
        return err;
    
    if ((err = print_stat()))
        return err;
    if ((err = print_fd()))
        return err;
    if ((err = print_file("/proc/self/cmdline")))
        return err; 
    
    return err;
}