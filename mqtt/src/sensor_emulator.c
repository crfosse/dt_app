/*#define STACK_SIZE 512
#define PRIORITY_ALARM 5
#define PRIORITY_MSG 4

K_THREAD_STACK_DEFINE(stack_area, STACK_SIZE);

struct k_work_q application_work_q;

k_work_q_start(&application_work_q, stack_area, 
                K_THREAD_STACK_SIZEOF(stack_area) PRIORITY)*/

// Produce random data at random interval
/*
Needs:
- Init
- Randomizing function
- Read function
*/


#define ALARM_THRESHOLD 100

void init_sensor() {

    // Sensor emulator thread
    // Will we produce and fetch data every timestep? 

    //If the data is above a certain value, we trigger an alarm
    //Else we add the data to a list to compute the avarage

    //The main program will fetch this avarage at certain timesteps(like every 15 minutes or so),
    //The alarm can be triggered at any time (sudden wakeup)

    //Read current value
    //




}

static void sensor_alarm_trigger(int temp) {

    alarm_msg.time = 0;
    alarm_msg.value = temp;
    k_work_submit(alarm_msg);

}

void sensor_read(int * data) {

}

void sensor_read_avg(int * data) {

}