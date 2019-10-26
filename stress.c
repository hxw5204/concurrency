#include "stress.h"

typedef unsigned int distance_t;
typedef struct {
    size_t src;
    size_t epoch;
    distance_t dist[0];
} distance_vector_t;

const distance_t inf_distance = 0x7fffffff;
distance_t* topology;
distance_t* solution;
size_t num_driver;
driver_t** drivers;
driver_t* done_driver;
driver_t* completed_driver;

distance_t get_link_distance(size_t src, size_t dst) {
    return topology[src * num_driver + dst];
}

void set_link_distance(size_t src, size_t dst, distance_t distance) {
    topology[src * num_driver + dst] = distance;
}

distance_t get_solution_distance(size_t src, size_t dst) {
    return solution[src * num_driver + dst];
}

void set_solution_distance(size_t src, size_t dst, distance_t distance) {
    solution[src * num_driver + dst] = distance;
}

void floyd_warshall()
{
    memcpy(solution, topology, sizeof(distance_t) * num_driver * num_driver);
    for (size_t intermediate = 0; intermediate < num_driver; intermediate++) {
        for (size_t src = 0; src < num_driver; src++) {
            for (size_t dst = 0; dst < num_driver; dst++) {
                if (get_solution_distance(src, intermediate) + get_solution_distance(intermediate, dst) < get_solution_distance(src, dst)) {
                    set_solution_distance(src, dst, get_solution_distance(src, intermediate) + get_solution_distance(intermediate, dst));
                }
            }
        }
    }
}

void print_graph()
{
    printf("GRAPH\n");
    for (size_t src = 0; src < num_driver; src++) {
        for (size_t dst = 0; dst < num_driver; dst++) {
            distance_t distance = get_link_distance(src, dst);
            if (distance == inf_distance) {
                printf("inf ");
            } else {
                printf("%03u ", distance);
            }
        }
        printf("\n");
    }
}

void print_solution()
{
    printf("SOLUTION\n");
    for (size_t src = 0; src < num_driver; src++) {
        for (size_t dst = 0; dst < num_driver; dst++) {
            distance_t distance = get_solution_distance(src, dst);
            if (distance == inf_distance) {
                printf("inf ");
            } else {
                printf("%03u ", distance);
            }
        }
        printf("\n");
    }
}

bool create_topology(const char* filename)
{
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Could not open topology file: %s\n", filename);
        return false;
    }
    int num_scanned = fscanf(file, "%zu", &num_driver);
    assert(num_scanned == 1);
    assert(num_driver > 0);
    topology = malloc(sizeof(distance_t) * num_driver * num_driver);
    assert(topology != NULL);
    solution = malloc(sizeof(distance_t) * num_driver * num_driver);
    assert(solution != NULL);
    // populate topology
    for (size_t src = 0; src < num_driver; src++) {
        for (size_t dst = 0; dst < num_driver; dst++) {
            distance_t distance;
            num_scanned = fscanf(file, "%d", (int*)&distance);
            assert(num_scanned == 1);
            // negative values get converted to inf_distance
            if (distance > inf_distance) {
                distance = inf_distance;
            }
            set_link_distance(src, dst, distance);
        }
    }
    fclose(file);
    // calculate solution using Floyd-Warshall algorithm
    floyd_warshall();
    return true;
}

void destroy_topology()
{
    free(topology);
    free(solution);
}

void* router(void* arg)
{
    bool driverged = false;
    size_t index = (size_t)arg;
    size_t selected_index;
    distance_vector_t* prev_state = malloc(sizeof(distance_vector_t) + sizeof(distance_t) * num_driver);
    assert(prev_state != NULL);
    distance_vector_t* curr_state = malloc(sizeof(distance_vector_t) + sizeof(distance_t) * num_driver);
    assert(curr_state != NULL);
    distance_vector_t* next_state = malloc(sizeof(distance_vector_t) + sizeof(distance_t) * num_driver);
    assert(next_state != NULL);
    prev_state->src = index;
    curr_state->src = index;
    next_state->src = index;
    prev_state->epoch = 0;
    curr_state->epoch = 1;
    next_state->epoch = 2;
    for (size_t i = 0; i < num_driver; i++) {
        prev_state->dist[i] = get_link_distance(index, i);
        curr_state->dist[i] = get_link_distance(index, i);
        next_state->dist[i] = get_link_distance(index, i);
    }
    size_t total_select_count = 2;
    for (size_t i = 0; i < num_driver; i++) {
        if ((i != index) && get_link_distance(index, i) != inf_distance) {
            total_select_count++;
        }
    }
    select_t* select_list = malloc(sizeof(select_t) * total_select_count);
    assert(select_list != NULL);
    size_t select_count = 0;
    select_list[select_count].driver = done_driver;
    select_list[select_count].op = HANDLE;
    select_list[select_count].job = NULL;
    select_count++;
    select_list[select_count].driver = drivers[index];
    select_list[select_count].op = HANDLE;
    select_list[select_count].job = NULL;
    select_count++;
    for (size_t i = 0; i < num_driver; i++) {
        if ((i != index) && get_link_distance(index, i) != inf_distance) {
            select_list[select_count].driver = drivers[i];
            select_list[select_count].op = SCHDLE;
            select_list[select_count].job = curr_state;
            select_count++;
        }
    }
    while (true) {
        enum driver_status status = driver_select(select_list, select_count, &selected_index);
        if (status == SUCCESS) {
            assert(selected_index != 0);
            if (selected_index == 1) {
                if (select_list[selected_index].job) {
                    // update next_state with new job
                    distance_vector_t* neighbor_state = select_list[selected_index].job;
                    distance_t neighbor_dist = get_link_distance(index, neighbor_state->src);
                    assert(neighbor_dist != inf_distance);
                    for (size_t i = 0; i < num_driver; i++) {
                        distance_t new_dist = neighbor_dist + neighbor_state->dist[i];
                        if (new_dist < next_state->dist[i]) {
                            next_state->dist[i] = new_dist;
                            driverged = true;
                        }
                    }
                } else {
                    // special message sent to test convergence
                    bool converged = (select_count == 2) && !driverged;
                    status = driver_schedule(completed_driver, converged ? curr_state : NULL);
                    assert(status == SUCCESS);
                }
            } else {
                select_count--;
                // swap last element and selected element
                driver_t* temp = select_list[select_count].driver;
                select_list[select_count].driver = select_list[selected_index].driver;
                select_list[selected_index].driver = temp;
            }
            // check if we've sent to everyone
            if (select_count == 2) {
                // check if we want to reset
                if (driverged) {
                    // cycle triple buffer
                    distance_vector_t* temp_state = curr_state;
                    curr_state = next_state;
                    next_state = prev_state;
                    prev_state = temp_state;
                    next_state->epoch = curr_state->epoch + 1;
                    for (size_t i = 0; i < num_driver; i++) {
                        next_state->dist[i] = curr_state->dist[i];
                    }
                    // reset to broadcast again
                    select_count = total_select_count;
                    for (size_t i = 2; i < select_count; i++) {
                        select_list[i].job = curr_state;
                    }
                    driverged = false;
                }
            }
        } else {
            assert(status == DRIVER_CLOSED_ERROR);
            assert(selected_index == 0);
            assert(driverged == false);
            break;
        }
    }
    free(select_list);
    free(prev_state);
    free(curr_state);
    free(next_state);
    return NULL;
}

bool check_done()
{
    bool valid = true;
    enum driver_status status;
    distance_vector_t** completed = malloc(sizeof(distance_vector_t*) * num_driver);
    assert(completed != NULL);
    // validate by scheduleing special NULL message to flush driver
    for (size_t i = 0; i < num_driver; i++) {
        status = driver_schedule(drivers[i], NULL);
        assert(status == SUCCESS);
    }
    // handle special response
    for (size_t i = 0; i < num_driver; i++) {
        void* job = NULL;
        status = driver_handle(completed_driver, &job);
        assert(status == SUCCESS);
        if (job == NULL) {
            valid = false;
        } else {
            distance_vector_t* new_job = (distance_vector_t*)job;
            size_t index = new_job->src;
            completed[index] = new_job;
        }
    }
    if (valid) {
        // ensure epoch hasn't driverged since first validation
        for (size_t i = 0; i < num_driver; i++) {
            status = driver_schedule(drivers[i], NULL);
            assert(status == SUCCESS);
        }
        // handle special response
        for (size_t i = 0; i < num_driver; i++) {
            void* job = NULL;
            status = driver_handle(completed_driver, &job);
            assert(status == SUCCESS);
            if (job == NULL) {
                valid = false;
            } else {
                distance_vector_t* new_job = (distance_vector_t*)job;
                size_t index = new_job->src;
                if (completed[index]->epoch != new_job->epoch) {
                    valid = false;
                }
            }
        }
        if (valid) {
            // check results
            for (size_t src = 0; src < num_driver; src++) {
                for (size_t dst = 0; dst < num_driver; dst++) {
                    assert(completed[src]->dist[dst] == get_solution_distance(src, dst));
                }
            }
        }
    }
    free(completed);
    return valid;
}

void run_stress(size_t buffer_size, const char* filename)
{
    assert(buffer_size <= 1); // only support up to a buffer size of 1
    int pthread_status;
    enum driver_status status;
    bool initialized = create_topology(filename);
    assert(initialized);
    drivers = malloc(sizeof(driver_t*) * num_driver);
    assert(drivers != NULL);
    for (size_t i = 0; i < num_driver; i++) {
        drivers[i] = driver_create(buffer_size);
        assert(drivers[i] != NULL);
    }
    done_driver = driver_create(buffer_size);
    assert(done_driver != NULL);
    completed_driver = driver_create(buffer_size);
    assert(completed_driver != NULL);

    pthread_t* pid = malloc(sizeof(pthread_t) * num_driver);
    assert(pid != NULL);
    for (size_t i = 0; i < num_driver; i++) {
        pthread_status = pthread_create(&pid[i], NULL, router, (void*)i);
        assert(pthread_status == 0);
    }

    // wait for convergence
    while (!check_done()) {
        usleep(1000);
    }

    // stop threads
    status = driver_close(done_driver);
    assert(status == SUCCESS);
    // join threads
    for (size_t i = 0; i < num_driver; i++) {
        pthread_join(pid[i], NULL);
    }
    // cleanup
    status = driver_destroy(done_driver);
    assert(status == SUCCESS);
    status = driver_close(completed_driver);
    assert(status == SUCCESS);
    status = driver_destroy(completed_driver);
    assert(status == SUCCESS);
    for (size_t i = 0; i < num_driver; i++) {
        status = driver_close(drivers[i]);
        assert(status == SUCCESS);
        status = driver_destroy(drivers[i]);
        assert(status == SUCCESS);
    }
    free(pid);
    free(drivers);
    destroy_topology();
}
