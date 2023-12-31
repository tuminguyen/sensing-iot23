#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "thread.h"
#include "ztimer.h"
#include "shell.h"
#include "mutex.h"
#include "net/sock/udp.h"
#include "net/ipv6/addr.h"

/* lps331ap (temperature/pressure) header */
#include "lpsxxx.h"
#include "lpsxxx_params.h"

/* lsm303dlhc (accelerometer/magnetometer) header */
#include "lsm303dlhc.h"
#include "lsm303dlhc_params.h"

#define SERVER_IPV6_ADDR "2a05:d016:4b4:100:2323:6378:f552:8961"
#define SERVER_PORT 8888

static sock_udp_ep_t remote;

/* Declare the lps331ap device variable */
static lpsxxx_t lpsxxx;

/* Declare the lsm303dlhc device variable */
static lsm303dlhc_t lsm303dlhc;

/* Declare and initialize the thread locks */
static mutex_t lsm303dlhc_mutex = MUTEX_INIT;
static bool lsm303dlhc_reading = false;

static mutex_t lpsxxx_mutex = MUTEX_INIT;
static bool lpsxxx_reading = false;

/* stack memory allocated for the lsm303dlhc thread */
static char lsm303dlhc_stack[THREAD_STACKSIZE_MAIN]; 

static char lpsxxx_stack[THREAD_STACKSIZE_MAIN]; 

/* THREAD HANDLERRRRRRR */
static void *lsm303dlhc_thread(void *arg)
{
    (void)arg;
    while (1) {
        /* Acquire the mutex */
        mutex_lock(&lsm303dlhc_mutex);

        /* Read the accelerometer/magnetometer values */
        lsm303dlhc_3d_data_t acc;
        lsm303dlhc_3d_data_t mag;
        if (lsm303dlhc_reading) {
            lsm303dlhc_read_acc(&lsm303dlhc, &acc);
            lsm303dlhc_read_mag(&lsm303dlhc, &mag);
            // printf("Accelerometer x: %i y: %i z: %i\n", acc.x_axis, acc.y_axis, acc.z_axis);
            // printf("Magnetometer x: %i y: %i z: %i\n", mag.x_axis, mag.y_axis, mag.z_axis);
            
            char msg[128];
            snprintf(msg, sizeof(msg), "Accelerometer (x:%i, y:%i, z:%i) | Magnetometer (x:%i, y:%i, z:%i)", 
            acc.x_axis, acc.y_axis, acc.z_axis, mag.x_axis, mag.y_axis, mag.z_axis);

            ssize_t res = sock_udp_send(NULL, msg, strlen(msg), &remote);
            if (res < 0) {
                printf("Error sending message: %d\n", (int)res);
            } else {
                printf("[MESSAGE]: \"%s\"\n", msg);
            }
        }
        /* Release the mutex */
        mutex_unlock(&lsm303dlhc_mutex);

        ztimer_sleep(ZTIMER_MSEC, 3000);
    }

    return 0;
}


static void *lpsxxx_thread(void *arg)
{
    (void)arg;
    while (1) {
        /* Acquire the mutex */
        mutex_lock(&lpsxxx_mutex);

        /* Read the accelerometer/magnetometer values */
        int16_t temp = 0;
        uint16_t pres = 0;
        
        if (lpsxxx_reading) {
            lpsxxx_read_temp(&lpsxxx, &temp);
            lpsxxx_read_pres(&lpsxxx, &pres);
            // printf("Temperature: %i.%u°C\n", (temp / 100), (temp % 100));
            // printf("Pressure: %uhPa\n", pres);

            char msg[128];
            snprintf(msg, sizeof(msg), "Temp: %i.%u°C | Pressure: %uhPa", 
                     temp / 100, temp % 100, pres);
            ssize_t res = sock_udp_send(NULL, msg, strlen(msg), &remote);
            if (res < 0) {
                printf("Error sending message: %d\n", (int)res);
            } else {
                printf("[MESSAGE]: \"%s\"\n", msg);
            }
        }
        /* Release the mutex */
        mutex_unlock(&lpsxxx_mutex);

        ztimer_sleep(ZTIMER_MSEC, 2000);
    }

    return 0;
}

/* COMMAND USAGE */

static void _lsm303dlhc_usage(char *cmd)
{
    printf("usage: %s <on|off>\n", cmd);
}

static int lsm303dlhc_handler(int argc, char *argv[])
{
    if (argc < 2) {
        _lsm303dlhc_usage(argv[0]);
        return -1;
    }

    if (!strcmp(argv[1], "on")) {
        lsm303dlhc_enable(&lsm303dlhc);
        lsm303dlhc_reading = true; // Start sensor reading
    }
    else if (!strcmp(argv[1], "off")) {
        lsm303dlhc_disable(&lsm303dlhc);
        lsm303dlhc_reading = false;
    }
    else {
        _lsm303dlhc_usage(argv[0]);
        return -1;
    }

    return 0;
}

static void _lpsxxx_usage(char *cmd)
{
    printf("usage: %s <on|off>\n", cmd);
}

static int lpsxxx_handler(int argc, char *argv[])
{
    if (argc < 2) {
        _lpsxxx_usage(argv[0]);
        return -1;
    }

    if (!strcmp(argv[1], "on")) {
        lpsxxx_enable(&lpsxxx);
        lpsxxx_reading = true;
    }
    else if (!strcmp(argv[1], "off")) {
        lpsxxx_disable(&lpsxxx);
        lpsxxx_reading = false;
    }
    else {
        _lpsxxx_usage(argv[0]);
        return -1;
    }

    return 0;
}

static const shell_command_t commands[] = {
    /* lsm303dlhc shell command handler */
    { "lsm", "control lsm303dlhc reading values - accelerometer and magnetometer", lsm303dlhc_handler },

    /* lps331ap command description */
    { "lps", "control lps331ap reading values - temperature and pressure", lpsxxx_handler },

    { NULL, NULL, NULL}
};

int main(void)
{
    /* Initialize the sensors */
    lpsxxx_init(&lpsxxx, &lpsxxx_params[0]);
    lsm303dlhc_init(&lsm303dlhc, &lsm303dlhc_params[0]);

    // Initialize the remote endpoint
    remote.family = AF_INET6;
    remote.port = SERVER_PORT;
    if (ipv6_addr_from_str((ipv6_addr_t *)&remote.addr.ipv6, SERVER_IPV6_ADDR) == NULL) {
        puts("Error: unable to parse destination address");
        return 1;
    }

    /* Thread create*/
    thread_create(lsm303dlhc_stack, sizeof(lsm303dlhc_stack), THREAD_PRIORITY_MAIN - 1,
                  0, lsm303dlhc_thread, NULL, "lsm303dlhc");
    thread_create(lpsxxx_stack, sizeof(lpsxxx_stack), THREAD_PRIORITY_MAIN - 2,
                  0, lpsxxx_thread, NULL, "lps331ap");

    /* Run shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
