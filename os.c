#include "os.h"
#include "keys.h"
#include "lua/lauxlib.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "queue.h"
#include <Uefi.h>
#include <Library/UefiLib.h>

int running = 1;
const char * label;
bool label_defined = false;
queue_t eventQueue;
lua_State* paramQueue;
EFI_EVENT * timers = NULL;
unsigned timers_size = 0;
extern EFI_SYSTEM_TABLE * SystemTable;

int getNextEvent(lua_State *L, const char * filter) {
    const char * ev;
    do {
        EFI_INPUT_KEY key;
        while (queue_size(&eventQueue) == 0) {
            if (SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &key) == EFI_SUCCESS && key.UnicodeChar < 256) {
                int ch = getKey((key.UnicodeChar & 0xFF) | (key.UnicodeChar == 0 ? key.ScanCode << 8 : 0));
                if (ch != -1) {
                    lua_State *param = lua_newthread(paramQueue);
                    lua_pushinteger(param, ch);
                    lua_pushboolean(param, false);
                    queue_push(&eventQueue, "key");
                    param = lua_newthread(paramQueue);
                    lua_pushinteger(param, ch);
                    queue_push(&eventQueue, "key_up");
                }
                if (key.UnicodeChar >= 32 && key.UnicodeChar < 128) {
                    char str[2];
                    str[0] = key.UnicodeChar & 0xFF;
                    str[1] = 0;
                    lua_State *param = lua_newthread(paramQueue);
                    lua_pushstring(param, str);
                    queue_push(&eventQueue, "char");
                }
            }
            for (int i = 0; i < timers_size; i++) {
                if (timers[i] != NULL && SystemTable->BootServices->CheckEvent(timers[i]) == EFI_SUCCESS) {
                    lua_State *param = lua_newthread(paramQueue);
                    lua_pushinteger(param, i);
                    queue_push(&eventQueue, "timer");
                    timers[i] = NULL;
                }
            }
        }
        ev = queue_front(&eventQueue);
        queue_pop(&eventQueue);
    } while (strlen(filter) > 0 && strcmp(ev, filter) != 0);
    lua_State *param = lua_tothread(paramQueue, 1);
    if (param == NULL) return 0;
    int count = lua_gettop(param);
    if (!lua_checkstack(L, count + 1)) {
        printf("Could not allocate enough space in the stack for %d elements, skipping event \"%s\"\n", count, ev);
        return 0;
    }
    lua_pushstring(L, ev);
    lua_xmove(param, L, count);
    lua_remove(paramQueue, 1);
    return count + 1;
}

int os_getComputerID(lua_State *L) {lua_pushinteger(L, 0); return 1;}
int os_getComputerLabel(lua_State *L) {
    if (!label_defined) return 0;
    lua_pushstring(L, label);
    return 1;
}

int os_setComputerLabel(lua_State *L) {
    label = lua_tostring(L, 1);
    label_defined = true;
    return 0;
}

int os_queueEvent(lua_State *L) {
    int count = lua_gettop(L);
    const char * name = lua_tostring(L, 1);
    lua_State *param = lua_newthread(paramQueue);
    lua_xmove(L, param, count - 1);
    queue_push(&eventQueue, name);
    return 0;
}

int os_clock(lua_State *L) {
    lua_pushinteger(L, clock() / CLOCKS_PER_SEC);
    return 1;
}

int os_startTimer(lua_State *L) {
    EFI_EVENT ev;
    SystemTable->BootServices->CreateEvent(EVT_TIMER, TPL_APPLICATION, NULL, NULL, &ev);
    int id;
    if (timers_size == 0) {
        timers = (EFI_EVENT*)malloc(sizeof(EFI_EVENT));
        timers_size = 1;
        id = 0;
    } else {
        for (id = 0; id < timers_size; id++) if (timers[id] == NULL) break;
        if (id == timers_size) timers = (EFI_EVENT*)realloc(timers, sizeof(EFI_EVENT) * ++timers_size);
    }
    timers[id] = ev;
    SystemTable->BootServices->SetTimer(ev, TimerRelative, (UINT64)(lua_tonumber(L, 1) * 10000000.0));
    lua_pushinteger(L, id);
    return 1;
}

int os_cancelTimer(lua_State *L) {
    int id = lua_tointeger(L, 1);
    if (id < 0 || id >= timers_size || timers[id] == NULL) return 0;
    SystemTable->BootServices->SetTimer(timers[id], TimerCancel, 0);
    timers[id] = NULL;
    return 0;
}

int os_time(lua_State *L) {
    const char * type = "ingame";
    if (lua_gettop(L) > 0) type = lua_tostring(L, 1);
    time_t t = time(NULL);
    struct tm rightNow;
    if (strcmp(type, "utc") == 0) rightNow = *gmtime(&t);
    else rightNow = *localtime(&t);
    int hour = rightNow.tm_hour;
    int minute = rightNow.tm_min;
    int second = rightNow.tm_sec;
    lua_pushnumber(L, (double)hour + ((double)minute/60.0) + ((double)second/3600.0));
    return 1;
}

int os_epoch(lua_State *L) {
    const char * type = "ingame";
    if (lua_gettop(L) > 0) type = lua_tostring(L, 1);
    if (strcmp(type, "utc") == 0) {
        lua_pushinteger(L, (long long)time(NULL) * 1000LL);
    } else if (strcmp(type, "local") == 0) {
        time_t t = time(NULL);
        lua_pushinteger(L, (long long)mktime(localtime(&t)) * 1000LL);
    } else {
        time_t t = time(NULL);
        struct tm rightNow = *localtime(&t);
        int hour = rightNow.tm_hour;
        int minute = rightNow.tm_min;
        int second = rightNow.tm_sec;
        double m_time = (double)hour + ((double)minute/60.0) + ((double)second/3600.0);
        double m_day = rightNow.tm_yday;
        lua_pushinteger(L, m_day * 86400000 + (int) (m_time * 3600000.0f));
    }
    return 1;
}

int os_day(lua_State *L) {
    const char * type = "ingame";
    if (lua_gettop(L) > 0) type = lua_tostring(L, 1);
    time_t t = time(NULL);
    if (strcmp(type, "ingame") == 0) {
        struct tm rightNow = *localtime(&t);
        lua_pushinteger(L, rightNow.tm_yday);
        return 1;
    } else if (strcmp(type, "local")) t = mktime(localtime(&t));
    lua_pushinteger(L, t/(60*60*24));
    return 1;
}

int os_setAlarm(lua_State *L) {
    //alarms.push_back(lua_tonumber(L, 1));
    //lua_pushinteger(L, alarms.size() - 1);
    return 0;
}

int os_cancelAlarm(lua_State *L) {
    //int id = lua_tointeger(L, 1);
    //if (id == alarms.size() - 1) alarms.pop_back();
    //else alarms[id] = -1;
    return 0;
}

int os_shutdown(lua_State *L) {
    running = 0;
    SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    return 0;
}

int os_reboot(lua_State *L) {
    running = 2;
    SystemTable->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
    return 0;
}

const char * os_keys[14] = {
    "getComputerID",
    "getComputerLabel",
    "setComputerLabel",
    "queueEvent",
    "clock",
    "startTimer",
    "cancelTimer",
    "time",
    "epoch",
    "day",
    "setAlarm",
    "cancelAlarm",
    "shutdown",
    "reboot"
};

lua_CFunction os_values[14] = {
    os_getComputerID,
    os_getComputerLabel,
    os_setComputerLabel,
    os_queueEvent,
    os_clock,
    os_startTimer,
    os_cancelTimer,
    os_time,
    os_epoch,
    os_day,
    os_setAlarm,
    os_cancelAlarm,
    os_shutdown,
    os_reboot
};

library_t os_lib = {"os", 14, os_keys, os_values};