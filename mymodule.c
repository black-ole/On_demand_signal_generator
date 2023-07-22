#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <asm/siginfo.h>
#include <linux/timer.h>

#define procfs_name "sig_target"
#define PROCFS_MAX_SIZE 2048
#define TIMEOUT 1000
static char procfs_buffer[PROCFS_MAX_SIZE];
static char temp_buffer[PROCFS_MAX_SIZE];
static unsigned long procfs_buffer_size = 0;
static struct proc_dir_entry *Our_Proc_File;
struct timer_list tim;
static int end = 0;
static int curr = 0;
static int buffend = 0;
static struct store {
	pid_t pid;
	int sig;
};

static struct store pidsig[100] ;

static ssize_t procfs_read(struct file *flip, char *buffer, size_t length, loff_t *offset)
{
	static int finished = 0;
	
	if(finished){
		finished = 0;
		return 0;
	}

	finished = 1;
	
	
	if(copy_to_user(buffer, procfs_buffer, PROCFS_MAX_SIZE)){
		return -EFAULT;
	}

	return PROCFS_MAX_SIZE;

}

static ssize_t procfs_write(struct file *file , const char *buffer, size_t len, loff_t *off)
{	
	int j = 0;
	int i= 0;
	pid_t pid = 0;
	int signum = 0;
	
	if(len > PROCFS_MAX_SIZE){
		procfs_buffer_size = PROCFS_MAX_SIZE;
	} else {
		procfs_buffer_size = len;
	}
	if(copy_from_user(temp_buffer, buffer, procfs_buffer_size)){
		return -EFAULT;
	}
	
	

	while ( temp_buffer[j] != '\0' ){
		procfs_buffer[buffend] = temp_buffer[j];
		buffend  = buffend + 1;
		j = j + 1;
	}
	
	
	
	while ( temp_buffer[i] != ','){
		pid = 10*pid + (temp_buffer[i] - 48);
		i = i + 1;
	}
	i = i + 2;
	
	while (i != procfs_buffer_size - 1){
		signum = 10*signum + (temp_buffer[i] - 48);	
		i = i + 1;
	}
	
	pidsig[curr].pid = pid;
	pidsig[curr].sig = signum;		
	curr = curr + 1;
	return procfs_buffer_size;
}

static const struct proc_ops hello_proc_fops = {
	.proc_read = procfs_read,
	.proc_write = procfs_write
};
	
void timer_callback(struct timer_list *data)
{
	struct kernel_siginfo info;
	struct task_struct *t;

	while (end < curr){

	t = pid_task(find_pid_ns(pidsig[end].pid, &init_pid_ns), PIDTYPE_PID);
	
	memset(&info, 0, sizeof(struct kernel_siginfo));
	info.si_signo = pidsig[end].sig;
	if(t != NULL){
		send_sig_info(pidsig[end].sig, &info, t);		
	}
	end = end + 1;
	}
	
	mod_timer(&tim, jiffies + msecs_to_jiffies(TIMEOUT));
}
int init_module()
{
	Our_Proc_File = proc_create(procfs_name, 0644, NULL, &hello_proc_fops);
	
	if(Our_Proc_File == NULL){
		remove_proc_entry(procfs_name, NULL);
		return -ENOMEM;
	}
	
	timer_setup(&tim, timer_callback ,0);
	
	mod_timer(&tim, jiffies + msecs_to_jiffies(TIMEOUT));

	return 0;
}

void cleanup_module()
{
	del_timer(&tim);
	remove_proc_entry(procfs_name, NULL);
}

MODULE_LICENSE("GPL");
