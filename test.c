#include <stdio.h>
#include "driver.h"
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <stdbool.h>
#include "stress.h"

#define mu_str_(text) #text
#define mu_str(text) mu_str_(text)
#define mu_assert(message, test) do { if (!(test)) return "FAILURE: See " __FILE__ " Line " mu_str(__LINE__) ": " message; } while (0)
#define mu_run_test(test) do { char *message = test(); tests_run++; \
	if (message) return message; } while (0)

typedef struct {
	driver_t *driver;
	void *job;
	enum driver_status out;
	sem_t *done;
} schedule_args;

typedef struct {
	driver_t *driver;
	void *job;
	enum driver_status out;
	sem_t *done;
} handle_args;

typedef struct {
	select_t *select_list;
	size_t list_size;
	sem_t *done;
	enum driver_status out;
	size_t index;
} select_args;

typedef struct {
	long double job;
	pthread_t pid;
} cpu_args;

int tests_run = 0;
int tests_passed = 0;
int string_equal(const char* str1, const char* str2) {
	if ((str1 == NULL) && (str2 == NULL)) {
		return 1;
	}
	if ((str1 == NULL) || (str2 == NULL)) {
		return 0;
	}
	return (strcmp(str1, str2) == 0);
}

schedule_args* create_object_for_schedule_api(driver_t* driver, char* message, sem_t* done) {
	schedule_args *new_args = malloc(sizeof(schedule_args));
	new_args->driver = driver;
	new_args->job = message;
	new_args->out = DRIVER_GEN_ERROR;
	new_args->done = done;

	return new_args;
}

handle_args* create_object_for_handle_api(driver_t* driver, sem_t* done) {
	handle_args *new_args = malloc(sizeof(handle_args));
	new_args->driver = driver;
	new_args->job = NULL;
	new_args->out = DRIVER_GEN_ERROR;
	new_args->done = done;

	return new_args;
}

select_args* create_object_for_select_api(select_t *list, size_t list_size, sem_t* done) {
	select_args *new_args = malloc(sizeof(select_args));
	new_args->select_list = list;
	new_args->list_size = list_size;
	new_args->out = DRIVER_GEN_ERROR;
	new_args->index = list_size;
	new_args->done = done;

	return new_args;
}

void destroy_handle_struct(handle_args *args) {
	free(args);
}

void destroy_schedule_struct(schedule_args *args) {
	free(args);
}

void destroy_select_struct(select_args *args) {
	free(args);
}

void print_test_details(const char* test_name, const char* message) {
	printf("Running test case: %s : %s ...\n", test_name, message);
}

void* helper_schedule(schedule_args *myargs) {
	myargs->out = driver_schedule(myargs->driver, myargs->job);
	if (myargs->done) {
		sem_post(myargs->done);
	}
	return NULL;
}

void* helper_handle(handle_args *myargs) {
	myargs->out = driver_handle(myargs->driver, &myargs->job);
	if (myargs->done) {
		sem_post(myargs->done);
	}
	return NULL;
}

void* helper_select(select_args *myargs) {
	myargs->out = driver_select(myargs->select_list, myargs->list_size, &myargs->index);
	if (myargs->done) {
		sem_post(myargs->done);
	}
	return NULL; 
}

void* helper_non_blocking_schedule(schedule_args *myargs) {
	myargs->out = driver_non_blocking_schedule(myargs->driver, myargs->job);
	if (myargs->done) {
		sem_post(myargs->done);
	}
	return NULL;
}

void* helper_non_blocking_handle(handle_args* myargs) {
	myargs->out = driver_non_blocking_handle(myargs->driver, &myargs->job);
	if (myargs->done) {
		sem_post(myargs->done);
	}
	return NULL;
}

static char* test_initialization() {
	print_test_details(__func__, "Testing the driver intialization");

	/* In this part of code we are creating a driver and checking if its intialization is correct */
	size_t capacity = 10000;
	driver_t* driver = driver_create(capacity);

	mu_assert("test_initialization: Could not create driver\n", driver != NULL);
	mu_assert("test_initialization: Did not create queue\n", driver->queue != NULL);
	mu_assert("test_initialization: Buffer size is not as expected\n", queue_current_size(driver->queue) ==  0);   
	mu_assert("test_initialization: Buffer capacity is not as expected\n", queue_capacity(driver->queue) ==  capacity);

	driver_close(driver);
	driver_destroy(driver);
	return NULL;
}

void* average_cpu_utilization(cpu_args* myargs) {

	struct rusage usage1;
	struct rusage usage2;

	getrusage(RUSAGE_SELF, &usage1);
	struct timeval start = usage1.ru_utime;
	struct timeval start_s = usage1.ru_stime;
	sleep(20);
	getrusage(RUSAGE_SELF, &usage2);
	struct timeval end = usage2.ru_utime;
	struct timeval end_s = usage2.ru_stime;

	long double result = (end.tv_sec - start.tv_sec)*1000000L + end.tv_usec - start.tv_usec + (end_s.tv_sec - start_s.tv_sec)*1000000L + end_s.tv_usec - start_s.tv_usec;

	myargs->job = result;
	return NULL;
}

static char* test_schedule_correctness() {
	print_test_details(__func__, "Testing the schedule correctness");

	/* In this part of code we create a driver with capacity 2 and spawn 3 threads to call schedule API.
	 * Expected response: 2 threads should return and one should be blocked
	 */
	size_t capacity = 2;
	driver_t* driver = driver_create(capacity);

	pthread_t pid[3];

	sem_t schedule_done;
	sem_init(&schedule_done, 0, 0);
	// Send first Message
	schedule_args *new_args = create_object_for_schedule_api(driver, "Message1", &schedule_done);
	pthread_create(&pid[0], NULL, (void *)helper_schedule, new_args);

	// wait before checking if its still blocking
	sem_wait(&schedule_done);

	mu_assert("test_schedule_correctness: Testing driver size failed" , queue_current_size(driver->queue) == 1);
	mu_assert("test_schedule_correctness: Testing driver value failed", string_equal(peek_queue(driver->queue, 0), "Message1"));
	mu_assert("test_schedule_correctness: Testing driver return failed", new_args->out == SUCCESS);

	// Send second message
	schedule_args *new_args_1 = create_object_for_schedule_api(driver, "Message2", &schedule_done);
	pthread_create(&pid[1], NULL, (void *)helper_schedule, new_args_1);

	// wait before checking if its still blocking
	sem_wait(&schedule_done);

	mu_assert("test_schedule_correctness: Testing queue size failed" , queue_current_size(driver->queue) == 2);
	mu_assert("test_schedule_correctness: Testing driver values failed", string_equal(peek_queue(driver->queue, 0), "Message1"));
	mu_assert("test_schedule_correctness: Testing driver values failed", string_equal(peek_queue(driver->queue, 1), "Message2"));
	mu_assert("test_schedule_correctness: Testing driver return failed", new_args_1->out == SUCCESS);

	// Send third thread
	schedule_args *new_args_2 = create_object_for_schedule_api(driver, "Message3", &schedule_done);
	pthread_create(&pid[2], NULL, (void *)helper_schedule, new_args_2);

	usleep(100);

	mu_assert("test_schedule_correctness: Testing queue size failed" , queue_current_size(driver->queue) == 2);
	mu_assert("test_schedule_correctness: Testing driver values failed", string_equal(peek_queue(driver->queue, 0), "Message1"));
	mu_assert("test_schedule_correctness: Testing driver values failed", string_equal(peek_queue(driver->queue, 1), "Message2"));
	mu_assert("test_schedule_correctness: Testing driver values failed", new_args_2->out == DRIVER_GEN_ERROR);

	// Receivinge from the driver to unblock
	void* out = NULL;
	driver_handle(driver, &out);

	for (size_t i = 0; i < 3; i++) {
		pthread_join(pid[i], NULL);
	}

	destroy_schedule_struct(new_args);
	destroy_schedule_struct(new_args_1);
	destroy_schedule_struct(new_args_2);

	// Empty driver again  
	driver_handle(driver, &out);
	driver_handle(driver, &out);

	// Checking for NULL as a acceptable value
	void* job = NULL;
	driver_schedule(driver, job);
	driver_schedule(driver, job);

	mu_assert("test_schedule_correctness: Testing queue size failed" , queue_current_size(driver->queue) == 2);
	mu_assert("test_schedule_correctness: Testing null value", peek_queue(driver->queue, 0) == NULL);
	mu_assert("test_schedule_correctness: Testing null value", peek_queue(driver->queue, 1) == NULL);

	// Clearing the memory  
	driver_close(driver);
	driver_destroy(driver);
	sem_destroy(&schedule_done);

	return NULL;
}

static char* test_handle_correctness() {
	print_test_details(__func__, "Testing the handle correctness");
	/* This test creates a driver of size 2 and calls handle thrice. The expectation 
	 * is that the last one is blocked.  
	 */
	size_t capacity = 2;
	driver_t* driver = driver_create(capacity);

	size_t RECEIVER_THREAD = 2; 
	handle_args *job_rec[RECEIVER_THREAD];
	// Filling driver with messages
	driver_schedule(driver, "Message4");
	driver_schedule(driver, "Message5");

	// Checking if the basic handles works
	void* out = NULL;
	driver_handle(driver, &out);

	mu_assert("test_handle_correctness: Testing queue size failed 1\n" , queue_current_size(driver->queue) == 1);
	mu_assert("test_handle_correctness: Testing driver values failed 1\n", string_equal(out, "Message4"));


	void* out1 = NULL;
	driver_handle(driver, &out1);

	mu_assert("test_handle_correctness: Testing queue size failed 2\n" , queue_current_size(driver->queue) == 0);
	mu_assert("test_handle_correctness: Testing driver values failed 2\n", string_equal(out1, "Message5"));

	pthread_t pid[RECEIVER_THREAD];
	sem_t done;
	sem_init(&done, 0, 0);

	// Start two threads with handles, then schedule to driver one by one.
	for (size_t i = 0; i < RECEIVER_THREAD; i++) {
		job_rec[i] = create_object_for_handle_api(driver, &done);
		pthread_create(&pid[i], NULL, (void *)helper_handle, job_rec[i]);

	}
	// Sleep is to wait before chekcing if handle calls are still blocked.
	usleep(100);

	mu_assert("test_handle_correctness: Testing driver size failed", queue_current_size(driver->queue) == 0);
	mu_assert("test_handle_correctness: Testing driver return failed", job_rec[0]->out == DRIVER_GEN_ERROR);
	mu_assert("test_handle_correctness: Testing driver return failed" , job_rec[1]->out == DRIVER_GEN_ERROR);

	driver_schedule(driver, "Message1");
	sem_wait(&done);

	mu_assert("test_handle_correctness: Testing driver return failed" , (job_rec[0]->out == SUCCESS || job_rec[1]->out == SUCCESS ));
	mu_assert("test_handle_correctness: Testing driver return failed" , !(job_rec[0]->out == SUCCESS && job_rec[1]->out == SUCCESS ));
	mu_assert("test_handle_correctness: Testing driver return failed" , string_equal(job_rec[0]->job, "Message1") || string_equal(job_rec[1]->job, "Message1")); 

	driver_schedule(driver, "Message2");
	sem_wait(&done);

	mu_assert("test_handle_correctness: Testing driver return failed" , (job_rec[0]->out == SUCCESS && job_rec[1]->out == SUCCESS ));
	mu_assert("test_handle_correctness: Testing driver return failed" , string_equal(job_rec[0]->job, "Message2") || string_equal(job_rec[1]->job, "Message2")); 

	for (size_t i = 0; i < RECEIVER_THREAD; i++) {
		pthread_join(pid[i], NULL);
	}

	// Free the allocated memory
	for (size_t i = 0; i < RECEIVER_THREAD; i++) {
		destroy_handle_struct(job_rec[i]);
	}

	// Checking for the NULL values in handle
	void* temp = (void*)0xdeadbeef;
	driver_schedule(driver, NULL);
	driver_handle(driver, &temp);

	mu_assert("test_handle_correctness: Testing NULL value from driver" , temp == NULL);

	driver_close(driver);
	driver_destroy(driver);
	sem_destroy(&done);

	return NULL;

}

static char* test_overall_schedule_handle() {
	print_test_details(__func__, "Testing schedule and handle overall");

	/* This test spwans 10 schedule and handle requests. The expectation is that none of the
	 *  calls should get blocked.
	 */
	size_t capacity = 2;
	driver_t* driver = driver_create(capacity);

	size_t RECEIVE_THREAD = 10;
	size_t SCHDLE_THREAD = 10;

	pthread_t rec_pid[RECEIVE_THREAD];
	pthread_t schedule_pid[SCHDLE_THREAD];

	handle_args *job_rec[RECEIVE_THREAD];
	schedule_args *job_schedule[SCHDLE_THREAD];
	// Spawning multiple schedule and handles
	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		job_rec[i] = create_object_for_handle_api(driver, NULL);
		pthread_create(&rec_pid[i], NULL, (void *)helper_handle, job_rec[i]);
	}

	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		job_schedule[i] = create_object_for_schedule_api(driver, "Message1", NULL);
		pthread_create(&schedule_pid[i], NULL, (void *)helper_schedule, job_schedule[i]);  
	}

	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		pthread_join(rec_pid[i], NULL);
	}

	for (size_t i = 0; i < SCHDLE_THREAD; i++){
		pthread_join(schedule_pid[i], NULL);
	}

	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		mu_assert("test_overall_schedule_handle: Testing driver schedule return failed" , job_schedule[i]->out == SUCCESS);
	}

	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		mu_assert("test_overall_schedule_handle: Testing driver handle return value failed" , job_rec[i]->out == SUCCESS);
		mu_assert("test_overall_schedule_handle: Testing driver handle return job failed" , string_equal(job_rec[i]->job, "Message1"));
	}

	// Free the allocated memory
	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		destroy_handle_struct(job_rec[i]);
	}

	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		destroy_schedule_struct(job_schedule[i]);
	}

	driver_close(driver);
	driver_destroy(driver);

	return NULL;

}

static char* test_non_blocking_schedule() {
	print_test_details(__func__, "Testing non_blocking schedule"); 

	/* This test ensures that when you try to schedule more number of messages 
	 * than what the driver can handle, non-blocking schedule should return with proper status. 
	 */
	size_t capacity = 2;
	driver_t* driver = driver_create(capacity);

	size_t SCHDLE_THREAD = 10;
	pthread_t schedule_pid[SCHDLE_THREAD];
	schedule_args *job_schedule[SCHDLE_THREAD];

	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		job_schedule[i] = create_object_for_schedule_api(driver, "Message1", NULL);
		pthread_create(&schedule_pid[i], NULL, (void *)helper_non_blocking_schedule, job_schedule[i]); 
	}

	for (size_t i = 0; i < SCHDLE_THREAD; i++){
		pthread_join(schedule_pid[i], NULL);
	}

	size_t schedule_count = 0;
	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		if (job_schedule[i]->out == DRIVER_REQUEST_FULL) {
			schedule_count ++;
		}
	}
	mu_assert("test_non_blocking_schedule: Testing driver schedule return value failed" , (schedule_count == SCHDLE_THREAD - capacity));

	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		destroy_schedule_struct(job_schedule[i]);
	}

	driver_close(driver);
	driver_destroy(driver);

	return NULL;

}

static char* test_non_blocking_handle() {
	print_test_details(__func__, "Testing non blocking handle");

	/* This test ensures that when you try to handle more number of messages 
	 * than what the driver contains, non-blocking handle should return with proper status. 
	 */
	size_t capacity = 2;
	driver_t* driver = driver_create(capacity);
	for (size_t i = 0; i < capacity; i++) {
		driver_schedule(driver, "Message");
	}

	size_t RECEIVE_THREAD = 10;
	size_t SCHDLE_THREAD = 3;

	pthread_t schedule_pid[SCHDLE_THREAD];
	pthread_t rec_pid[RECEIVE_THREAD];

	handle_args *job_rec[RECEIVE_THREAD]; 
	schedule_args *job_schedule[SCHDLE_THREAD];

	job_schedule[0] = create_object_for_schedule_api(driver, "Message1", NULL);
	pthread_create(&schedule_pid[0], NULL, (void *)helper_schedule, job_schedule[0]);  

	job_schedule[1] = create_object_for_schedule_api(driver, "Message2", NULL);
	pthread_create(&schedule_pid[1], NULL, (void *)helper_schedule, job_schedule[1]);  

	job_schedule[2] = create_object_for_schedule_api(driver, "Message3", NULL);
	pthread_create(&schedule_pid[2], NULL, (void *)helper_schedule, job_schedule[2]);  

	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		job_rec[i] = create_object_for_handle_api(driver, NULL);
		pthread_create(&rec_pid[i], NULL, (void *)helper_non_blocking_handle, job_rec[i]);
		// Allow time for schedules to take effect
		usleep(100000);
	}

	size_t handle_count = 0;
	int message_handled = 0;
	int message1_handled = 0;
	int message2_handled = 0;
	int message3_handled = 0;

	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		pthread_join(schedule_pid[i], NULL);
	}   

	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		pthread_join(rec_pid[i], NULL);
	}

	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		if (job_rec[i]->out == DRIVER_REQUEST_EMPTY) {
			handle_count ++;
		} else {
			if (string_equal(job_rec[i]->job, "Message")) {
				message_handled++;
			} else if (string_equal(job_rec[i]->job, "Message1")) {
				message1_handled++;
			} else if (string_equal(job_rec[i]->job, "Message2")) {
				message2_handled++;
			} else if (string_equal(job_rec[i]->job, "Message3")) {
				message3_handled++;
			} else {
				mu_assert("test_non_blocking_handle: Received invalid message", 0);
			}
		}
	}

	mu_assert("test_non_blocking_handle: Failed to handle 2 Message messages", message_handled == capacity);
	mu_assert("test_non_blocking_handle: Failed to handle Message1", message1_handled == 1);
	mu_assert("test_non_blocking_handle: Failed to handle Message2", message2_handled == 1);
	mu_assert("test_non_blocking_handle: Failed to handle Message3", message3_handled == 1);
	mu_assert("test_non_blocking_handle: Testing driver handle return value failed", (RECEIVE_THREAD - SCHDLE_THREAD - capacity) == handle_count);


	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		destroy_schedule_struct(job_schedule[i]);  
	}

	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		destroy_handle_struct(job_rec[i]);
	}

	driver_close(driver);
	driver_destroy(driver);

	return NULL;
}

static char* test_driver_close_with_schedule() {
	print_test_details(__func__, "Testing driver close API");

	/*This test ensures that all the blocking calls should return with proper status 
	 * when the driver gets closed 
	 */

	// Testing for the schedule API 
	size_t capacity = 2;
	driver_t* driver = driver_create(capacity);

	size_t SCHDLE_THREAD = 10;

	pthread_t schedule_pid[SCHDLE_THREAD];
	schedule_args *job_schedule[SCHDLE_THREAD];

	sem_t schedule_done;
	sem_init(&schedule_done, 0, 0);

	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		job_schedule[i] = create_object_for_schedule_api(driver, "Message1", &schedule_done);
		pthread_create(&schedule_pid[i], NULL, (void *)helper_schedule, job_schedule[i]);  
	}

	for (size_t i = 0; i < capacity; i++) {
		sem_wait(&schedule_done);
	}

	driver_close(driver);

	// XXX: All the threads should return in finite amount of time else it will be in infinite loop. Hence incorrect implementation
	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		pthread_join(schedule_pid[i], NULL);    
	}

	size_t count = 0;
	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		if (job_schedule[i]->out == DRIVER_CLOSED_ERROR) {
			count ++;
		}
	}

	mu_assert("test_driver_close: Testing driver close failed" , count == SCHDLE_THREAD - capacity);

	// Making a normal schedule call to check if its closed
	enum driver_status out = driver_schedule(driver, "Message");
	mu_assert("test_driver_close: Testing driver close failed" , out == DRIVER_CLOSED_ERROR);


	out = driver_non_blocking_schedule(driver, "Message");
	mu_assert("test_driver_close: Testing driver close failed" , out == DRIVER_CLOSED_ERROR);


	// Free all the allocated memory    
	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		destroy_schedule_struct(job_schedule[i]);
	}

	mu_assert("test_driver_close: Testing driver close failed" , driver_close(driver) == DRIVER_GEN_ERROR);
	driver_destroy(driver);
	sem_destroy(&schedule_done);

	return NULL;

}

static char* test_driver_close_with_handle() {
	print_test_details(__func__, "Testing driver close API");

	/*This test ensures that all the blocking calls should return with proper status 
	 * when the driver gets closed 
	 */

	// Testing for the close API 
	size_t capacity = 2;
	driver_t* driver = driver_create(capacity);
	for (size_t i = 0; i < capacity; i++) {
		driver_schedule(driver, "Message");
	}

	size_t RECEIVE_THREAD = 10;

	handle_args *job_rec[RECEIVE_THREAD];
	pthread_t rec_pid[RECEIVE_THREAD];

	sem_t done;
	sem_init(&done, 0, 0);  

	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		job_rec[i] = create_object_for_handle_api(driver, &done);
		pthread_create(&rec_pid[i], NULL, (void *)helper_handle, job_rec[i]); 
	}

	// Wait for handle threads to drain the queue
	for (size_t i = 0; i < capacity; i++) {
		sem_wait(&done);
	}

	// Close driver to stop the rest of the handle threads
	driver_close(driver);

	// XXX: All calls should immediately after close else close implementation is incorrect.
	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		pthread_join(rec_pid[i], NULL); 
	}

	size_t count = 0;
	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		if (job_rec[i]->out == DRIVER_CLOSED_ERROR) {
			count ++;
		}
	}

	mu_assert("test_driver_close_with_handle: Testing driver close failed" , count == RECEIVE_THREAD - capacity);

	// Making a normal schedule call to check if its closed
	void *job = NULL;
	enum driver_status out = driver_handle(driver, &job);
	mu_assert("test_driver_close_with_handle: Testing driver close failed" , out == DRIVER_CLOSED_ERROR);

	out = driver_non_blocking_handle(driver, &job);
	mu_assert("test_driver_close_with_handle: Testing driver close failed" , out == DRIVER_CLOSED_ERROR);

	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		destroy_handle_struct(job_rec[i]);
	}

	driver_destroy(driver);
	sem_destroy(&done);

	return NULL;
}

static char* test_multiple_drivers() {
	print_test_details(__func__, "Testing creating multiple drivers");

	/* This test ensures that multiple drivers can work simultaneously
	*/ 

	size_t driver1_capacity = 1;
	size_t driver2_capacity = 2;

	driver_t* driver1 = driver_create(driver1_capacity);
	driver_t* driver2 = driver_create(driver2_capacity);

	size_t SCHDLE_THREAD = 4;

	pthread_t schedule_pid[SCHDLE_THREAD];
	schedule_args *job_schedule[SCHDLE_THREAD];

	job_schedule[0] = create_object_for_schedule_api(driver1, "CHAN1_Message1", NULL);
	pthread_create(&schedule_pid[0], NULL, (void *)helper_schedule, job_schedule[0]);      
	job_schedule[1] = create_object_for_schedule_api(driver2, "CHAN2_Message1", NULL);
	pthread_create(&schedule_pid[1], NULL, (void *)helper_schedule, job_schedule[1]); 

	// To ensure first two messages gets delivered first
	pthread_join(schedule_pid[0], NULL);
	pthread_join(schedule_pid[1], NULL);

	job_schedule[2] = create_object_for_schedule_api(driver1, "CHAN1_Message2", NULL);
	pthread_create(&schedule_pid[2], NULL, (void *)helper_schedule, job_schedule[2]); 
	job_schedule[3] = create_object_for_schedule_api(driver2, "CHAN2_Message2", NULL);
	pthread_create(&schedule_pid[3], NULL, (void *)helper_schedule, job_schedule[3]); 

	void *job = NULL;
	enum driver_status out = driver_handle(driver1, &job);
	mu_assert("test_multiple_drivers: Testing multiple drivers1" , string_equal(job, "CHAN1_Message1"));
	mu_assert("test_multiple_drivers: Testing multiple drivers2", out == SUCCESS);

	void *job1 = NULL;
	out = driver_handle(driver2, &job1);
	mu_assert("test_multiple_drivers: Testing multiple drivers3" , string_equal(job1, "CHAN2_Message1"));
	mu_assert("test_multiple_drivers: Testing multiple drivers4", out == SUCCESS);

	void *job2 = NULL;
	out = driver_handle(driver2, &job2);
	mu_assert("test_multiple_drivers: Testing multiple drivers5" , string_equal(job2, "CHAN2_Message2"));
	mu_assert("test_multiple_drivers: Testing multiple drivers6", out == SUCCESS);

	void *job3 = NULL;
	out = driver_handle(driver1, &job3);
	mu_assert("test_multiple_drivers: Testing multiple drivers7" , string_equal(job3, "CHAN1_Message2"));
	mu_assert("test_multiple_drivers: Testing multiple drivers8", out == SUCCESS);

	for (size_t i = 2; i < SCHDLE_THREAD; i++) {
		pthread_join(schedule_pid[i], NULL);
	}

	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		destroy_schedule_struct(job_schedule[i]);
	}

	driver_close(driver1);
	driver_close(driver2);

	driver_destroy(driver1);
	driver_destroy(driver2);

	return NULL;
}

static char* test_response_time() {
	print_test_details(__func__, "Testing response time");

	/* This test ensures that all API calls return back in reasonable time frame
	*/

	size_t capacity = 2;
	driver_t* driver = driver_create(capacity);

	clock_t t; 
	void *job = NULL;

	pthread_t pid;
	schedule_args *job_schedule;
	handle_args *job_rec;

	sem_t done;
	sem_init(&done, 0, 0);

	job_rec = create_object_for_handle_api(driver, &done);
	pthread_create(&pid, NULL, (void *)helper_handle, job_rec);

	usleep(100);

	t = clock();
	driver_schedule(driver, "Message");
	sem_wait(&done);
	t = clock() - t;

	double time_taken = ((double)t)/CLOCKS_PER_SEC;
	mu_assert("test_response_time: Response time for schedule/handle is higher than 0.01", time_taken < 0.01);

	pthread_join(pid, NULL);
	destroy_handle_struct(job_rec);


	for (size_t i = 0; i < capacity; i++) {
		driver_schedule(driver, "Message");
	}

	job_schedule = create_object_for_schedule_api(driver, "Message", &done);
	pthread_create(&pid, NULL, (void *)helper_schedule, job_schedule);      

	usleep(100);

	t = clock();
	driver_handle(driver, &job);
	sem_wait(&done);
	t = clock() - t;

	time_taken = ((double)t)/CLOCKS_PER_SEC;
	mu_assert("test_response_time: Response time for schedule/handle is higher than 0.01", time_taken < 0.01);

	pthread_join(pid, NULL);
	destroy_schedule_struct(job_schedule);

	// Free memory
	driver_close(driver);
	driver_destroy(driver);
	return NULL;
}

static char* test_select() {
	print_test_details(__func__, "Testing select API");

	/* This test is for select API to work with multiple schedule and handle requests
	*/

	/* This part of code we spawn multiple blocking schedule calls with select and schedule one handle */
	size_t DRIVERS = 3;

	pthread_t pid, pid_1;
	driver_t* driver[DRIVERS];
	select_t list[DRIVERS];

	for (size_t i = 0; i < DRIVERS; i++) {
		driver[i] = driver_create(1);
		list[i].op = HANDLE;
		list[i].driver = driver[i];
	}

	sem_t done;
	sem_init(&done, 0, 0);
	// Testing with empty drivers and handle API
	select_args *args = create_object_for_select_api(list, DRIVERS, &done);
	pthread_create(&pid, NULL, (void *)helper_select, args);

	// To wait sometime before we check select is blocking now  
	usleep(100);
	mu_assert("test_select: It isn't blocked as expected", args->out == DRIVER_GEN_ERROR);

	clock_t t = clock();
	driver_schedule(driver[2], "Message1");
	sem_wait(&done);
	t = clock() - t;

	double time_taken = ((double)t)/CLOCKS_PER_SEC;
	mu_assert("test_select: Response time for select is higher than 0.01", time_taken < 0.01);

	// XXX: Code will go in infinite loop here if the select didn't return . 
	pthread_join(pid, NULL);
	mu_assert("test_select: Returned value doesn't match", args->index == 2);

	/* This part of code is to test select with multiple handles */
	for (size_t i = 0; i < DRIVERS; i++) {
		driver_schedule(driver[i], "Message");   
		list[i].op = SCHDLE;
		list[i].job = "Message4";
	}

	select_args *args_1 = create_object_for_select_api(list, DRIVERS, &done);
	pthread_create(&pid_1, NULL, (void *)helper_select, args_1);
	// Before chekcing for block sleep
	usleep(100);
	mu_assert("test_select: It isn't blocked as expected", args_1->out == DRIVER_GEN_ERROR);

	void* job;
	t = clock();
	driver_handle(driver[0], &job);
	sem_wait(&done);
	t = clock() - t;

	time_taken = ((double)t)/CLOCKS_PER_SEC;
	mu_assert("test_select: Response time for select is higher than 0.01", time_taken < 0.01);

	// XXX: Code will go in infinite loop here if the select didn't return . 
	pthread_join(pid_1, NULL);
	mu_assert("test_select: Returned value doesn't match", args_1->index == 0);

	pthread_create(&pid_1, NULL, (void *)helper_select, args_1);
	usleep(10000);
	driver_close(driver[0]);

	pthread_join(pid_1, NULL); 
	mu_assert("test_select: Driver is closed, it should propogate the same error", args_1->out == DRIVER_CLOSED_ERROR);

	size_t index;
	mu_assert("test_select: Select on closed driver should return DRIVER_CLOSED_ERROR", driver_select(list, DRIVERS, &index) == DRIVER_CLOSED_ERROR);

	destroy_select_struct(args);
	// Since destroy select struct deleted the list already
	free(args_1);

	for (size_t i = 0; i < DRIVERS; i++) {
		driver_close(driver[i]);
		driver_destroy(driver[i]);
	}
	sem_destroy(&done);

	return NULL;
}

char* test_cpu_utilization_for_schedule_handle() {
	print_test_details(__func__, "Testing CPU utilization for schedule, handle, API (takes around 70 seconds)");

	/* This test checks the CPU utilization of all the schedule, handle , select APIs.     
	*/
	size_t capacity = 2;
	driver_t *driver = driver_create(capacity);
	// For SCHDLE API
	pthread_t cpu_pid, pid;

	// Fill queue
	for (size_t i = 0; i < capacity; i++) {
		driver_schedule(driver, "Message");
	}

	schedule_args* job_schedule = create_object_for_schedule_api(driver, "Message", NULL);   
	pthread_create(&pid, NULL, (void *)helper_schedule, job_schedule);

	cpu_args* args = malloc(sizeof(cpu_args));
	pthread_create(&cpu_pid, NULL, (void *)average_cpu_utilization, args);  

	sleep(22);

	void* job;
	driver_handle(driver, &job);

	pthread_join(pid, NULL);
	pthread_join(cpu_pid, NULL);

	mu_assert("test_cpu_utilization: Invalid message", string_equal(job, "Message"));
	mu_assert("test_cpu_utilization: CPU Utilization is higher than required", args->job < 100000);

	driver_close(driver);
	driver_destroy(driver);
	free(args);
	destroy_schedule_struct(job_schedule);

	// For RECEIVE API
	driver_t *driver_1 = driver_create(2);
	pthread_t cpu_pid_1, pid_1;

	handle_args* job_handle = create_object_for_handle_api(driver_1, NULL);
	pthread_create(&pid_1, NULL, (void *)helper_handle, job_handle);

	cpu_args* args_1 = malloc(sizeof(cpu_args));
	pthread_create(&cpu_pid_1, NULL, (void *)average_cpu_utilization, args_1);

	sleep(22);

	driver_schedule(driver_1, "Message");

	pthread_join(pid_1, NULL);
	pthread_join(cpu_pid_1, NULL);

	mu_assert("test_cpu_utilization: Invalid message", string_equal(job_handle->job, "Message"));
	mu_assert("test_cpu_utilization: CPU Utilization is higher than required", args_1->job < 100000);

	driver_close(driver_1);
	driver_destroy(driver_1);
	free(args_1);
	destroy_handle_struct(job_handle);
	return NULL;
}
char* test_cpu_utilization_for_select() {
	print_test_details(__func__, "Testing CPU utilization for select API (takes around 70 seconds)");

	// Testing with empty drivers and handle API
	size_t DRIVERS = 3;

	pthread_t pid_3, cpu_pid_2;
	driver_t * drivers[DRIVERS];
	select_t list[DRIVERS];

	// For SELECT API
	for (size_t i = 0; i < DRIVERS; i++) {
		drivers[i] = driver_create(1);
		list[i].op = HANDLE;
		list[i].driver = drivers[i];
	}

	select_args *job_select = create_object_for_select_api(list, DRIVERS, NULL);
	pthread_create(&pid_3, NULL, (void *)helper_select, job_select);

	cpu_args* args_3 = malloc(sizeof(cpu_args));
	pthread_create(&cpu_pid_2, NULL, (void *)average_cpu_utilization, args_3);

	sleep(22);

	driver_schedule(drivers[0], "Message");

	pthread_join(pid_3, NULL);
	pthread_join(cpu_pid_2, NULL);

	mu_assert("test_cpu_utilization: Invalid select index picked", job_select->index == 0);
	mu_assert("test_cpu_utilization: Invalid message", string_equal(list[0].job, "Message"));
	mu_assert("test_cpu_utilization: CPU Utilization is higher than required", args_3->job < 100000);

	for (size_t i = 0; i < DRIVERS; i++) {
		driver_close(drivers[i]);
		driver_destroy(drivers[i]);
	}
	free(args_3);
	destroy_select_struct(job_select);

	return NULL;
}

static char* test_free() {
	print_test_details(__func__, "Testing driver destroy");
	/* This test is to check if free memory fails if the driver is not closed */ 
	driver_t *driver = driver_create(2);

	mu_assert("test_free: Doesn't report error if the driver is closed", driver_destroy(driver) == DRIVER_DESTROY_ERROR);

	driver_close(driver);
	driver_destroy(driver); 
	return NULL;
}

static char* test_zero_queue() {
	/* This test checks for zero queue */
	print_test_details(__func__, "Testing zero queue");

	size_t capacity = 0;

	driver_t* driver = driver_create(capacity);

	/* This test spwans 10 schedule and handle requests. The expectation is that none of the
	 *  calls should get blocked.
	 */

	size_t RECEIVE_THREAD = 10;
	size_t SCHDLE_THREAD = 10;

	pthread_t rec_pid[RECEIVE_THREAD];
	pthread_t schedule_pid[SCHDLE_THREAD];

	handle_args *job_rec[RECEIVE_THREAD];
	schedule_args *job_schedule[SCHDLE_THREAD];
	// Spawning multiple schedule and handles
	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		job_rec[i] = create_object_for_handle_api(driver, NULL);
		pthread_create(&rec_pid[i], NULL, (void *)helper_handle, job_rec[i]);
	}

	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		job_schedule[i] = create_object_for_schedule_api(driver, "Message1", NULL);
		pthread_create(&schedule_pid[i], NULL, (void *)helper_schedule, job_schedule[i]);  
	}

	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		pthread_join(rec_pid[i], NULL);
	}

	for (size_t i = 0; i < SCHDLE_THREAD; i++){
		pthread_join(schedule_pid[i], NULL);
	}

	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		mu_assert("test_zero_queue: Testing driver schedule return failed" , job_schedule[i]->out == SUCCESS);
	}

	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		mu_assert("test_zero_queue: Testing driver handle return value failed" , job_rec[i]->out == SUCCESS);
		mu_assert("test_zero_queue: Testing driver handle return job failed" , string_equal(job_rec[i]->job, "Message1"));
	}

	// Free the allocated memory
	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		destroy_handle_struct(job_rec[i]);
	}

	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		destroy_schedule_struct(job_schedule[i]);
	}

	driver_close(driver);
	driver_destroy(driver);

	return NULL;
}


static char* test_zero_queue_non_blocking() {
	/* This test checks for zero queue */
	print_test_details(__func__, "Testing zero queue for non_blocking");

	/* Sanity check for schedule and recieve. 
	 * Send one blocking schedule followed by un-blocking
	 * Send one blocking handle followed by un-blocking
	 * Send another non-blocking. It should return immediately . 
	 */
	size_t capacity = 0;
	driver_t* driver = driver_create(capacity);

	pthread_t s_pid, r_pid;

	schedule_args* schedule_ = create_object_for_schedule_api(driver, "Message", NULL);
	handle_args* rec_ = create_object_for_handle_api(driver, NULL);

	pthread_create(&s_pid, NULL, (void *)helper_schedule, schedule_);

	usleep(500);

	mu_assert("test_zero_queue_non_blocking: Testing driver non blocking queue", driver_non_blocking_schedule(driver, "Message_") == DRIVER_REQUEST_FULL);

	void* job = NULL;
	mu_assert("test_zero_queue_non_blocking: Testing driver non blocking queue", driver_non_blocking_handle(driver, &job) == SUCCESS);
	mu_assert("test_zero_queue_non_blocking: Testing driver non blocking queue", string_equal(job, "Message"));

	pthread_join(s_pid, NULL);

	pthread_create(&r_pid, NULL, (void *)helper_handle, rec_);
	void* out;

	usleep(500);

	mu_assert("test_zero_queue_non_blocking: Testing driver non blocking queue", driver_non_blocking_handle(driver, &out) == DRIVER_REQUEST_EMPTY);
	mu_assert("test_zero_queue_non_blocking: Testing driver non blocking queue", driver_non_blocking_schedule(driver, "Message_1") == SUCCESS);

	pthread_join(r_pid, NULL);
	mu_assert("test_zero_queue_non_blocking: Testing driver non blocking queue", rec_->out == SUCCESS);
	mu_assert("test_zero_queue_non_blocking: Testing driver non blocking queue", string_equal(rec_->job, "Message_1"));

	destroy_handle_struct(rec_);
	destroy_schedule_struct(schedule_);

	/* This test spwans 10 schedule and 2 handle requests before it. The expectation is that none of the
	 *  calls should get blocked.
	 */

	size_t RECEIVE_THREAD = 2;
	pthread_t rec_pid[RECEIVE_THREAD];
	handle_args *job_rec[RECEIVE_THREAD];

	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		job_rec[i] = create_object_for_handle_api(driver, NULL);
		pthread_create(&rec_pid[i], NULL, (void *)helper_handle, job_rec[i]);
	}

	usleep(500);

	size_t SCHDLE_THREAD = 10;
	pthread_t schedule_pid[SCHDLE_THREAD];
	schedule_args *job_schedule[SCHDLE_THREAD];
	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		job_schedule[i] = create_object_for_schedule_api(driver, "Message1", NULL);
		pthread_create(&schedule_pid[i], NULL, (void *)helper_non_blocking_schedule, job_schedule[i]);  
	}

	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		pthread_join(rec_pid[i], NULL);
	}

	for (size_t i = 0; i < SCHDLE_THREAD; i++){
		pthread_join(schedule_pid[i], NULL);
	}

	int count = 0;
	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		if (job_schedule[i]->out == DRIVER_REQUEST_FULL) {
			count ++;
		}
	}

	for (size_t i = 0; i < RECEIVE_THREAD; i++) {
		destroy_handle_struct(job_rec[i]);
	}

	for (size_t i = 0; i < SCHDLE_THREAD; i++) {
		destroy_schedule_struct(job_schedule[i]);
	}

	mu_assert("test_zero_queue_non_blocking: Testing driver non blocking queue" , count == 8);

	driver_close(driver);
	driver_destroy(driver);

	return NULL;

}

char* test_stress() {
	print_test_details(__func__, "Stress Testing for buffer");
	run_stress(1, "topology.txt");
	run_stress(1, "connected_topology.txt");
	run_stress(1, "random_topology.txt");
	run_stress(1, "random_topology_1.txt");
	run_stress(1, "big_graph.txt");
	return NULL;
}

char* test_zero_buffer_stress() {
	print_test_details(__func__, "Stress Testing for zero buffer");
	run_stress(0, "topology.txt");
	run_stress(0, "connected_topology.txt");
	run_stress(0, "random_topology.txt");
	run_stress(0, "random_topology_1.txt");
	run_stress(0, "big_graph.txt");
	return NULL;
}

static char* test_for_basic_global_declaration() {
	print_test_details(__func__, "Testing for global declaration");

	/* This test checks the CPU utilization of all the schedule, handle , select APIs.     
	*/
	size_t DRIVERS = 2;
	size_t THREADS = 200;
	pthread_t pid[THREADS], pid_1;
	driver_t* driver[DRIVERS];
	select_t list[1];
	select_t list1[1];

	// NUmber of drivers
	driver[0] = driver_create(1);
	list[0].op = HANDLE;
	list[0].driver = driver[0];

	driver[1] = driver_create(1);
	list1[0].op = HANDLE;
	list1[0].driver = driver[1];

	select_args *args[THREADS];

	for (size_t i = 0; i < THREADS; i++) {
		args[i] = create_object_for_select_api(list, 1, NULL);
		pthread_create(&pid[i], NULL, (void *)helper_select, args[i]);
	}

	// To wait sometime before we check select is blocking now.
	select_args *args1;
	args1 = create_object_for_select_api(list1, 1, NULL);
	pthread_create(&pid_1, NULL, (void *)helper_select, args1);

	usleep(100);
	mu_assert("test_for_basic_global_declaration: It isn't blocked as expected", args1->out == DRIVER_GEN_ERROR);

	driver_schedule(driver[1], "Message1");

	// XXX: Code will go in infinite loop here if the select didn't return. 
	pthread_join(pid_1, NULL);
	mu_assert("test_for_basic_global_declaration: Returned value doesn't match", args1->index == 0);
	mu_assert("test_for_basic_global_declaration: Returned value doesn't match", string_equal(list1[0].job, "Message1"));

	struct rusage usage1;
	struct rusage usage2;

	getrusage(RUSAGE_SELF, &usage1);
	struct timeval start = usage1.ru_utime;
	struct timeval start_s = usage1.ru_stime;
	for (size_t i = 0; i < 1000; i++) {
		pthread_create(&pid_1, NULL, (void *)helper_select, args1);
		usleep(100);
		driver_schedule(driver[1], "Message1");
		pthread_join(pid_1, NULL);
	}
	getrusage(RUSAGE_SELF, &usage2);
	struct timeval end = usage2.ru_utime;
	struct timeval end_s = usage2.ru_stime;

	long double result = (end.tv_sec - start.tv_sec)*1000000L + end.tv_usec - start.tv_usec + (end_s.tv_sec - start_s.tv_sec)*1000000L + end_s.tv_usec - start_s.tv_usec;
	mu_assert("test_for_basic_global_declaration: CPU Utilization is higher than required", result < 10000000);

	for (size_t i = 0; i < THREADS; i++) {
		driver_schedule(driver[0], "Some");
	}

	for (size_t i = 0; i < THREADS; i++) {
		pthread_join(pid[i], NULL);
	}

	for (size_t i = 0; i < THREADS; i++) {
		destroy_select_struct(args[i]);
	}
	destroy_select_struct(args1);

	for (size_t i = 0; i < DRIVERS; i++) {
		driver_close(driver[i]);
		driver_destroy(driver[i]);
	}
	return NULL;

} 

static char* test_select_and_non_blocking_schedule(size_t capacity) {

	size_t DRIVERS = 1;

	pthread_t pid;
	driver_t* driver[DRIVERS];
	select_t list[DRIVERS];

	for (size_t i = 0; i < DRIVERS; i++) {
		driver[i] = driver_create(capacity);
		list[i].op = HANDLE;
		list[i].driver = driver[i];
	}

	// Testing with empty drivers and handle API
	select_args *args = create_object_for_select_api(list, DRIVERS, NULL);
	pthread_create(&pid, NULL, (void *)helper_select, args);

	// To wait sometime before we check select is blocking now  
	usleep(10000);
	mu_assert("test_select_and_non_blocking_schedule: It isn't blocked as expected", args->out == DRIVER_GEN_ERROR);

	mu_assert("test_select_and_non_blocking_schedule: Non-blocking schedule failed", driver_non_blocking_schedule(driver[0], "Message") == SUCCESS);
	pthread_join(pid, NULL);    
	mu_assert("test_select_and_non_blocking_schedule: Select failed", args->out == SUCCESS);
	mu_assert("test_select_and_non_blocking_schedule: Received wrong index", args->index == 0);
	mu_assert("test_select_and_non_blocking_schedule: Received wrong message", string_equal(list[0].job, "Message"));

	destroy_select_struct(args);

	for (size_t i = 0; i < DRIVERS; i++) {
		driver_close(driver[i]);
		driver_destroy(driver[i]);
	}

	return NULL;
}

static char* test_select_and_non_blocking_handle(size_t capacity) {

	size_t DRIVERS = 1;

	pthread_t pid;
	driver_t* driver[DRIVERS];
	select_t list[DRIVERS];

	for (size_t i = 0; i < DRIVERS; i++) {
		driver[i] = driver_create(capacity);
		list[i].op = SCHDLE;
		list[i].driver = driver[i];
		list[i].job = "Message";
	}

	for (size_t i = 0; i < capacity; i++) {
		driver_schedule(driver[0], "Message");
	}

	// Testing with empty drivers and handle API
	select_args *args = create_object_for_select_api(list, DRIVERS, NULL);
	pthread_create(&pid, NULL, (void *)helper_select, args);

	// To wait sometime before we check select is blocking now  
	usleep(10000);

	mu_assert("test_select_and_non_blocking_handle: It isn't blocked as expected", args->out == DRIVER_GEN_ERROR);

	void *job;
	mu_assert("test_select_and_non_blocking_handle: Non-blocking handle failed", driver_non_blocking_handle(driver[0], &job) == SUCCESS);
	pthread_join(pid, NULL);
	mu_assert("test_select_and_non_blocking_handle: Select failed", args->out == SUCCESS);
	mu_assert("test_select_and_non_blocking_handle: Received wrong index", args->index == 0);
	mu_assert("test_select_and_non_blocking_handle: Received wrong message", string_equal(job, "Message"));

	destroy_select_struct(args);

	for (size_t i = 0; i < DRIVERS; i++) {
		driver_close(driver[i]);
		driver_destroy(driver[i]);
	}

	return NULL;
}

static char* test_select_and_non_blocking_handle_queueed() {
	print_test_details(__func__, "Testing select and non-blocking handle : queueed");
	return test_select_and_non_blocking_handle(1);
}

static char* test_select_and_non_blocking_handle_unqueueed() {
	print_test_details(__func__, "Testing select and non-blocking handle : unqueueed");
	return test_select_and_non_blocking_handle(0);
}

static char* test_select_and_non_blocking_schedule_queueed() {
	print_test_details(__func__, "Testing select and non-blocking schedule : queueed");
	return test_select_and_non_blocking_schedule(1);
}

static char* test_select_and_non_blocking_schedule_unqueueed() {
	print_test_details(__func__, "Testing select and non-blocking schedule : unqueueed");
	return test_select_and_non_blocking_schedule(0);
}

static char* test_select_with_select(size_t capacity) {

	/* Two selects with different operations on same driver */
	size_t SELECT = 2;
	size_t DRIVERS = 1;

	pthread_t pid, pid_1;
	driver_t* driver[1];
	select_t list[SELECT][DRIVERS];

	driver[0] = driver_create(capacity);
	list[0][0].op = SCHDLE;
	list[0][0].driver = driver[0];
	list[0][0].job = "Message";

	list[1][0].op = HANDLE;
	list[1][0].driver = driver[0];

	select_args *args = create_object_for_select_api(list[1], DRIVERS, NULL);

	pthread_create(&pid, NULL, (void *)helper_select, args);

	// To wait sometime before we check select is blocking now  
	usleep(10000);
	mu_assert("test_select_with_select: It isn't blocked as expected", args->out == DRIVER_GEN_ERROR);

	select_args *args1 = create_object_for_select_api(list[0], DRIVERS, NULL);
	pthread_create(&pid_1, NULL, (void *)helper_select, args1);

	pthread_join(pid_1, NULL);
	pthread_join(pid, NULL);


	mu_assert("test_select_with_select: Select failed", args->out == SUCCESS);
	mu_assert("test_select_with_select: Select failed", args1->out == SUCCESS);
	mu_assert("test_select_with_select: Received wrong index", args->index == 0);
	mu_assert("test_select_with_select: Received wrong index", args1->index == 0);
	mu_assert("test_select_with_select: wrong message recieved", string_equal(list[1][0].job, "Message"));


	destroy_select_struct(args);
	destroy_select_struct(args1);        

	for (size_t i = 0; i < DRIVERS; i++) {
		driver_close(driver[i]);
		driver_destroy(driver[i]);
	}

	return NULL;
}

static char* test_select_with_select_queueed () {
	print_test_details(__func__, "Testing select with select : queueed");
	return test_select_with_select(1);
}

static char* test_select_with_select_unqueueed () {
	print_test_details(__func__, "Testing select with select : unqueueed");
	return test_select_with_select(0);
}

static char* test_selects_with_same_driver (size_t capacity) {

	/* Testing with 3 selects handle on same two drivers. Only two should be able to process schedule */
	size_t SELECT = 3;
	size_t DRIVERS = 2;

	pthread_t pid[SELECT];
	driver_t* driver[DRIVERS];
	select_t list[SELECT][DRIVERS];

	for (size_t j = 0; j < DRIVERS; j++) {
		driver[j] = driver_create(capacity);
		for (size_t i = 0; i < SELECT; i++) {
			list[i][j].op = HANDLE;
			list[i][j].driver = driver[j];
		}
	}

	sem_t done;
	sem_init(&done, 0, 0);

	select_args *args[SELECT];
	for (size_t i = 0; i < SELECT; i++) {
		args[i] = create_object_for_select_api(list[i], DRIVERS, &done);
		pthread_create(&pid[i], NULL, (void *)helper_select, args[i]);
	}

	for (size_t j = 0; j < DRIVERS; j++) {
		driver_schedule(driver[j], "Message");
	}
	for (size_t j = 0; j < DRIVERS; j++) {
		sem_wait(&done);
	}

	size_t success_count = 0;
	for (size_t i = 0; i < SELECT; i++) {
		if (args[i]->out == SUCCESS) {
			success_count++;
			mu_assert("test_select_with_same_driver: Wrong message", string_equal(args[i]->select_list[args[i]->index].job, "Message"));
		}
	}
	mu_assert("test_select_with_same_driver: Only two drivers should handle", success_count == DRIVERS);

	for (size_t i = 0; i < (SELECT - DRIVERS); i++) {
		driver_schedule(driver[0], "Message2");
		sem_wait(&done);
	}

	success_count = 0;
	size_t success_count2 = 0;
	for (size_t i = 0; i < SELECT; i++) {
		if (args[i]->out == SUCCESS) {
			if (string_equal(args[i]->select_list[args[i]->index].job, "Message")) {
				success_count++;
			} else if (string_equal(args[i]->select_list[args[i]->index].job, "Message2")) {
				success_count2++;
			} else {
				mu_assert("test_select_with_same_driver: Wrong message", false);
			}
		}
	}
	mu_assert("test_select_with_same_driver: Only original two drivers should handle", success_count == DRIVERS);
	mu_assert("test_select_with_same_driver: All other drivers should handle second message", success_count2 == (SELECT - DRIVERS));

	for (size_t i = 0; i < SELECT; i++) {
		pthread_join(pid[i], NULL);
		destroy_select_struct(args[i]);
	}

	for (size_t i = 0; i < DRIVERS; i++) {
		driver_close(driver[i]);
		driver_destroy(driver[i]);
	}

	return NULL;
}

static char* test_selects_with_same_driver_queueed() {
	print_test_details(__func__, "Testing selects with same driver: queueed");
	return test_selects_with_same_driver(1);
}

static char* test_selects_with_same_driver_unqueueed() {
	print_test_details(__func__, "Testing selects with same driver : unqueueed");
	return test_selects_with_same_driver(0);
}

static char* test_select_with_schedule_handle_on_same_driver(size_t capacity) {

	pthread_t pid[2];
	driver_t* driver = driver_create(capacity);
	select_t list[2][2];
	list[0][0].op = HANDLE;
	list[0][0].driver = driver;
	list[0][0].job = (void*)0xdeadbeef;
	list[0][1].op = SCHDLE;
	list[0][1].driver = driver;
	list[0][1].job = "Message1";
	list[1][0].op = HANDLE;
	list[1][0].driver = driver;
	list[1][0].job = (void*)0xdeadbeef;
	list[1][1].op = SCHDLE;
	list[1][1].driver = driver;
	list[1][1].job = "Message2";

	select_args *args[2];
	args[0] = create_object_for_select_api(list[0], 2, NULL);
	pthread_create(&pid[0], NULL, (void *)helper_select, args[0]);
	args[1] = create_object_for_select_api(list[1], 2, NULL);
	pthread_create(&pid[1], NULL, (void *)helper_select, args[1]);

	pthread_join(pid[0], NULL);
	pthread_join(pid[1], NULL);

	mu_assert("test_select_with_schedule_handle_on_same_driver: Failed select", args[0]->out == SUCCESS);
	mu_assert("test_select_with_schedule_handle_on_same_driver: Failed select", args[1]->out == SUCCESS);
	if (args[0]->index == 0) {
		mu_assert("test_select_with_schedule_handle_on_same_driver: Invalid index", args[0]->index == 0);
		mu_assert("test_select_with_schedule_handle_on_same_driver: Invalid index", args[1]->index == 1);
		mu_assert("test_select_with_schedule_handle_on_same_driver: Invalid message", string_equal(list[0][0].job, "Message2"));
		mu_assert("test_select_with_schedule_handle_on_same_driver: Overwrote job", list[1][0].job == (void*)0xdeadbeef);
	} else {
		mu_assert("test_select_with_schedule_handle_on_same_driver: Invalid index", args[0]->index == 1);
		mu_assert("test_select_with_schedule_handle_on_same_driver: Invalid index", args[1]->index == 0);
		mu_assert("test_select_with_schedule_handle_on_same_driver: Invalid message", string_equal(list[1][0].job, "Message1"));
		mu_assert("test_select_with_schedule_handle_on_same_driver: Overwrote job", list[0][0].job == (void*)0xdeadbeef);
	}

	destroy_select_struct(args[0]);
	destroy_select_struct(args[1]);

	driver_close(driver);
	driver_destroy(driver);
	return NULL;
}

static char* test_select_with_schedule_handle_on_same_driver_queueed() {
	print_test_details(__func__, "Testing selects with schedule/handle on same driver: queueed");
	return test_select_with_schedule_handle_on_same_driver(1);
}

static char* test_select_with_schedule_handle_on_same_driver_unqueueed() {
	print_test_details(__func__, "Testing selects with schedule/handle on same driver: unqueueed");
	return test_select_with_schedule_handle_on_same_driver(0);
}

static char* test_select_with_duplicate_driver(size_t capacity) {

	// test duplicate handle
	pthread_t pid;
	driver_t* driver = driver_create(capacity);
	select_t list[2];
	list[0].op = HANDLE;
	list[0].driver = driver;
	list[0].job = (void*)0xdeadbeef;
	list[1].op = HANDLE;
	list[1].driver = driver;
	list[1].job = (void*)0xdeadbeef;

	select_args *args;
	args = create_object_for_select_api(list, 2, NULL);
	pthread_create(&pid, NULL, (void *)helper_select, args);

	mu_assert("test_select_with_duplicate_driver: Send failed", driver_schedule(driver, "Message") == SUCCESS);
	pthread_join(pid, NULL);

	mu_assert("test_select_with_duplicate_driver: Failed select", args->out == SUCCESS);
	mu_assert("test_select_with_duplicate_driver: Invalid index", args->index == 0);
	mu_assert("test_select_with_duplicate_driver: Invalid message", string_equal(list[0].job, "Message"));

	destroy_select_struct(args);

	// test duplicate schedule
	for (size_t i = 0; i < capacity; i++) {
		driver_schedule(driver, "Message");
	}
	list[0].op = SCHDLE;
	list[0].driver = driver;
	list[0].job = "Message1";
	list[1].op = SCHDLE;
	list[1].driver = driver;
	list[1].job = "Message2";

	args = create_object_for_select_api(list, 2, NULL);
	pthread_create(&pid, NULL, (void *)helper_select, args);

	void* job = NULL;
	mu_assert("test_select_with_duplicate_driver: Receive failed", driver_handle(driver, &job) == SUCCESS);
	pthread_join(pid, NULL);

	for (size_t i = 0; i < capacity; i++) {
		mu_assert("test_select_with_duplicate_driver: Invalid message", string_equal(job, "Message"));
		mu_assert("test_select_with_duplicate_driver: Receive failed", driver_handle(driver, &job) == SUCCESS);
	}
	mu_assert("test_select_with_duplicate_driver: Invalid message", string_equal(job, "Message1"));

	destroy_select_struct(args);

	driver_close(driver);
	driver_destroy(driver);
	return NULL;
}

static char* test_select_with_duplicate_driver_queueed() {
	print_test_details(__func__, "Testing selects with duplicate operations on same driver: queueed");
	return test_select_with_duplicate_driver(1);
}

static char* test_select_with_duplicate_driver_unqueueed() {
	print_test_details(__func__, "Testing selects with duplicate operations on same driver: unqueueed");
	return test_select_with_duplicate_driver(0);
}

typedef char* (*test_fn_t)();
typedef struct {
	char* name;
	test_fn_t test;
} test_t;
typedef struct {
	test_fn_t test;
	int grade;
} grade_t;
test_t tests[] = {{"test_initialization", test_initialization},
	{"test_schedule_correctness", test_schedule_correctness},
	{"test_handle_correctness", test_handle_correctness},
	{"test_non_blocking_schedule", test_non_blocking_schedule},
	{"test_non_blocking_handle", test_non_blocking_handle},
	{"test_driver_close_with_schedule", test_driver_close_with_schedule},
	{"test_driver_close_with_handle", test_driver_close_with_handle},
	{"test_multiple_drivers", test_multiple_drivers},
	{"test_response_time", test_response_time},
	{"test_overall_schedule_handle", test_overall_schedule_handle},
	{"test_free", test_free},
	{"test_cpu_utilization_for_schedule_handle", test_cpu_utilization_for_schedule_handle},
	{"test_select", test_select},
	{"test_select_and_non_blocking_handle_queueed", test_select_and_non_blocking_handle_queueed},
	{"test_select_and_non_blocking_schedule_queueed", test_select_and_non_blocking_schedule_queueed},
	{"test_select_with_select_queueed", test_select_with_select_queueed},
	{"test_selects_with_same_driver_queueed", test_selects_with_same_driver_queueed},
	{"test_select_with_schedule_handle_on_same_driver_queueed", test_select_with_schedule_handle_on_same_driver_queueed},
	{"test_select_with_duplicate_driver_queueed", test_select_with_duplicate_driver_queueed},
	{"test_for_basic_global_declaration", test_for_basic_global_declaration},
	{"test_stress", test_stress},
	{"test_zero_queue", test_zero_queue},
	{"test_zero_queue_non_blocking", test_zero_queue_non_blocking},
	{"test_select_and_non_blocking_handle_unqueueed", test_select_and_non_blocking_handle_unqueueed},
	{"test_select_and_non_blocking_schedule_unqueueed", test_select_and_non_blocking_schedule_unqueueed},
	{"test_select_with_select_unqueueed", test_select_with_select_unqueueed},
	{"test_selects_with_same_driver_unqueueed", test_selects_with_same_driver_unqueueed},
	{"test_select_with_schedule_handle_on_same_driver_unqueueed", test_select_with_schedule_handle_on_same_driver_unqueueed},
	{"test_select_with_duplicate_driver_unqueueed", test_select_with_duplicate_driver_unqueueed},
	{"test_zero_buffer_stress", test_zero_buffer_stress},
	{"test_cpu_utilization_for_select", test_cpu_utilization_for_select}};

size_t num_tests = sizeof(tests)/sizeof(tests[0]);
static char* single_test(test_fn_t test, size_t iters) {
	for (size_t i = 0; i < iters; i++) {
		mu_run_test(test);
	}
	return NULL;
}

static char* all_tests(size_t iters) {
	for (size_t i = 0; i < num_tests; i++) {
		char* result = single_test(tests[i].test, iters);
		if (result != NULL) {
			return result;
		}
	}
	return NULL;
}

int main(int argc, char** argv) {
	char* result = NULL;
	size_t iters = 1;
	if (argc == 1) {
		result = all_tests(iters);
		return result != NULL;
	} else if (argc == 3) {
		iters = (size_t)atoi(argv[2]);
	} else if (argc > 3) {
		printf("Wrong number of arguments, only one test is accepted at time");
	}

	result = "Did not find test";

	for (size_t i = 0; i < num_tests; i++) {
		if (string_equal(argv[1], tests[i].name)) {
			result = single_test(tests[i].test, iters);
			break;
		}
	}
	if (result) {
		printf("%s\n", result);
	}
	return result != NULL;
}
