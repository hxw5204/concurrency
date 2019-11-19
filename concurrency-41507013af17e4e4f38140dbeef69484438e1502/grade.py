#!/usr/bin/env python

import os
import sys
if __name__ == '__main__':
    tests = [
            ("test_initialization",2),
            ("test_schedule_correctness",5),
            ("test_handle_correctness",5),
            ("test_non_blocking_schedule",5),
            ("test_non_blocking_handle",5),
            ("test_driver_close_with_schedule",2),
            ("test_driver_close_with_handle",2),
            ("test_multiple_drivers",5),
            ("test_response_time",2),
            ("test_overall_schedule_handle",5),
            ("test_free",2),
            ("test_zero_queue",10),
            ("test_zero_queue_non_blocking",10),
            ("test_cpu_utilization_for_schedule_handle",10),
            ("test_select",5),
            ("test_select_and_non_blocking_handle_queueed",3),
            ("test_select_and_non_blocking_schedule_queueed",3),
            ("test_select_with_select_queueed",3),
            ("test_selects_with_same_driver_queueed",3),
            ("test_select_with_schedule_handle_on_same_driver_queueed",3),
            ("test_select_with_duplicate_driver_queueed",3),
            ("test_for_basic_global_declaration",3),
	    ("test_stress",4),
	]
    bonus=  [
            ("test_select_and_non_blocking_handle_unqueueed",1),
            ("test_select_and_non_blocking_schedule_unqueueed",1),
            ("test_select_with_select_unqueueed",1),
            ("test_selects_with_same_driver_unqueueed",1),
            ("test_select_with_schedule_handle_on_same_driver_unqueueed",1),
            ("test_select_with_duplicate_driver_unqueueed",1),
            ("test_cpu_utilization_for_select",2),
	    ("test_zero_buffer_stress",2),
            ]
    memoryLeakGrade = 1
    memoryErrorGrade = 1
    bonusTest = 0
    if (len(sys.argv) == 2):
        bonusTest = str(sys.argv[1])
    points = 0
    totalValue = 0
    statusValgrind = 1
    print( "############################")
    print( "# RUNNING CORRECTNESS TEST #")
    print( "############################")
    for testName, testValue in tests:
        status = os.system("./driver %s" %(testName))
        totalValue += testValue
        if status == 0:
            print ("[INFO]  %s [%dpts]: passed" %(testName, testValue))
            points += testValue
        else:
            print ("[ERROR] %s [%dpts]: failed" %(testName, testValue))
    print ("#########################")
    print ("# RUNNING VALGRIND TEST #")
    print ("#########################")
    for testName, testValue in tests:
	totalValue += testValue
        if testName == "test_select" or testName == "test_response_time":
	    points += testValue
            continue;
        os.system('rm valgrind.log')
        os.system("valgrind --log-fd=9 9>>valgrind.log ./driver %s" %(testName))
	status = os.system("cat valgrind.log | grep -q All\ heap\ blocks\ were\ freed\ --\ no\ leaks\ are\ possible")
	if status != 0:
	    print("[ERROR] FAILURE: Valgrind detected memory leaks in your code!! Check the generated valgrind.log file for more details")
	else:
	    points += testValue
	    print("[INFO]  Valgrind did not detect any memory leaks!!")
    print ("[INFO]  Total score: %d / %d" %(points, totalValue))

    if bonusTest == str('bonus'):
        os.system('rm valgrind.log')
        print( "############################")
        print( "# RUNNING CORRECTNESS TEST #")
        print( "############################")
        for testName, testValue in bonus:
            status = os.system("./driver %s" %(testName))
            if status == 0:
                print ("[INFO]  %s [%dpts]: passed" %(testName, testValue))
                points += testValue
            else:
                print ("[ERROR] %s [%dpts]: failed" %(testName, testValue))
        print ("#########################")
        print ("# RUNNING VALGRIND TEST #")
        print ("#########################")
	for testName, testValue in bonus:
	    totalValue += testValue
            if testName == "test_select" or testName == "test_response_time":
		points += testValue
                continue;
            os.system('rm valgrind.log')
            os.system("valgrind --log-fd=9 9>>valgrind.log ./driver %s" %(testName))
            status = os.system("cat valgrind.log | grep -q All\ heap\ blocks\ were\ freed\ --\ no\ leaks\ are\ possible")
            if status != 0:
                print("[ERROR] FAILURE: Valgrind detected memory leaks in your code!! Check the generated valgrind.log file for more details")
            else:
                points += testValue

        print ("[INFO]  Bonus score: %d / %d" %(points, totalValue))

