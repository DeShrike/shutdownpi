#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "ini.h"
#include "config.h"
#include "shutdownpi.h"

char* presses[] = { PRESS_SHORT_NAME, PRESS_LONG_NAME, PRESS_HOLD_NAME };
int press_id[] = { PRESS_SHORT, PRESS_LONG, PRESS_HOLD };

char* states[] = { STATE_IDLE_NAME, STATE_RUNNING_NAME, STATE_WAITFORCONFIRM_NAME, STATE_SHUTTINGDOWN_NAME };
int state_id[] = { STATE_SLEEP, STATE_MOVE, STATE_WAITFORCONFIRM, STATE_SHUTTINGDOWN};

char* actions[] = { ACTION_START_RUNNING_NAME, ACTION_REVERSE_NAME, ACTION_CANCEL_SHUTDOWN_NAME, ACTION_REQUEST_SHUTDOWN_NAME, ACTION_CONFIRM_SHUTDOWN_NAME, ACTION_GET_NAME};
int action_id[] = { ACTION_START_RUNNING, ACTION_REVERSE, ACTION_CANCEL_SHUTDOWN, ACTION_REQUEST_SHUTDOWN, ACTION_CONFIRM_SHUTDOWN, ACTION_GET};

int num_presses = sizeof(presses) / sizeof(char*);
int num_states = sizeof(states) / sizeof(char*);
int num_actions = sizeof(actions) / sizeof(char*);

char* press_name(int press)
{
    for (int i = 0; i < num_presses; i++)
    {
        if (press_id[i] == press)
        {
            return presses[i];
        }
    }

    return NULL;
}

char* state_name(int state)
{
    for (int i = 0; i < num_states; i++)
    {
        if (state_id[i] == state)
        {
            return states[i];
        }
    }

    return NULL;
}

char* action_name(int action)
{
    for (int i = 0; i < num_actions; i++)
    {
        if (action_id[i] == action)
        {
            return actions[i];
        }
    }

    return NULL;
}

void free_button_config(buttonconfiguration** config)
{
    for (int i = 0; i < MAX_ACTIONS; i++)
    {
        buttonconfiguration* c = config[i];
        if (c != NULL)
        {
            if (c->action_param != NULL)
            {
                // printf("Free Action Param %p\n", (void*)c->action_param);
                free(c->action_param);
            }

            // printf("Free %p\n", (void*)c);
            free(c);
        }
    }
}

void free_config(configuration* config)
{
    if (config->name != NULL)
    {
        // printf("Free Name: %p\n", (void*)config->name);
        free(config->name);
    }

    free_button_config(config->buttonconfig);

    // printf("Free Config: %p\n", (void*)config);
    free(config);
}

void try_add_action(configuration* pconfig, int button, int press_id, int state_id, char* value)
{
    char* temp = lskip(value);
    rstrip(temp);
    int found = 0;

    // printf("Found %d | %d | %d = '%s'\n", button, press_id, state_id, temp);
    for (int a = 0; a < num_actions; a++)
    {
        if (strncmp(temp, actions[a], strlen(actions[a])) == 0)
        {
            found = 1;
            // printf("Action = %d - %s\n", action_id[a], actions[a]);
            int ix = -1;
            buttonconfiguration* bc;
            do
            {
                ix++;
                bc = pconfig->buttonconfig[ix];
            } while (bc != NULL && ix < MAX_ACTIONS - 1);

            if (bc != NULL)
            {
                printf("Too many actions\n");
                return;
            }

            pconfig->buttonconfig[ix] = (buttonconfiguration*)malloc(sizeof(buttonconfiguration));
            // printf("Malloc %p  %d bytes\n", (void*)pconfig->buttonconfig[ix], sizeof(buttonconfiguration));

            pconfig->buttonconfig[ix]->button = button;
            pconfig->buttonconfig[ix]->press_id = press_id;
            pconfig->buttonconfig[ix]->state_id = state_id;
            pconfig->buttonconfig[ix]->action_id = action_id[a];
            pconfig->buttonconfig[ix]->action_param = NULL;
            if (strlen(temp) > strlen(actions[a]))
            {
                pconfig->buttonconfig[ix]->action_param = strdup(temp + strlen(actions[a]));
                // printf("strdup() = %p\n", (void*)pconfig->buttonconfig[ix]->action_param);
            }

            return;
        }
    }

    if (found == 0)
    {
        printf("Unknown action: %s\n", temp);
    }
}

int handler(void* config, const char* section, const char* name,
                   const char* value)
{
    configuration* pconfig = (configuration*)config;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MATCH("pins", "led1"))
    {
        pconfig->led1Pin = atoi(value);
    }
    else if (MATCH("pins", "led2"))
    {
        pconfig->led2Pin = atoi(value);
    }
    else if (MATCH("pins", "led3"))
    {
        pconfig->led3Pin = atoi(value);
    }
    else if (MATCH("pins", "led4"))
    {
        pconfig->led4Pin = atoi(value);
    }
    else if (MATCH("pins", "fan"))
    {
        pconfig->fanPin = atoi(value);
    }
    else if (MATCH("pins", "button1"))
    {
        pconfig->button1Pin = atoi(value);
    }
    else if (MATCH("pins", "button2"))
    {
        pconfig->button2Pin = atoi(value);
    }
    else if (MATCH("fan", "on"))
    {
        pconfig->fanOn = atoi(value);
    }
    else if (MATCH("fan", "off"))
    {
        pconfig->fanOff = atoi(value);
    }
    else if (MATCH("user", "name"))
    {
        pconfig->name = strdup(value);
        // printf("strdup() name: %p\n", (void*)pconfig->name);
    }
    else
    {
        int found = 0;
        char* sectionname = (char*)malloc(100);
        for (int b = 1; b <= 2; b++)
        {
            for (int p = 0; p < num_presses; p++)
            {
                sprintf(sectionname, "button-%d-%s", b, presses[p]);
                for (int s = 0; s < num_states; s++)
                {
                    if (MATCH(sectionname, states[s]))
                    {
                        char temp[1000];
                        strcpy(temp, value);
                        char* token = strtok(temp, ",");
                        while (token != NULL)
                        {
                            try_add_action(pconfig, b, press_id[p], state_id[s], token);
                            token = strtok(NULL, ",");
                        }

                        found = 1;
                    }
                }
            }
        }

        if (found == 0)
        {
            printf("Unknown entry: [%s] %s\n", section, name);
        }

        free(sectionname);
        return 0;  /* unknown section/name, error */
    }

    return 1;
}

buttonconfiguration* foundactions[MAX_ACTIONS + 1];

buttonconfiguration** find_actions(configuration* config, int button, int press_id, int state_id)
{
    int count = 0;
    for (int i = 0; i < MAX_ACTIONS + 1; i++)
    {
        foundactions[i] = NULL;
    }

    buttonconfiguration** actions = config->buttonconfig;
    buttonconfiguration* action = *actions;

    while (action != NULL)
    {
        if (action->button == button &&
            action->press_id == press_id &&
            action->state_id == state_id)
        {
            foundactions[count] = action;
            count++;
        }

        action = *(++actions);
    }

    return foundactions;
}

configuration* read_config(char* filename)
{
    configuration* config;
    config = (configuration*)malloc(sizeof(configuration));
    // printf("Alloc Config %p - %d bytes\n", (void*)config, sizeof(configuration));

    for (int i = 0; i < MAX_ACTIONS; i++)
    {
        config->buttonconfig[i] = NULL;
    }

    if (ini_parse(filename, handler, config) < 0)
    {
        printf("Can't load 'initest.ini'\n");
        return NULL;
    }

    return config;
}
