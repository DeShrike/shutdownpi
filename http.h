#ifndef HTTP_H
#define HTTP_H

#ifdef __cplusplus
extern "C" {
#endif

int http_get(char* hostname, int port, char* request);
int http_get_url(char* url);

#ifdef __cplusplus
}
#endif

#endif
