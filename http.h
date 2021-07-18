#ifndef HTTP_H
#define HTTP_H

#ifdef __cplusplus
extern "C" {
#endif

int httpget(char* hostname, int port, char* request);

#ifdef __cplusplus
}
#endif

#endif
