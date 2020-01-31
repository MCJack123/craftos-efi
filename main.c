#include <Lua/lauxlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Uefi.h>
#include <Library/UefiLib.h>

#include "bit.h"
#include "fs.h"
#include "os.h"
#include "term.h"
#include "redstone.h"
#include "keys.h"
#include "queue.h"

extern lua_State* paramQueue;
extern queue_t eventQueue;
extern const char * bios_str;
extern EFI_SYSTEM_TABLE * GlobalSystemTable;

int main (IN int argc, IN char** argv) {
    Print(L"Hello World!\n");
    int status;
    lua_State *L;
    lua_State *coro;
    eventQueue._back = NULL;
    eventQueue._front = NULL;
start:
    /*
     * All Lua contexts are held in this structure. We work with it almost
     * all the time.
     */
    
    L = luaL_newstate();
    
    coro = lua_newthread(L);
    paramQueue = lua_newthread(L);
    
    luaL_openlibs(coro); /* Load Lua libraries */
    load_library(coro, bit_lib);
    load_library(coro, fs_lib);
    load_library(coro, os_lib);
    load_library(coro, rs_lib);
    lua_getglobal(coro, "redstone");
    lua_setglobal(coro, "rs");
    load_library(coro, term_lib);
    termInit(GlobalSystemTable);
    initKeys();
    
    lua_pushstring(L, "bios.use_multishell=false");
    lua_setglobal(L, "_CC_DEFAULT_SETTINGS");
    lua_pushstring(L, "CraftOS-EFI 1.8");
    lua_setglobal(L, "_HOST");
    
    /* Load the file containing the script we are going to run */
    status = luaL_loadstring(coro, bios_str);
    if (status) {
        /* If something went wrong, error message is at the top of */
        /* the stack */
        GlobalSystemTable->ConOut->OutputString(GlobalSystemTable->ConOut, L"Couldn't load BIOS: ");
        const char * fullstr = lua_tostring(L, -1);
        CHAR16 str[2];
        str[1] = 0;
        for (int i = 0; i < lua_strlen(L, -1); i++) {
            str[0] = fullstr[i];
            GlobalSystemTable->ConOut->OutputString(GlobalSystemTable->ConOut, str);
        }
        for (int i = 0; i < 9999999; i++);
        return 2;
    }
    
    /* Ask Lua to run our little script */
    status = LUA_YIELD;
    int narg = 0;
    while (status == LUA_YIELD && running == 1) {
        status = lua_resume(coro, narg);
        if (status == LUA_YIELD) {
            if (lua_isstring(coro, -1)) narg = getNextEvent(coro, lua_tostring(coro, -1));
            else narg = getNextEvent(coro, "");
        } else if (status != 0) {
            termClose();
            const char * fullstr = lua_tostring(coro, -1);
            CHAR16 str[2];
            str[1] = 0;
            for (int i = 0; i < lua_strlen(coro, -1); i++) {
                str[0] = fullstr[i];
                GlobalSystemTable->ConOut->OutputString(GlobalSystemTable->ConOut, str);
            }
            lua_close(L);
            return 1;
        }
        
    }
    termClose();
    closeKeys();
    lua_close(L);   /* Cya, Lua */
    
    if (running == 2) {
        goto start;
    }
    return 0;
}
