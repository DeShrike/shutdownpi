#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ACTIONS 50

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

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
