#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "shutdownpi.h"
#include "utils.h"
#include "config.h"
#include "http.h"

static volatile int keepRunning = 1;

// sudo apt-get install wiringpi
// gcc -lwiringPi
// gcc -lwiringPiDev

// Checkout https://www.youtube.com/watch?v=gymfmJIrc3g

#define CONFIG_FILE           "shutdownpi.ini"

#define delayMilliseconds     delay

#define LEDCOUNT              4

#define STATE_UP            10
#define STATE_PRESSED       11
#define STATE_RELEASED      12
#define STATE_DOWN          13

#define SHORT_PRESS         0.5
#define LONG_PRESS          3.0
#define WAITING_TIMEOUT     5.0
#define MOVE_TIME           30.0    // Go from STATE_MOVE to STATE_SLEEP after this number of seconds

#define STEPDELAY           50  // Delay between steps

#define INITIALMOVESPEED    10  // 10 * STEPSPEED
#define WAITSPEED           3
#define COUNTDOWNSPEED      2
#define SLEEPSPEED          100

#define PINMODE(p, m)           if (p != 0) { pinMode(p, m); }
#define PULLUPDNCONTROL(p, m)   if (p != 0) { pullUpDnControl(p, m); }
#define DIGITALWRITE(p, m)      if (p != 0) { digitalWrite(p, m); }
#define DIGITALREAD(p)          (p != 0 ? digitalRead(p) : 0)

int currentStep = 0;
int moveSpeed = INITIALMOVESPEED;
int currentLed = -1;
int direction = 1;

int state = STATE_MOVE;
int button_1_state = STATE_UP;
int button_2_state = STATE_UP;

struct timespec time_pressed_1;
struct timespec time_released_1;
struct timespec time_pressed_2;
struct timespec time_released_2;

struct timespec time_waiting_start;
struct timespec time_move_start;

const int ledPin1 = 16;  // GPIO16
const int ledPin2 = 20;  // GPIO20
const int ledPin3 = 21;  // GPIO21
const int ledPin4 = 7;  // GPIO7

const int btnPin1 = 13;   // GPIO13   was GPIO12
const int btnPin2 = 26;   // GPIO26

void intHandler(int dummy)
{
    keepRunning = 0;
}

void cleanup()
{
    printf("Cleanup\n");

    DIGITALWRITE(ledPin1, LOW);
    DIGITALWRITE(ledPin2, LOW);
    DIGITALWRITE(ledPin3, LOW);
    DIGITALWRITE(ledPin4, LOW);

    delayMilliseconds(100);

    PINMODE(ledPin1, INPUT);
    PINMODE(ledPin2, INPUT);
    PINMODE(ledPin3, INPUT);
    PINMODE(ledPin4, INPUT);

    delayMilliseconds(100);

    PULLUPDNCONTROL(btnPin1, PUD_DOWN);
    PULLUPDNCONTROL(btnPin2, PUD_DOWN);
}

void move()
{
    currentLed = advanceLed(currentLed, direction, LEDCOUNT);
    DIGITALWRITE(ledPin1, currentLed == 0 ? HIGH : LOW);
    DIGITALWRITE(ledPin2, currentLed == 1 ? HIGH : LOW);
    DIGITALWRITE(ledPin3, currentLed == 2 ? HIGH : LOW);
    DIGITALWRITE(ledPin4, currentLed == 3 ? HIGH : LOW);
}

void slaap()
{
    currentLed = advanceLed(currentLed, direction, LEDCOUNT);
    DIGITALWRITE(ledPin1, currentLed == 0 ? HIGH : LOW);
    DIGITALWRITE(ledPin2, currentLed == 1 ? HIGH : LOW);
    DIGITALWRITE(ledPin3, currentLed == 2 ? HIGH : LOW);
    DIGITALWRITE(ledPin4, currentLed == 3 ? HIGH : LOW);
    delayMilliseconds(100);
    DIGITALWRITE(ledPin1, LOW);
    DIGITALWRITE(ledPin2, LOW);
    DIGITALWRITE(ledPin3, LOW);
    DIGITALWRITE(ledPin4, LOW);
}

void waitforconfirmation()
{
    currentLed = (currentLed + 1) % 2;
    DIGITALWRITE(ledPin1, LOW);
    DIGITALWRITE(ledPin2, currentLed == 1 ? HIGH : LOW);
    DIGITALWRITE(ledPin3, LOW);
    DIGITALWRITE(ledPin4, LOW);
}

void shuttingdown()
{
    currentLed = (currentLed + 1) % 2;
    DIGITALWRITE(ledPin1, currentLed == 1 ? HIGH : LOW);
    DIGITALWRITE(ledPin2, LOW);
    DIGITALWRITE(ledPin3, LOW);
    DIGITALWRITE(ledPin4, LOW);
}

void startMoveState()
{
    state = STATE_MOVE;
    clock_gettime(CLOCK_REALTIME, &time_move_start);
    moveSpeed = INITIALMOVESPEED;
}

void handle_short_push_1()
{
    printf("Button 1: Short Press\n");
    if (state == STATE_SLEEP)
    {
        startMoveState();
    }
    else if (state == STATE_MOVE)
    {
        direction *= -1;
    }
    else if (state == STATE_WAITFORCONFIRM)
    {
        startMoveState();
    }
    else if (state == STATE_SHUTTINGDOWN)
    {
        printf("sudo shutdown -c \n");
        system("sudo shutdown -c");
        startMoveState();
    }
}

void handle_long_push_1()
{
    printf("Button 1: Long Press\n");
    if (state == STATE_SLEEP)
    {
        startMoveState();
    }
    else if (state == STATE_MOVE)
    {
        direction *= -1;
    }
    else if (state == STATE_WAITFORCONFIRM)
    {
        state = STATE_SHUTTINGDOWN;
        printf("sudo shutdown +1 -hP \n");
        system("sudo shutdown +1 -hP");
    }
    else if (state == STATE_SHUTTINGDOWN)
    {
        printf("sudo shutdown -c \n");
        system("sudo shutdown -c");
        startMoveState();
    }
}

void handle_super_long_push_1()
{
    printf("Button 1: Super Long Press\n");
    if (state == STATE_MOVE)
    {
        clock_gettime(CLOCK_REALTIME, &time_waiting_start);
        state = STATE_WAITFORCONFIRM;
    }
}

void handle_short_push_2()
{
    printf("Button 2: Short Press\n");
}

void handle_long_push_2()
{
    printf("Button 2: Long Press\n");
}

void handle_super_long_push_2()
{
    printf("Button 2: Super Long Press\n");
}

void check_button_1()
{
    ////////////////////////////////////////////////////////////////////////////////
    // Button 1 ////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    if (DIGITALREAD(btnPin1) == HIGH) // Button 1 is not pressed
    {
        if (button_1_state == STATE_DOWN)
        {
            button_1_state = STATE_RELEASED;
            clock_gettime(CLOCK_REALTIME, &time_released_1);
            double duration = calcDuration(&time_pressed_1, &time_released_1);
            if (duration < SHORT_PRESS)
            {
                handle_short_push_1();
            }
            else if (duration > LONG_PRESS)
            {
            }
            else
            {
                handle_long_push_1();
            }

            button_1_state = STATE_UP;
        }
        else if (button_1_state != STATE_UP)
        {
            printf("Strange: Button 1 State is %d\n", button_1_state);
            button_1_state = STATE_UP;
        }
    }
    else // Button 1 is pressed
    {
        if (button_1_state == STATE_PRESSED)
        {
            button_1_state = STATE_DOWN;
        }
        else if (button_1_state == STATE_UP)
        {
            button_1_state = STATE_PRESSED;
            clock_gettime(CLOCK_REALTIME, &time_pressed_1);
        }
        else if (button_1_state == STATE_DOWN)
        {
            double duration = ellapsedSince(&time_pressed_1);
            if (duration > LONG_PRESS)
            {
                handle_super_long_push_1();
            }
        }
        else
        {
            printf("Hmm, Button 1 State = %d\n", button_1_state);
        }
    }
}

void check_button_2()
{
    ////////////////////////////////////////////////////////////////////////////////
    // Button 2 ////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    if (DIGITALREAD(btnPin2) == HIGH) // Button 2 is not pressed
    {
        if (button_2_state == STATE_DOWN)
        {
            button_2_state = STATE_RELEASED;
            clock_gettime(CLOCK_REALTIME, &time_released_2);
            double duration = calcDuration(&time_pressed_2, &time_released_2);
            if (duration < SHORT_PRESS)
            {
                handle_short_push_2();
            }
            else if (duration > LONG_PRESS)
            {
            }
            else
            {
                handle_long_push_2();
            }

            button_2_state = STATE_UP;
        }
        else if (button_2_state != STATE_UP)
        {
            printf("Strange: Button 2 State is %d\n", button_2_state);
            button_2_state = STATE_UP;
        }
    }
    else // Button 2 is pressed
    {
        if (button_2_state == STATE_PRESSED)
        {
            button_2_state = STATE_DOWN;
        }
        else if (button_2_state == STATE_UP)
        {
            button_2_state = STATE_PRESSED;
            clock_gettime(CLOCK_REALTIME, &time_pressed_2);
        }
        else if (button_2_state == STATE_DOWN)
        {
            double duration = ellapsedSince(&time_pressed_2);
            if (duration > LONG_PRESS)
            {
                handle_super_long_push_2();
            }
        }
        else
        {
            printf("Hmm, Button 2 State = %d\n", button_2_state);
        }
    }
}

void loop()
{
    if (state == STATE_MOVE)
    {
        if (currentStep % moveSpeed == 0)
        {
            move();
            double duration = ellapsedSince(&time_move_start);
            if (duration > MOVE_TIME)
            {
                if (moveSpeed <= 2)
                {
                    state = STATE_SLEEP;
                }
                else
                {
                    moveSpeed--;
                    clock_gettime(CLOCK_REALTIME, &time_move_start);
                }
            }
        }
    }
    else if (state == STATE_SLEEP)
    {
        if (currentStep % SLEEPSPEED == 0)
        {
            slaap();

            time_t rawtime;
            struct tm * timeinfo;

            time(&rawtime);
            timeinfo = localtime(&rawtime);
            if (timeinfo->tm_min == 0)
            {
                printf("[%d %d %d %d:%d:%d]\n", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
                startMoveState();
            }
        }
    }
    else if (state == STATE_WAITFORCONFIRM)
    {
        if (currentStep % WAITSPEED == 0)
        {
            waitforconfirmation();
            double duration = ellapsedSince(&time_waiting_start);
            if (duration > WAITING_TIMEOUT)
            {
                startMoveState();
            }
        }
    }
    else if (state == STATE_SHUTTINGDOWN)
    {
        if (currentStep % COUNTDOWNSPEED == 0)
        {
            shuttingdown();
        }
    }

    check_button_1();
    check_button_2();

    delayMilliseconds(STEPDELAY);
    currentStep++;
}

int start(configuration* config)
{
    signal(SIGKILL, intHandler);
    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);
    signal(SIGHUP, intHandler);

    wiringPiSetupGpio();  // setup wih Broadcom numbering

    PINMODE(ledPin1, OUTPUT);
    PINMODE(ledPin2, OUTPUT);
    PINMODE(ledPin3, OUTPUT);
    PINMODE(ledPin4, OUTPUT);

    PINMODE(btnPin1, INPUT);
    PINMODE(btnPin2, INPUT);
    PULLUPDNCONTROL(btnPin1, PUD_UP); // Enable pull-up resistor on button
    PULLUPDNCONTROL(btnPin2, PUD_UP); // Enable pull-up resistor on button

    printf("Running! Press Ctrl+C to quit. \n");

    startMoveState();

    while (keepRunning)
    {
        loop();
    }

    printf("\nStopping\n");
    cleanup();

    return 0;
}

int main(int argc, char* argv[])
{
    configuration* config;
    config = read_config(CONFIG_FILE);
    if (config == NULL)
    {
        printf("Can't load '%s'\n", CONFIG_FILE);
        return 1;
    }

    printf("Config loaded from '%s':\n", CONFIG_FILE);

    printf("     led1: %d \n     led2: %d \n     led3: %d \n     led4: %d \n  button1: %d \n  button2: %d \n      fan: %d \n",
        config->led1Pin, config->led2Pin, config->led3Pin, config->led4Pin,
        config->button1Pin, config->button2Pin,
        config->fanPin);

    printf("\nName: %s\n", config->name);

    int ret = 0; // start(config);

    free_config(config);

    return ret;
}
