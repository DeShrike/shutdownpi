#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "utils.h"
#include "http.h"

static volatile int keepRunning = 1;

// sudo apt-get install wiringpi
// gcc -lwiringPi
// gcc -lwiringPiDev

// Checkout https://www.youtube.com/watch?v=gymfmJIrc3g

#define delayMilliseconds     delay

#define LEDCOUNT              4

#define STATE_NONE            0
#define STATE_MOVE            1
#define STATE_WAITFORCONFIRM  2
#define STATE_SHUTTINGDOWN    3
#define STATE_SLEEP           4

#define STATE_UP            10
#define STATE_PRESSED       11
#define STATE_RELEASED      12
#define STATE_DOWN          13

#define SHORT_PRESS         0.5
#define LONG_PRESS          3.0
#define WAITING_TIMEOUT     5.0
#define MOVE_TIME           30.0    // Go from STATE_MOVE to STATE_SLEEP after this number of seconds

#define STEPDELAY           40  // Delay between steps

#define INITIALMOVESPEED    10  // 10 * STEPSPEED
#define WAITSPEED           3
#define COUNTDOWNSPEED      2
#define SLEEPSPEED          100

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

    digitalWrite(ledPin1, LOW);
    digitalWrite(ledPin2, LOW);
    digitalWrite(ledPin3, LOW);
    digitalWrite(ledPin4, LOW);

    delayMilliseconds(100);

    pinMode(ledPin1, INPUT);
    pinMode(ledPin2, INPUT);
    pinMode(ledPin3, INPUT);
    pinMode(ledPin4, INPUT);

    delayMilliseconds(100);

    pullUpDnControl(btnPin1, PUD_DOWN);
    pullUpDnControl(btnPin2, PUD_DOWN);
}

void move()
{
    currentLed = advanceLed(currentLed, direction, LEDCOUNT);
    digitalWrite(ledPin1, currentLed == 0 ? HIGH : LOW);
    digitalWrite(ledPin2, currentLed == 1 ? HIGH : LOW);
    digitalWrite(ledPin3, currentLed == 2 ? HIGH : LOW);
    digitalWrite(ledPin4, currentLed == 3 ? HIGH : LOW);
}

void slaap()
{
    currentLed = advanceLed(currentLed, direction, LEDCOUNT);
    digitalWrite(ledPin1, currentLed == 0 ? HIGH : LOW);
    digitalWrite(ledPin2, currentLed == 1 ? HIGH : LOW);
    digitalWrite(ledPin3, currentLed == 2 ? HIGH : LOW);
    digitalWrite(ledPin4, currentLed == 3 ? HIGH : LOW);
    delayMilliseconds(100);
    digitalWrite(ledPin1, LOW);
    digitalWrite(ledPin2, LOW);
    digitalWrite(ledPin3, LOW);
    digitalWrite(ledPin4, LOW);
}

void waitforconfirmation()
{
    currentLed = (currentLed + 1) % 2;
    digitalWrite(ledPin1, LOW);
    digitalWrite(ledPin2, currentLed == 1 ? HIGH : LOW);
    digitalWrite(ledPin3, LOW);
    digitalWrite(ledPin4, LOW);
}

void shuttingdown()
{
    currentLed = (currentLed + 1) % 2;
    digitalWrite(ledPin1, currentLed == 1 ? HIGH : LOW);
    digitalWrite(ledPin2, LOW);
    digitalWrite(ledPin3, LOW);
    digitalWrite(ledPin4, LOW);
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
    if (digitalRead(btnPin1) == HIGH) // Button 1 is not pressed
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
    if (digitalRead(btnPin2) == HIGH) // Button 2 is not pressed
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
}

int main()
{
    signal(SIGKILL, intHandler);
    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);
    signal(SIGHUP, intHandler);

    wiringPiSetupGpio();  // setup wih Broadcom numbering

    pinMode(ledPin1, OUTPUT);
    pinMode(ledPin2, OUTPUT);
    pinMode(ledPin3, OUTPUT);
    pinMode(ledPin4, OUTPUT);

    pinMode(btnPin1, INPUT);
    pinMode(btnPin2, INPUT);
    pullUpDnControl(btnPin1, PUD_UP); // Enable pull-up resistor on button
    pullUpDnControl(btnPin2, PUD_UP); // Enable pull-up resistor on button

    printf("Running! Press Ctrl+C to quit. \n");

    startMoveState();

    while (keepRunning)
    {
        loop();
        delayMilliseconds(STEPDELAY);
        currentStep++;
    }

    printf("\nStopping\n");
    cleanup();

    return 0;
}
