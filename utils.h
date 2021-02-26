#ifndef UTILS_H
#define UTILS_H

int advanceLed(int currentLed, int direction, int ledCount);
double calcDuration(struct timespec *van, struct timespec *tot);
double ellapsedSince(struct timespec *van);

#endif
