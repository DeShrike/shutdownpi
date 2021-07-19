#include <time.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"

#define BILLION  1000000000.0

int advanceLed(int currentLed, int direction, int ledCount)
{
    currentLed = currentLed + direction;
    if (currentLed < 0)
    {
        currentLed = ledCount - 1;
    }

    return currentLed % ledCount;
}

double calcDuration(struct timespec *van, struct timespec *tot)
{
    return (tot->tv_sec - van->tv_sec) +
           (tot->tv_nsec - van->tv_nsec) / BILLION;
}

double ellapsedSince(struct timespec *van)
{
	struct timespec time_now;
    clock_gettime(CLOCK_REALTIME, &time_now);
    return (time_now.tv_sec - van->tv_sec) +
           (time_now.tv_nsec - van->tv_nsec) / BILLION;
}

/* Strip whitespace chars off end of given string, in place. Return s. */
char* rstrip(char* s)
{
    char* p = s + strlen(s);
    while (p > s && isspace((unsigned char)(*--p)))
    {
        *p = '\0';
    }

    return s;
}

/* Return pointer to first non-whitespace char in given string. */
char* lskip(const char* s)
{
    while (*s && isspace((unsigned char)(*s)))
    {
        s++;
    }

    return (char*)s;
}

