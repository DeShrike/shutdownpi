#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ACTIONS 20

typedef struct
{
    int button;  // 1-2
    int press_id;
    int state_id;
    int action_id;
    char* action_param;
} buttonconfiguration;

typedef struct
{
    int led1Pin;
    int led2Pin;
    int led3Pin;
    int led4Pin;
    int fanPin;
    int button1Pin;
    int button2Pin;
    buttonconfiguration* buttonconfig[MAX_ACTIONS];
    char* name;
    int fanOn;
    int fanOff;
} configuration;

void free_config(configuration* config);
configuration* read_config(char* filename);
buttonconfiguration** find_actions(configuration* config, int button, int press_id, int state_id);
char* action_name(int action);
char* state_name(int state);
char* press_name(int press);

#define ACTION_START_RUNNING_NAME       "START_RUNNING"
#define ACTION_REVERSE_NAME             "REVERSE"
#define ACTION_CANCEL_SHUTDOWN_NAME     "CANCEL_SHUTDOWN"
#define ACTION_REQUEST_SHUTDOWN_NAME    "REQUEST_SHUTDOWN"
#define ACTION_CONFIRM_SHUTDOWN_NAME    "CONFIRM_SHUTDOWN"
#define ACTION_GET_NAME                 "GET "

#define STATE_IDLE_NAME                 "IDLE"
#define STATE_RUNNING_NAME              "RUNNING"
#define STATE_WAITFORCONFIRM_NAME       "WAITFORCONFIRM"
#define STATE_SHUTTINGDOWN_NAME         "SHUTTINGDOWN"

#define PRESS_SHORT_NAME    "short"
#define PRESS_LONG_NAME     "long"
#define PRESS_HOLD_NAME     "hold"

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
