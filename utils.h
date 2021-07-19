#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

int advanceLed(int currentLed, int direction, int ledCount);
double calcDuration(struct timespec *van, struct timespec *tot);
double ellapsedSince(struct timespec *van);
char* rstrip(char* s);
char* lskip(const char* s);

#ifdef __cplusplus
}
#endif

#endif
