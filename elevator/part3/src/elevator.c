#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/list.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("group-21");
MODULE_DESCRIPTION("Kernel module proc file for elevator");

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL
#define LOG_BUF_LEN 1024
#define NUM_FLOORS 6
#define MAX_WEIGHT 750
#define MAX_PASSENGERS 5

int start_elevator(void);                                                           // starts the elevator to pick up and drop off passengers
int issue_request(int start_floor, int destination_floor, int type);                // add passengers requests to specific floors
int stop_elevator(void); 
int get_passenger_weight(int passenger_type);


extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int,int,int);
extern int (*STUB_stop_elevator)(void);

enum state {OFFLINE, IDLE, LOADING, UP, DOWN};

struct passenger{
    int dest_floor, type, weight, is_valid;
    struct list_head list;
};

struct floor {
    int size;
    struct list_head list;
};

struct building {
    struct floor floors [6];
};

struct elevator {
    enum state state;
    int current_floor;
    int current_load;
    char ** status;
    struct task_struct * kthread;
};

static int passServiced = 0;
static int passWaiting = 0;
static int target = -1;
static struct elevator elevator_thread;
static struct building building;
static bool isOff = true;
static bool goingUp = true;
static struct proc_dir_entry* elevator_entry;
static struct passenger passengers[5];
static int numOfPassengers = 0;
static DEFINE_MUTEX(buffer_mutex);

int start_elevator(void) {
    elevator_thread.state = IDLE;
    isOff = false;
    return 0;
}

int issue_request(int start_floor, int destination_floor, int type) {
    mutex_lock(&buffer_mutex);
    building.floors[start_floor - 1].size++;

    struct passenger * new_passenger = kmalloc(sizeof(struct passenger), GFP_KERNEL);
    //if(!new_passenger){
       // printk(KERN_INFO "ERORR: Could not allocate memory for passenger");
      //  return -ENOMEM;
    //}
    new_passenger->type = type;
    new_passenger->dest_floor = destination_floor;
    new_passenger->weight = 100 + (50*new_passenger->type);
    new_passenger->is_valid = 1;

    list_add_tail(&new_passenger->list, &building.floors[start_floor-1].list);
    passWaiting++;
    mutex_unlock(&buffer_mutex);
    return 0;
}

int stop_elevator(void) {
    isOff = true;
    return 0;
}

/* Move to the upper floor */
int elevator_up(int floor) {
    if ((floor + 1) > (NUM_FLOORS - 1)){
        return floor;
    }
    return floor + 1;
}

/* Move to the lower floor */
int elevator_down(int floor) {
    if ((floor - 1) < 0){
        return floor;
    }
    return floor - 1;
}

void process_elevator_state(struct elevator * e_thread){
    switch (e_thread->state){
    case OFFLINE:
        break;
    case IDLE:
        mutex_lock(&buffer_mutex);
        if (isOff == true){
            e_thread->state = OFFLINE;
        }
        else{
            target = -1;
            for (int i = 0; i < NUM_FLOORS; ++i){
                if (building.floors[i].size > 0){
                    target = 1;
                    break;
                }
            }
            //If there isnt anyone left to service we stop
            if (target < 0 && numOfPassengers == 0){
                e_thread->state = IDLE;
            }
            else if (building.floors[e_thread->current_floor].size > 0){
                e_thread->state = LOADING;
            }
            else if (goingUp){
                e_thread->state = UP;
            }
            else{
                e_thread->state = DOWN;
            }
        }
        mutex_unlock(&buffer_mutex);
        break;
    case LOADING:
        ssleep(1);
        mutex_lock(&buffer_mutex);
        //Check if there is someone to unload
        for (int i = 0; i < MAX_PASSENGERS; ++i){
            if (passengers[i].dest_floor == (e_thread->current_floor + 1)){
                printk(KERN_INFO "%d - %d",passengers[i].dest_floor, passengers[i].is_valid);
                passServiced++;
                numOfPassengers--;
                e_thread->current_load -= passengers[i].weight;
                passengers[i].is_valid = 0;
                passengers[i].dest_floor = 0;
                passengers[i].type = 0;
                passengers[i].weight = 0;
            }
        }
        //Check if there is someone to load
        if (list_count_nodes(& building.floors[e_thread->current_floor].list) != 0){
            struct passenger *passenger;

            while (!list_empty(&building.floors[e_thread->current_floor].list)) {
                // Get the next entry and update temp
                passenger = list_first_entry_or_null(&building.floors[e_thread->current_floor].list, struct passenger, list);
                if (!passenger) {
                    // Handle the case where the list is empty
                    break;
                }
                if (numOfPassengers == MAX_PASSENGERS)
                    break;
                if ((e_thread->current_load + passenger->weight) > 750)
                    break;
                // Process the current entry
                for (int i = 0; i < MAX_PASSENGERS; ++i) {
                    if (passengers[i].is_valid == 0) {
                        passengers[i].type = passenger->type;
                        passengers[i].dest_floor = passenger->dest_floor;
                        passengers[i].weight = passenger->weight;
                        passengers[i].is_valid = passenger->is_valid;
                        e_thread->current_load += passenger->weight;
                        numOfPassengers++;
                        passWaiting--;
                        list_del(&passenger->list);
                        building.floors[e_thread->current_floor].size--;
                        break;  // Exit the loop after processing one passenger
                    }
                }
            }
        }
        target = -1;
        for (int i = 0; i < NUM_FLOORS; ++i){
            if (building.floors[i].size > 0){
                target = 1;
                break;
            }
        }
        if (target < 0 && numOfPassengers == 0){
            e_thread->state = IDLE;
        }
        else if (goingUp){
            e_thread->state = UP;
        }
        else{
            e_thread->state = DOWN;
        }
        mutex_unlock(&buffer_mutex);
        printk(KERN_INFO "Working%d",e_thread->state);
        break;
    case UP:
        ssleep(2);
        mutex_lock(&buffer_mutex);
        e_thread->current_floor = elevator_up(e_thread->current_floor);
        if (target < 0){
            e_thread->state = IDLE;
        }

        //Handles the logic for going up and down
        if (e_thread->current_floor == 5){
            goingUp = false;
        }

        //If there is no one there we can skip it (NEED TO ADD LOGIC TO CHECK IF ANYONE NEEDS TO BE DROPPED OFF HERE)
        if (building.floors[e_thread->current_floor].size == 0){
            bool getOff = false;
            for (int i = 0; i < MAX_PASSENGERS; ++i){
                if (passengers[i].dest_floor == (e_thread->current_floor + 1) && passengers[i].is_valid == 1){
                    getOff = true;
                    break;
                }
            }   
            if (getOff){
                e_thread->state = LOADING;
            }
            else if (goingUp)
                e_thread->state = UP;
            else
                e_thread->state = DOWN;
        }
        else{
            e_thread->state = LOADING;
        }
        mutex_unlock(&buffer_mutex);
        break;
    case DOWN:
        ssleep(2);
        mutex_lock(&buffer_mutex);
        e_thread->current_floor = elevator_down(e_thread->current_floor);

        //Idles if we dont have a target
        if (target < 0){
            e_thread->state = IDLE;
        }
        //Handles the logic for going up and down
        if (e_thread->current_floor == 0){
            goingUp = true;
        }

        //If there is no one there we can skip it
        if (building.floors[e_thread->current_floor].size == 0){
            bool getOff = false;
            for (int i = 0; i < MAX_PASSENGERS; ++i){
                if (passengers[i].dest_floor == (e_thread->current_floor + 1) && passengers[i].is_valid == 1){
                    getOff = true;
                    break;
                }
            }   
            if (getOff){
                e_thread->state = LOADING;
            }
            else if (goingUp)
                e_thread->state = UP;
            else
                e_thread->state = DOWN;
        }
        else{
            e_thread->state = LOADING;
        }
        mutex_unlock(&buffer_mutex);
        break;
    default:
        break;
    }
}

int elevator_active(void * _elevator){
    struct elevator * e_thread = (struct elevator *) _elevator;
    printk(KERN_INFO "Elevator is now running\n");
    while(!kthread_should_stop()){
        process_elevator_state(e_thread);
    }
    return 0;
}

int spawn_elevator(struct elevator * e_thread) {
    static int current_floor = 0;

    e_thread->current_floor = current_floor;
    e_thread->state = OFFLINE;
    e_thread->kthread = kthread_run(elevator_active, e_thread, "Thread Elevator\n");
    for(int i = 0; i < NUM_FLOORS; i++){
        for(int i = 0; i < NUM_FLOORS; i++){
            INIT_LIST_HEAD(&building.floors[i].list);
        }
    }
    return 0;
}

static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
    char buf[10000];
    int len = 0;
    const char *states[5] = { "OFFLINE", "IDLE", "LOADING", "UP", "DOWN" };

    mutex_lock(&buffer_mutex);
    len = sprintf(buf, "Elevator state: %s\n", states[elevator_thread.state]);
    len += sprintf(buf + len, "Current floor: %d\n", elevator_thread.current_floor + 1);
    len += sprintf(buf + len, "Current load: %d lbs\n", elevator_thread.current_load);
    len += sprintf(buf + len, "Elevator status:");

    if (numOfPassengers > 0){
        for (int i = 0; i < MAX_PASSENGERS; ++i) {
            if (passengers[i].is_valid == 1){
                len += sprintf(buf + len, " ");
                int type = passengers[i].type;
                int dest = passengers[i].dest_floor;
                switch (type)
                {
                    case 0:
                        len += sprintf(buf + len, "%c%d", 'F', dest);
                        break;
                    case 1:
                        len += sprintf(buf + len, "%c%d", 'O', dest);
                        break;
                    case 2:
                        len += sprintf(buf + len, "%c%d", 'J', dest);
                        break;
                    case 3:
                        len += sprintf(buf + len, "%c%d", 'S', dest);
                        break;
                    default:
                        break;
                }
            }
        }
    }
    len += sprintf(buf + len, "\n\n");

    for (int i = NUM_FLOORS - 1; i >= 0; --i) {
        int floor = i + 1;
        len += (i != elevator_thread.current_floor)
            ? sprintf(buf + len, "[ ] Floor %d:", floor)
            : sprintf(buf + len, "[*] Floor %d:", floor);

        // Display passenger counter without brackets
        len += sprintf(buf + len, " %d", building.floors[i].size);

        struct passenger *current_passenger;
        list_for_each_entry(current_passenger, &building.floors[i].list, list) {
            char passenger_type;
            int passenger_type_id = current_passenger->type;

            if (passenger_type_id == 0) {
                passenger_type = 'F';
            } else if (passenger_type_id == 1) {
                passenger_type = 'O';
            } else if (passenger_type_id == 2) {
                passenger_type = 'J';
            } else if (passenger_type_id == 3) {
                passenger_type = 'S';
            } 

            len += sprintf(buf + len, " %c%d", passenger_type, current_passenger->dest_floor);
        }

        len += sprintf(buf + len, "\n");
    }

    len += sprintf(buf + len, "Number of passengers: %d\n", numOfPassengers);
    len += sprintf(buf + len, "Number of passengers waiting: %d\n", passWaiting);
    len += sprintf(buf + len, "Number of passengers serviced: %d\n", passServiced);

    mutex_unlock(&buffer_mutex);

    return simple_read_from_buffer(ubuf, count, ppos, buf, len);
}

static const struct proc_ops elevator_fops = {
    .proc_read = elevator_read,
};

static int __init elevator_init(void)
{
    STUB_start_elevator = start_elevator;
	STUB_issue_request = issue_request;
	STUB_stop_elevator = stop_elevator;
    for (int i = 0; i < MAX_PASSENGERS; ++i){
        passengers[i].is_valid = 0;
    }
    spawn_elevator(&elevator_thread);
    if(IS_ERR(elevator_thread.kthread)){
        printk(KERN_WARNING "Error: error creating thread");
        remove_proc_entry(ENTRY_NAME, PARENT);
        return PTR_ERR(elevator_thread.kthread);
    }
    elevator_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &elevator_fops);
    if (!elevator_entry) {
        return -ENOMEM;
    }
    return 0;
}

static void __exit elevator_exit(void)
{
    struct passenger * pass, * next;

    for(int i = 0; i < NUM_FLOORS; i++){
        list_for_each_entry_safe(pass, next, &building.floors[i].list,list){
            list_del(&pass->list);
            kfree(pass);
        }

    }
    kthread_stop(elevator_thread.kthread);
    proc_remove(elevator_entry);

}

int get_passenger_weight(int passenger_type) {
    int weight;

    if (passenger_type == 0) {  // Freshmen
        weight = 100;
    } else if (passenger_type == 1) {  // Sophomores
        weight = 150;
    } else if (passenger_type == 2) {  // Juniors
        weight = 200;
    } else if (passenger_type == 3) {  // Seniors
        weight = 250;
    }

    return weight;
}

module_init(elevator_init);
module_exit(elevator_exit);
