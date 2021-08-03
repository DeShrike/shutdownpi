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

#define CONFIG_FILE         "/etc/shutdownpi.ini"

#define delayMilliseconds   delay

#define LEDCOUNT            4

#define STATE_UP            10
#define STATE_PRESSED       11
#define STATE_RELEASED      12
#define STATE_DOWN          13

#define SHORT_PRESS         0.5
#define LONG_PRESS          3.0
#define WAITING_TIMEOUT     5.0
#define MOVE_TIME           30.0    // Go from STATE_MOVE to STATE_SLEEP after this number of seconds
#define INTERACTIVE_TIME    60.0    // Go from STATE_INTERACTIVE to STATE_MOVE after this number of seconds

#define STEPDELAY           50      // Delay between steps

#define INITIALMOVESPEED    10      // 10 * STEPSPEED
#define WAITSPEED           3
#define COUNTDOWNSPEED      2
#define SLEEPSPEED          100

#define PINMODE(p, m)           if (p != 0) { pinMode(p, m); }
#define PULLUPDNCONTROL(p, m)   if (p != 0) { pullUpDnControl(p, m); }
#define DIGITALWRITE(p, m)      if (p != 0) { digitalWrite(p, m); }
#define DIGITALREAD(p)          (p != 0 ? digitalRead(p) : 0)

typedef struct
{
    int id;
    int state;
    int pin;
    struct timespec time_pressed;
    struct timespec time_released;
} buttondata;

configuration* config;
buttondata button_1_data;
buttondata button_2_data;

int currentStep = 0;
int moveSpeed = INITIALMOVESPEED;
int currentLed = -1;
int direction = 1;

int state = STATE_MOVE;

struct timespec time_waiting_start;
struct timespec time_move_start;
struct timespec time_interactive_start;

void intHandler(int dummy)
{
    keepRunning = 0;
}

void cleanup()
{
    printf("Cleanup\n");

    DIGITALWRITE(config->led1Pin, LOW);
    DIGITALWRITE(config->led2Pin, LOW);
    DIGITALWRITE(config->led3Pin, LOW);
    DIGITALWRITE(config->led4Pin, LOW);

    delayMilliseconds(100);

    PINMODE(config->led1Pin, INPUT);
    PINMODE(config->led2Pin, INPUT);
    PINMODE(config->led3Pin, INPUT);
    PINMODE(config->led4Pin, INPUT);

    delayMilliseconds(100);

    PULLUPDNCONTROL(config->button1Pin, PUD_DOWN);
    PULLUPDNCONTROL(config->button2Pin, PUD_DOWN);
}

void move_leds()
{
    currentLed = advanceLed(currentLed, direction, LEDCOUNT);
    DIGITALWRITE(config->led1Pin, currentLed == 0 ? HIGH : LOW);
    DIGITALWRITE(config->led2Pin, currentLed == 1 ? HIGH : LOW);
    DIGITALWRITE(config->led3Pin, currentLed == 2 ? HIGH : LOW);
    DIGITALWRITE(config->led4Pin, currentLed == 3 ? HIGH : LOW);
}

void interactive_leds()
{
    currentLed = currentLed == 1 ? 3 : 1;
    DIGITALWRITE(config->led1Pin, currentLed == 0 ? HIGH : LOW);
    DIGITALWRITE(config->led2Pin, currentLed == 1 ? HIGH : LOW);
    DIGITALWRITE(config->led3Pin, currentLed == 2 ? HIGH : LOW);
    DIGITALWRITE(config->led4Pin, currentLed == 3 ? HIGH : LOW);
}

void slaap_leds()
{
    currentLed = advanceLed(currentLed, direction, LEDCOUNT);
    DIGITALWRITE(config->led1Pin, currentLed == 0 ? HIGH : LOW);
    DIGITALWRITE(config->led2Pin, currentLed == 1 ? HIGH : LOW);
    DIGITALWRITE(config->led3Pin, currentLed == 2 ? HIGH : LOW);
    DIGITALWRITE(config->led4Pin, currentLed == 3 ? HIGH : LOW);

    delayMilliseconds(100);

    DIGITALWRITE(config->led1Pin, LOW);
    DIGITALWRITE(config->led2Pin, LOW);
    DIGITALWRITE(config->led3Pin, LOW);
    DIGITALWRITE(config->led4Pin, LOW);
}

void waitforconfirmation_leds()
{
    currentLed = (currentLed + 1) % 2;
    DIGITALWRITE(config->led1Pin, LOW);
    DIGITALWRITE(config->led2Pin, currentLed == 1 ? HIGH : LOW);
    DIGITALWRITE(config->led3Pin, LOW);
    DIGITALWRITE(config->led4Pin, LOW);
}

void shuttingdown_leds()
{
    currentLed = (currentLed + 1) % 2;
    DIGITALWRITE(config->led1Pin, currentLed == 1 ? HIGH : LOW);
    DIGITALWRITE(config->led2Pin, LOW);
    DIGITALWRITE(config->led3Pin, LOW);
    DIGITALWRITE(config->led4Pin, LOW);
}

void startMoveState()
{
    state = STATE_MOVE;
    clock_gettime(CLOCK_REALTIME, &time_move_start);
    moveSpeed = INITIALMOVESPEED;
}

void startInteractiveState()
{
    state = STATE_INTERACTIVE;
    clock_gettime(CLOCK_REALTIME, &time_interactive_start);
}

void do_actions(int button, int press_id, int state_id)
{
    buttonconfiguration** actions;
    actions = find_actions(config, button, press_id, state_id);
    buttonconfiguration* action = *actions;

    while (action != NULL)
    {
        printf("Action %p\n", (void*)actions);
        printf("  Button: %d\n", action->button);
        printf("   Press: %d - %s\n", action->press_id, press_name(action->press_id));
        printf("   State: %d - %s\n", action->state_id, state_name(action->state_id));
        printf("  Action: %d - %s\n", action->action_id, action_name(action->action_id));

        if (action->action_param != NULL)
        {
            printf("   Param: %s\n", action->action_param);
        }

        switch (action->action_id)
        {
            case ACTION_START_RUNNING:
                startMoveState();
                break;
            case ACTION_START_INTERACTIVE:
                startInteractiveState();
                break;
            case ACTION_CANCEL_SHUTDOWN:
                printf("sudo shutdown -c \n");
                system("sudo shutdown -c");
                startMoveState();
                break;
            case ACTION_REQUEST_SHUTDOWN:
                clock_gettime(CLOCK_REALTIME, &time_waiting_start);
                state = STATE_WAITFORCONFIRM;
                break;
            case ACTION_CONFIRM_SHUTDOWN:
                state = STATE_SHUTTINGDOWN;
                printf("sudo shutdown +1 -hP \n");
                system("sudo shutdown +1 -hP");
                break;
            case ACTION_REVERSE:
                direction *= -1;
                break;
            case ACTION_GET:
                if (action->action_param != NULL)
                {
                    http_get_url(action->action_param);
                }
                else
                {
                    printf("Missing parameter for action GET\n");
                }

                break;
        }

        action = *(++actions);
    }

    if (state_id == STATE_INTERACTIVE)
    {
        startInteractiveState();
    }

    printf("\n");
}

void handle_button_press(buttondata* data, int press_id)
{
    printf("Button %d: Press %s - %d\n", data->id, press_name(press_id), press_id);
    do_actions(data->id, press_id, state);
}

void check_button(buttondata* data)
{
    // printf("Check button %d - Pin %d (%d)\n", data->id, data->pin, data->state);
    if (DIGITALREAD(data->pin) == HIGH)
    {
        if (data->state == STATE_DOWN)
        {
            data->state = STATE_RELEASED;
            clock_gettime(CLOCK_REALTIME, &data->time_released);
            double duration = calcDuration(&data->time_pressed, &data->time_released);
            if (duration < SHORT_PRESS)
            {
                handle_button_press(data, PRESS_SHORT);
            }
            else if (duration > LONG_PRESS)
            {
            }
            else
            {
                handle_button_press(data, PRESS_LONG);
            }

            data->state = STATE_UP;
        }
        else if (data->state != STATE_UP)
        {
            // printf("Strange: Button %d State is %d\n", data->id, data->state);
            data->state = STATE_UP;
        }
    }
    else // Button is pressed
    {
        // printf("Pressed %d\n", data->id);
        if (data->state == STATE_PRESSED)
        {
            data->state = STATE_DOWN;
        }
        else if (data->state == STATE_UP)
        {
            data->state = STATE_PRESSED;
            clock_gettime(CLOCK_REALTIME, &data->time_pressed);
        }
        else if (data->state == STATE_DOWN)
        {
            double duration = ellapsedSince(&data->time_pressed);
            if (duration > LONG_PRESS)
            {
                clock_gettime(CLOCK_REALTIME, &data->time_pressed);
                data->state = STATE_RELEASED;
                handle_button_press(data, PRESS_HOLD);
            }
        }
        else
        {
            // NOOP
            // printf("Hmm, Button %d State = %d\n", data->id, data->state);
        }
    }
}

void loop()
{
    if (state == STATE_INTERACTIVE)
    {
        if (currentStep % WAITSPEED == 0)
        {
            interactive_leds();
            double duration = ellapsedSince(&time_interactive_start);
            if (duration > INTERACTIVE_TIME)
            {
                startMoveState();
            }
        }
    }
    else if (state == STATE_MOVE)
    {
        if (currentStep % moveSpeed == 0)
        {
            move_leds();
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
            slaap_leds();

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
            waitforconfirmation_leds();
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
            shuttingdown_leds();
        }
    }

    check_button(&button_1_data);
    check_button(&button_2_data);

    delayMilliseconds(STEPDELAY);
    currentStep++;
}

int start()
{
    signal(SIGKILL, intHandler);
    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);
    signal(SIGHUP, intHandler);

    wiringPiSetupGpio();  // setup wih Broadcom numbering

    PINMODE(config->led1Pin, OUTPUT);
    PINMODE(config->led2Pin, OUTPUT);
    PINMODE(config->led3Pin, OUTPUT);
    PINMODE(config->led4Pin, OUTPUT);

    PINMODE(config->button1Pin, INPUT);
    PINMODE(config->button2Pin, INPUT);
    PULLUPDNCONTROL(config->button1Pin, PUD_UP); // Enable pull-up resistor on button
    PULLUPDNCONTROL(config->button2Pin, PUD_UP); // Enable pull-up resistor on button

    printf("Running! Press Ctrl+C to quit. \n");

    startMoveState();

    button_1_data.id = 1;
    button_1_data.pin = config->button1Pin;
    button_1_data.state = STATE_UP;

    button_2_data.id = 2;
    button_2_data.pin = config->button2Pin;
    button_2_data.state = STATE_UP;

    while (keepRunning)
    {
        loop();
    }

    printf("\nStopping\n");
    cleanup();

    return 0;
}

void show_config(configuration* config)
{
    printf("Config loaded from '%s':\n", CONFIG_FILE);

    printf("     led1: %d \n     led2: %d \n     led3: %d \n     led4: %d \n  button1: %d \n  button2: %d \n",
        config->led1Pin, config->led2Pin, config->led3Pin, config->led4Pin,
        config->button1Pin, config->button2Pin);

    printf("\nName: %s\n", config->name);
}

int main(int argc, char* argv[])
{
    config = read_config(CONFIG_FILE);
    if (config == NULL)
    {
        fprintf(stderr, "Can't load '%s'\n", CONFIG_FILE);
        return EXIT_FAILURE;
    }

    // show_config(config);

    start();

    free_config(config);

    return EXIT_SUCCESS;
}
