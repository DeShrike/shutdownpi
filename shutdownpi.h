#ifndef SHUTDOWNPI_H
#define SHUTDOWNPI_H

#ifdef __cplusplus
extern "C" {
#endif


#define PRESS_SHORT 	1
#define PRESS_LONG 		2
#define PRESS_HOLD 		3

#define STATE_NONE            0
#define STATE_MOVE            1
#define STATE_WAITFORCONFIRM  2
#define STATE_SHUTTINGDOWN    3
#define STATE_SLEEP           4

#define ACTION_START_RUNNING 	1
#define ACTION_REVERSE 			2
#define ACTION_CANCEL_SHUTDOWN 	3
#define ACTION_START_SHUTDOWN 	4
#define ACTION_GET 				5


#ifdef __cplusplus
}
#endif

#endif /* SHUTDOWNPI_H */
