#ifndef SHUTDOWNPI_H
#define SHUTDOWNPI_H

#ifdef __cplusplus
extern "C" {
#endif


#define PRESS_SHORT 	        1
#define PRESS_LONG 		        2
#define PRESS_HOLD 		        3

#define STATE_NONE              0
#define STATE_MOVE              1
#define STATE_WAITFORCONFIRM    2
#define STATE_SHUTTINGDOWN      3
#define STATE_SLEEP             4
#define STATE_INTERACTIVE       5

#define ACTION_START_RUNNING        1
#define ACTION_REVERSE 			    2
#define ACTION_CANCEL_SHUTDOWN 	    3
#define ACTION_REQUEST_SHUTDOWN	    4
#define ACTION_CONFIRM_SHUTDOWN	    5
#define ACTION_GET 				    6
#define ACTION_START_INTERACTIVE    7

#define PRESS_SHORT_NAME                "short"
#define PRESS_LONG_NAME                 "long"
#define PRESS_HOLD_NAME                 "hold"

#define STATE_NONE_NAME                 "NONE"
#define STATE_IDLE_NAME                 "IDLE"
#define STATE_RUNNING_NAME              "RUNNING"
#define STATE_WAITFORCONFIRM_NAME       "WAITFORCONFIRM"
#define STATE_SHUTTINGDOWN_NAME         "SHUTTINGDOWN"
#define STATE_INTERACTIVE_NAME          "INTERACTIVE"

#define ACTION_START_RUNNING_NAME       "START_RUNNING"
#define ACTION_REVERSE_NAME             "REVERSE"
#define ACTION_CANCEL_SHUTDOWN_NAME     "CANCEL_SHUTDOWN"
#define ACTION_REQUEST_SHUTDOWN_NAME    "REQUEST_SHUTDOWN"
#define ACTION_CONFIRM_SHUTDOWN_NAME    "CONFIRM_SHUTDOWN"
#define ACTION_GET_NAME                 "GET "
#define ACTION_START_INTERACTIVE_NAME   "START_INTERACTIVE"


#ifdef __cplusplus
}
#endif

#endif /* SHUTDOWNPI_H */
