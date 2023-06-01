#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define MSEC_TO_USEC(usec) ({int msec = 1000 * usec; msec;})

void* threadfunc(void* thread_param)
{
    // Make sure pointer is valid
    assert(thread_param != NULL);
    // Cast data
    struct thread_data* pThread_func_args = (struct thread_data*)thread_param;

    // Default to failed
    pThread_func_args->thread_complete_success = false;

    if (  (pThread_func_args->wait_to_obtain_ms >= 1000)
       || (pThread_func_args->wait_to_release_ms >= 1000)
       || (pThread_func_args->pMutex == NULL)) {
       // Return failed when invalid data is in the struct
       // Note: usleep(3) on Linux man page mentions wait time not smaller than 1.000.000us may cause an EINVAL error.
       return thread_param;
    }

    usleep(MSEC_TO_USEC(pThread_func_args->wait_to_obtain_ms));

    int result = pthread_mutex_lock(pThread_func_args->pMutex);
    if (result != 0) {
       return thread_param;
    }

    usleep(MSEC_TO_USEC(pThread_func_args->wait_to_release_ms));

    result = pthread_mutex_unlock(pThread_func_args->pMutex);
    if (result != 0) {
       return thread_param;
    }

    pThread_func_args->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    // Dynamically allocate memory for data struct (calling function should free when thread exits)
    struct thread_data* pThread_data = malloc(sizeof(pThread_data));

    // Fill struct data
    pThread_data->pMutex = mutex;
    pThread_data->wait_to_obtain_ms = wait_to_obtain_ms;
    pThread_data->wait_to_release_ms = wait_to_release_ms;
    pThread_data->thread_complete_success = false;

    // Create thread with default attributes (a.o. Detach state = PTHREAD_CREATE_JOINABLE)
    const int result = pthread_create(thread, NULL, threadfunc, (void*)pThread_data);
    return (result == 0) ? true : false;
}

