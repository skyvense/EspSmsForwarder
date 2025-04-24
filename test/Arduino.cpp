#include "Arduino.h"
#include <unistd.h>
#include <sys/time.h>

SerialClass Serial;

void delay(unsigned long ms) {
    usleep(ms * 1000);
}

unsigned long millis() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
} 