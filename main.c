#include <zephyr.h>
#include <sys/printk.h>
#include "task_model.h"
#include <shell/shell.h>

/* stack size */
#define STACK_SIZE 1024

/*define stack array*/
K_THREAD_STACK_ARRAY_DEFINE(arr, NUM_THREADS, STACK_SIZE);

/*Thread IDs*/
k_tid_t t_arr[NUM_THREADS];

/*struct k_thread defined for threads*/
struct k_thread my_thread_data[NUM_THREADS];

/* thread declaration */
void entry_thread_func(void*, void*, void*);

struct k_mutex my_mutex[NUM_MUTEXES];	
struct k_timer timer_thread[NUM_THREADS]; 
struct k_sem com_sem;
bool timed_thread = false;
bool Done=false;

/*Function for thread defined*/
void entry_thread_func(void * a, void * b, void * c) {
	struct task_s * threads = (struct task_s * ) a;
	struct k_timer * timer_thread = (struct k_timer * ) b; 
	ARG_UNUSED(c);
	const char * t_name;
	/*structure for current thread*/
	struct k_thread * current_thread;
	/*getting the thread ID of current ID*/
	current_thread = k_current_get();
	t_name = k_thread_name_get(current_thread);
	k_timer_start(timer_thread, K_MSEC(threads->period), K_MSEC(threads->period)); 
	while (Done == false){
		volatile uint64_t n;       //variable for storing iterations
		n = threads->loop_iter[0];
		/*compute 1*/
		printk("entering compute 1");
		while(n>0 && (k_timer_status_get(timer_thread)==0)){ 
		n--;
		//printk("Executing thread: %s\n",t_name);
		}
		if(n!=0)
		{
			timed_thread = true;
			printk("%s Task missed Deadline\n",t_name);
			timed_thread = false;
			continue;
		}
		int l = 1;
		l = k_mutex_lock(&my_mutex[threads->mutex_m], K_FOREVER);
		if (Done == true && !l)
		{
			k_mutex_unlock(&my_mutex[threads->mutex_m]);
			k_thread_abort(current_thread);
			continue;
		}
		/*compute 2*/
		n = threads->loop_iter[1];
		while(n>0 && (k_timer_status_get(timer_thread)==0)){ 
		n--;
		//printk("Executing thread: %s\n",t_name);
		}
		if(n!=0)
		{
			timed_thread = true;
			printk("%s Task Missed Deadline\n",t_name);			
		}
		k_mutex_unlock(&my_mutex[threads->mutex_m]);
		if(timed_thread)
		{
			timed_thread = false;
			continue;
		}
		/*compute 2*/
		n = threads->loop_iter[2];
		while(n>0 && (k_timer_status_get(timer_thread)==0)){ 
		n--;
		//printk("Executing thread: %s\n",t_name);
		}
		if(n!=0)
		{
			timed_thread = true;
			printk("%s Task Missed Deadline\n",t_name);
			timed_thread = false;
			continue;
		}
		
		if(timed_thread)
		{
			timed_thread = false;
			continue;
		}
		k_msleep(k_timer_remaining_get(timer_thread));
	}
	k_timer_stop(timer_thread);
}

static int cmd_activate(const struct shell * shell, size_t argc, char ** argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	k_sem_give(&com_sem);
	 for (int i = 0; i < NUM_THREADS; i++) {
		k_thread_start(t_arr[i]);		
     }
  	return 0;
}
SHELL_CMD_ARG_REGISTER(activate, NULL, "Activates threads after creation", cmd_activate, 1, 0);

/* Start of main function */
void main(void) {
  //printk("Inside main!\n");
   for (int j = 0 ; j<NUM_MUTEXES; j++)
 	 {
  	  k_mutex_init(&my_mutex[j]);
 	 }
   
   /*semaphore initialisation*/
	k_sem_init(&com_sem,0, 1);
   for (int i = 0; i < NUM_THREADS; i++) {
		printk("Inside iteration %d", i);
		k_timer_init(&timer_thread[i], NULL, NULL); 
		t_arr[i] = k_thread_create( &my_thread_data[i], arr[i], STACK_SIZE, entry_thread_func, &threads[i], &timer_thread[i], NULL, threads[i].priority, 0, K_FOREVER); 
		/*set the thread name for subsequent threads*/
		k_thread_name_set(t_arr[i], threads[i].t_name);
		#if PIN_THREADS
		/*Thread not schedulable on cpu*/
		k_thread_cpu_mask_clear(t_arr[i]);
		/*Thread not running but enabling to run on specified CPU*/
		k_thread_cpu_mask_enable(t_arr[i], 0);
		#endif

		}
    
	k_sem_take(&com_sem, K_FOREVER); 
	
	/*wait for specified time*/
    k_msleep(TOTAL_TIME);
    Done = true;
    int j;
    for(j=0; j<NUM_THREADS; j++){
    	k_thread_join(&my_thread_data[j], K_FOREVER);
    }

  }

