//
// Created by Roman Oberenkowski on 21.11.2021.
//
#include <wiringPi.h>
#include <bcm2835.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
//shmem
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cerrno>
//fifo
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "led_constants.h"
#include "frame_t.h"

//shared memory of the current frame
frame_t *frame;

//positioning
int current_step = 0;
uint64_t last_time;

//running average
const int running_average_init_time = 100*1000;
const int running_average_amount = 10;
int running_average_index = 0;
unsigned int running_average_tab[running_average_amount];
uint64_t running_average_sum = running_average_amount * running_average_init_time;

//synchronization
pthread_mutex_t current_step_mutex;
pthread_mutex_t spi_mutex;
pthread_mutex_t should_exit;
bool shutting_down = false;

int fifo_fd;

void init_shmem() {
    key_t key = ftok("/tmp", SHM_KEY);
    int shm_id = shmget(key, sizeof(frame_t), 0600 | IPC_CREAT);
    if (shm_id == -1 && errno == 22) {
        printf("Probably size has increased - remove old shmem...\n");
        shm_id = shmget(key, 1, 0600);
        if (shm_id != -1) {
            if (shmctl(shm_id, IPC_RMID, nullptr) != -1) {
                shm_id = shmget(key, sizeof(frame_t), 0600 | IPC_CREAT);
            }
        }
    }
    if (shm_id == -1) {
        perror("SHMGET: ");
        exit(-1);
    } else {
        printf("Shared Memory: OK!\n");
    }
    frame = (frame_t *) shmat(shm_id, NULL, 0);
    if (frame == (void *) -1) {
        perror("SHMAT: ");
        exit(-1);
    }
}

void init_running_average() {
    for (int i = 0; i < running_average_amount; i++) {
        running_average_tab[i] = running_average_init_time;
    }
    running_average_sum = running_average_init_time * running_average_amount;
    return;
}

void init_fifo(){
    mkfifo("/tmp/rotation_times.fifo",0600);
    fifo_fd = open("/tmp/rotation_times.fifo",O_WRONLY);
}

void hallInterrupt(void) {

    int rotation_time = micros()/100 - last_time;
    last_time = micros()/100;

    write(fifo_fd,&rotation_time,sizeof(int));

    pthread_mutex_lock(&current_step_mutex);
    current_step = 0;
    pthread_mutex_unlock(&current_step_mutex);

    //move to next cell in tab
    running_average_index = (running_average_index + 1) % running_average_amount;
    //subtract old partial sum
    running_average_sum -= running_average_tab[running_average_index];
    //add current to partial sum
    running_average_tab[running_average_index] = rotation_time ;
    running_average_sum += running_average_tab[running_average_index];


    uint64_t running_rotation_time = running_average_sum/running_average_amount;

    int alarm_time_repeat = running_rotation_time*100 / COLUMNS_COUNT;
    ualarm(1, alarm_time_repeat);
    return;
}

void timer_handler(int sig_num) {

    if (shutting_down)return;

    pthread_mutex_lock(&current_step_mutex);
    int current_step_local = current_step;
    current_step++;
    pthread_mutex_unlock(&current_step_mutex);

    if (current_step_local >= COLUMNS_COUNT) {
        return;
    }

    //to add rotation effect
    current_step_local = (current_step_local + frame->offset) % COLUMNS_COUNT;
    pthread_mutex_lock(&spi_mutex);
    bcm2835_spi_writenb(frame->data[current_step_local], PAYLOAD_LENGTH);
    pthread_mutex_unlock(&spi_mutex);
}

void SIGINT_handler(int sig_num) {
    static bool exit_again = false;
    pthread_mutex_unlock(&should_exit);
    if (exit_again)exit(0);
    printf("\nExiting...\n");
    exit_again = true;

}

void prepare_frame(char *frame, int r, int g, int b, int frame_num = 0) {
    //start_frame
    for (int pos = 0; pos < 4; pos++) {
        frame[pos] = 0;
    }

    //led frames
    for (int led = 0; led < LED_COUNT; led++) {
        frame[4 + led * 4] = char(224 + 1);
        if (frame_num != 0 && (frame_num % (LED_COUNT / 2)) == led % (LED_COUNT / 2)) {
            frame[4 + led * 4 + 1] = 255;
            frame[4 + led * 4 + 2] = 255;
            frame[4 + led * 4 + 3] = 255;
        } else {
            frame[4 + led * 4 + 1] = b;
            frame[4 + led * 4 + 2] = g;
            frame[4 + led * 4 + 3] = r;
        }
    }

    //end frame
    for (int i = 0; i < END_PAYLOAD_LENGTH; i++) {
        frame[4 + LED_COUNT * 4 + i] = 255;
    }
}


int main(int argc, char **argv) {
    init_shmem();
    init_fifo();
    if(true) {
        prepare_frame(frame->data[0], 255, 255, 255);
        prepare_frame(frame->data[1], 32, 64, 96);
        prepare_frame(frame->data[2], 255, 255, 0);
        for (int i = 3; i < COLUMNS_COUNT; i++) {
            int r = 0;
            int g = 0;
            int b = 0;
            if (i % 3 == 0) {
                r = 8;
                g = 8;
            }
            if ((i + 1) % 3 == 0) {
                g = 128;
                b = 128;
            }
            if ((i + 2) % 3 == 0) {
                b = 64;
                r = 64;
            }

            prepare_frame(frame->data[i], r, g, b, i);
        }
    }
    printf("Starting...\n");
    srand(time(0));
    pthread_mutex_init(&current_step_mutex, NULL);
    pthread_mutex_init(&spi_mutex, NULL);
    pthread_mutex_init(&should_exit, NULL);
    pthread_mutex_lock(&should_exit);
    piHiPri(2) ;
    init_running_average();
    //signal handlers
    signal(SIGALRM, timer_handler);
    signal(SIGINT, SIGINT_handler);

    //Initiate the SPI Data Frame
    if (!bcm2835_init()) {
        printf("bcm2835_init failed. Are you running as root??\n");
        return 1;
    }
    if (!bcm2835_spi_begin()) {
        printf("bcm2835_spi_begin failed. Are you running as root??\n");
        return 1;
    }
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_16);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

    //display 3 colors for a test
    for (int i = 0; i < 1; i++) {
        bcm2835_spi_writenb(frame->data[0], PAYLOAD_LENGTH);
        bcm2835_delay(250);
        bcm2835_spi_writenb(frame->data[1], PAYLOAD_LENGTH);
        bcm2835_delay(250);
        bcm2835_spi_writenb(frame->data[2], PAYLOAD_LENGTH);
        bcm2835_delay(250);
    }

    //setup hall input and interrupt
    wiringPiSetup();
    pinMode(3, INPUT);
    wiringPiISR(3, INT_EDGE_FALLING, hallInterrupt);
    last_time = millis() - 1000;
    printf("Ready\n");

    // wait for exit signal
    pthread_mutex_lock(&should_exit);

    //exit
    printf("Stopping the timer and interrupts\n");
    shutting_down = true;
    ualarm(0, 0);

    // wait a bit - can't use sleep, because SIG_ALM
    int stop_time = millis();
    while (millis() - stop_time < 1000) {
        //wait
    }

    //turn off LEDs
    printf("turn off LEDs\n");
    pthread_mutex_lock(&spi_mutex);
    prepare_frame(frame->data[0], 0, 0, 0);
    bcm2835_spi_writenb(frame->data[0], PAYLOAD_LENGTH);

    //cleanup - close the spi bus
    bcm2835_spi_end();
    bcm2835_close();
    return 0;
}
