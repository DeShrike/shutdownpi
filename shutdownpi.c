#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

static volatile int keepRunning = 1;

void intHandler(int dummy) 
{
    keepRunning = 0;
}

// sudo apt-get install wiringpi

// git clone git://git.drogon.net/wiringPi
// cd wiringPi
// git pull origin
// ./build

// gcc -lwiringPi
// gcc -lwiringPiDev

/*
For LED:
gpio -g mode 18 output
gpio -g write 18 1
gpio -g write 18 0

For buttons:
gpio -g mode 17 up
gpio -g read 17
0 = pressed : 1 = released
*/

#define delayMilliseconds     delay
#define BILLION  1000000000.0

#define STATE_NONE            0
#define STATE_RUNNING         1
#define STATE_WAITFORCONFIRM  2
#define STATE_SHUTTINGDOWN    3

#define STATE_UP              10
#define STATE_PRESSED         11
#define STATE_RELEASED        12
#define STATE_DOWN            13

#define SHORT_PRESS  0.5
#define LONG_PRESS   3.0
#define WAITING_TIMEOUT 5.0

int runspeed = 200;   // lower is faster
int waitspeed = 50;   // lower is faster

int currentLed = -1;
int direction = 1;

int state = STATE_RUNNING;
int button_state = STATE_UP;

struct timespec time_pressed;
struct timespec time_released;
struct timespec time_now;
struct timespec time_waiting_start;

const int ledPin1 = 16;  // GPIO16
const int ledPin2 = 20;  // GPIO20
const int ledPin3 = 21;  // GPIO21

const int btnPin = 12;   // GPIO12

void cleanup()
{
   digitalWrite(ledPin1, LOW);
   digitalWrite(ledPin2, LOW);
   digitalWrite(ledPin3, LOW);
}

void advance()
{
   currentLed = currentLed + direction;
   if (currentLed < 0)
   {
      currentLed = 2;
   }

   currentLed = currentLed % 3;
      
   digitalWrite(ledPin1, currentLed == 0 ? HIGH : LOW);
   digitalWrite(ledPin2, currentLed == 1 ? HIGH : LOW);
   digitalWrite(ledPin3, currentLed == 2 ? HIGH : LOW);
   delayMilliseconds(runspeed);
}

void waitforconfirmation()
{
   currentLed = (currentLed + 1) % 2;
   digitalWrite(ledPin1, LOW);
   digitalWrite(ledPin2, currentLed == 1 ? HIGH : LOW);
   digitalWrite(ledPin3, LOW);
   delayMilliseconds(waitspeed);
}

void shuttingdown()
{
   currentLed = (currentLed + 1) % 2;
   digitalWrite(ledPin1, currentLed == 1 ? HIGH : LOW);
   digitalWrite(ledPin2, LOW);
   digitalWrite(ledPin3, LOW);
   delayMilliseconds(waitspeed);
}

void handle_short_push()
{
   printf("Short Press\n");
   if (state == STATE_RUNNING)
   {
      direction *= -1;
/*      runspeed -= 50;
      if (runspeed < 50)
      {
         runspeed = 50;
      }*/
   }
   else if (state == STATE_WAITFORCONFIRM)
   {
      state = STATE_RUNNING;
   }
   else if (state == STATE_SHUTTINGDOWN)
   {
      printf("sudo shutdown -c \n");
      system("sudo shutdown -c");
      state = STATE_RUNNING;
   }
}

void handle_long_push()
{
   printf("Long Press\n");
   if (state == STATE_RUNNING)
   {
      //runspeed += 50;
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
      state = STATE_RUNNING;
   }
}

int main()
{
   signal(SIGKILL, intHandler);
   signal(SIGINT, intHandler);
   signal(SIGTERM, intHandler);
   wiringPiSetupGpio();  // setup wih Broadcom numbering

   pinMode(ledPin1, OUTPUT);
   pinMode(ledPin2, OUTPUT);
   pinMode(ledPin3, OUTPUT);

   pinMode(btnPin, INPUT);

   // pullUpDnControl(btnPin, PUD_UP); // Enable pull-up resistor on button

   printf("Running! Press Ctrl+C to quit. \n");

   while (keepRunning)
   {
      if (state == STATE_RUNNING)
      {
         advance();
      }
      else if (state == STATE_WAITFORCONFIRM)
      {
         waitforconfirmation();
         clock_gettime(CLOCK_REALTIME, &time_now);
         double duration = (time_now.tv_sec - time_waiting_start.tv_sec) +
                           (time_now.tv_nsec - time_waiting_start.tv_nsec) / BILLION;
         if (duration > WAITING_TIMEOUT)
         {
            state = STATE_RUNNING;
         }
      }
      else if (state == STATE_SHUTTINGDOWN)
      {
         shuttingdown();
      }

      if (digitalRead(btnPin) == HIGH) // Button is not pressed
      {
         if (button_state == STATE_DOWN)
         {
            // printf("Released\n");
            button_state = STATE_RELEASED;
            clock_gettime(CLOCK_REALTIME, &time_released);

            double duration = (time_released.tv_sec - time_pressed.tv_sec) +
                              (time_released.tv_nsec - time_pressed.tv_nsec) / BILLION;
 
            // printf("Duration: %.6f seconds\n", duration);
            if (duration < SHORT_PRESS)
            {
               handle_short_push();
            }
            else if (duration > LONG_PRESS)
            {
            }
            else
            {
               handle_long_push();
            }

            button_state = STATE_UP;
         }
         else if (button_state != STATE_UP)
         {
            printf("Strange: Button State is %d\n", button_state);
            button_state = STATE_UP;
         }
      }
      else // Button is pressed
      {
         if (button_state == STATE_PRESSED)
         {
            button_state = STATE_DOWN;
         }
         else if (button_state == STATE_UP)
         {
            // printf("Pressed\n");
            button_state = STATE_PRESSED;
            clock_gettime(CLOCK_REALTIME, &time_pressed);
         }
         else if (button_state == STATE_DOWN)
         {
            clock_gettime(CLOCK_REALTIME, &time_now);
            double duration = (time_now.tv_sec - time_pressed.tv_sec) +
                              (time_now.tv_nsec - time_pressed.tv_nsec) / BILLION;
            if (duration > LONG_PRESS)
            {
               if (state == STATE_RUNNING)
               {
                  clock_gettime(CLOCK_REALTIME, &time_waiting_start);
                  state = STATE_WAITFORCONFIRM;
               }
            }
         }
         else
         {
            printf("Hmm, button_state = %d\n", button_state);
         }
      }

      delayMilliseconds(10);
   }

   printf("\nStopping\n");
   cleanup();

   return 0;
}
