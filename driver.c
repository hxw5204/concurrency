#include "driver.h"

driver_t* driver_create(size_t size){
	/* IMPLEMENT THIS */
}

enum driver_status driver_schedule(driver_t *driver, void* job) {
	/* IMPLEMENT THIS */
}

enum driver_status driver_handle(driver_t *driver, void **job) {
	/* IMPLEMENT THIS */
}

enum driver_status driver_non_blocking_schedule(driver_t *driver, void* job) {
	/* IMPLEMENT THIS */
}

enum driver_status driver_non_blocking_handle(driver_t *driver, void **job) {
	/* IMPLEMENT THIS */
}

enum driver_status driver_close(driver_t *driver) {
	/* IMPLEMENT THIS */
}

enum driver_status driver_destroy(driver_t *driver) {
	/* IMPLEMENT THIS */
}

enum driver_status driver_select(select_t *driver_list, size_t driver_count, size_t* selected_index) {
	/* IMPLEMENT THIS */
}
