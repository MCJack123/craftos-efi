#include "fs.h"
#include "fs_handle.h"
#include "lua/lauxlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef NULL
#define NULL (void*)0
#endif
#include <unistd.h>
//#include <sys/stat.h>
//#include <libgen.h>
#include <errno.h>
//#include <dirent.h>
#define true 1
#define false 0

/*
char * fixpath(const char * path) {
    char * retval = (char*)malloc(strlen(path) + 6);
    strcpy(retval, "FS0:");
    if (path[0] != '/') strcat(retval, "\\");
    strcat(retval, path);
    for (int i = 0; i < strlen(retval); i++) if (retval[i] == '/') retval[i] = '\\';
    return retval;
}
*/
char * fixpath(const char * path) {
    char * retval = (char*)malloc(strlen(path) + 2);
    if (path[0] != '/') {
        retval[0] = '/';
        strcpy(&retval[1], path);
    } else strcpy(retval, path);
    return retval;
}

void err(lua_State *L, char * path, const char * err) {
    //free(path);
    luaL_error(L, "%s: %s (%d)", path, err, errno);
}

char * unconst(const char * str) {
    char * retval = malloc(strlen(str) + 1);
    strcpy(retval, str);
    return retval;
}

const char * fs_list_rom[] = {
    "apis",
    "autorun",
    "help",
    "modules",
    "programs",
    "startup.lua",
    NULL
};

const char * fs_list_rom_apis[] = {
    "colors.lua",
    "colours.lua",
    "command",
    "disk.lua",
    "gps.lua",
    "help.lua",
    "io.lua",
    "keys.lua",
    "paintutils.lua",
    "parallel.lua",
    "peripheral.lua",
    "rednet.lua",
    "settings.lua",
    "term.lua",
    "textutils.lua",
    "turtle",
    "vector.lua",
    "window.lua",
    NULL
};

const char * fs_list_rom_apis_command[] = {"commands.lua", NULL};
const char * fs_list_rom_apis_turtle[] = {"turtle.lua", NULL};
const char * fs_list_ignoreme[] = {".ignoreme", NULL};

const char * fs_list_rom_help[] = {
    "adventure.txt",
    "alias.txt",
    "apis.txt",
    "bg.txt",
    "bit.txt",
    "bundled.txt",
    "cd.txt",
    "changelog.txt",
    "chat.txt",
    "clear.txt",
    "colors.txt",
    "colours.txt",
    "commands.txt",
    "commandsapi.txt",
    "copy.txt",
    "coroutine.txt",
    "craft.txt",
    "credits.txt",
    "dance.txt",
    "delete.txt",
    "disk.txt",
    "dj.txt",
    "drive.txt",
    "drives.txt",
    "earth.txt",
    "edit.txt",
    "eject.txt",
    "equip.txt",
    "events.txt",
    "excavate.txt",
    "exec.txt",
    "exit.txt",
    "falling.txt",
    "fg.txt",
    "fs.txt",
    "go.txt",
    "gps.txt",
    "gpsapi.txt",
    "hello.txt",
    "help.txt",
    "helpapi.txt",
    "http.txt",
    "id.txt",
    "intro.txt",
    "io.txt",
    "keys.txt",
    "label.txt",
    "list.txt",
    "lua.txt",
    "math.txt",
    "mkdir.txt",
    "modems.txt",
    "monitor.txt",
    "monitors.txt",
    "move.txt",
    "multishell.txt",
    "os.txt",
    "paint.txt",
    "paintutils.txt",
    "parallel.txt",
    "pastebin.txt",
    "peripheral.txt",
    "peripherals.txt",
    "pocket.txt",
    "printers.txt",
    "programming.txt",
    "programs.txt",
    "reboot.txt",
    "redirection.txt",
    "rednet.txt",
    "redstone.txt",
    "redstoneapi.txt",
    "refuel.txt",
    "rename.txt",
    "repeat.txt",
    "rs.txt",
    "set.txt",
    "settings.txt",
    "shell.txt",
    "shellapi.txt",
    "shutdown.txt",
    "speakers.txt",
    "string.txt",
    "table.txt",
    "term.txt",
    "textutils.txt",
    "time.txt",
    "tunnel.txt",
    "turn.txt",
    "turtle.txt",
    "type.txt",
    "unequip.txt",
    "vector.txt",
    "wget.txt",
    "whatsnew.txt",
    "window.txt",
    "workbench.txt",
    "worm.txt",
    NULL
};

const char * fs_list_rom_modules[] = {"command", "main", "turtle", NULL};

const char * fs_list_rom_programs[] = {
    "advanced",
    "alias.lua",
    "apis.lua",
    "cd.lua",
    "clear.lua",
    "command",
    "copy.lua",
    "delete.lua",
    "drive.lua",
    "edit.lua",
    "eject.lua",
    "exit.lua",
    "fun",
    "gps.lua",
    "help.lua",
    "http",
    "id.lua",
    "label.lua",
    "list.lua",
    "lua.lua",
    "mkdir.lua",
    "monitor.lua",
    "move.lua",
    "peripherals.lua",
    "pocket",
    "programs.lua",
    "reboot.lua",
    "rednet",
    "redstone.lua",
    "rename.lua",
    "set.lua",
    "shell.lua",
    "shutdown.lua",
    "time.lua",
    "turtle",
    "type.lua",
    NULL
};

const char * fs_list_rom_programs_advanced[] = {"bg.lua", "fg.lua", "multishell.lua", NULL};
const char * fs_list_rom_programs_command[] = {"commands.lua", "exec.lua", NULL};

const char * fs_list_rom_programs_fun[] = {
    "advanced",
    "adventure.lua",
    "dj.lua",
    "hello.lua",
    "worm.lua",
    NULL
};

const char * fs_list_rom_programs_fun_advanced[] = {"levels", "paint.lua", "redirection.lua", NULL};

const char * fs_list_rom_programs_fun_advanced_levels[] = {
    "0.dat",
    "1.dat",
    "10.dat",
    "11.dat",
    "12.dat",
    "2.dat",
    "3.dat",
    "4.dat",
    "5.dat",
    "6.dat",
    "7.dat",
    "8.dat",
    "9.dat",
    NULL
};

const char * fs_list_rom_programs_http[] = {"pastebin.lua", "wget.lua", NULL};
const char * fs_list_rom_programs_pocket[] = {"equip.lua", "falling.lua", "unequip.lua", NULL};
const char * fs_list_rom_programs_rednet[] = {"chat.lua", "repeat.lua", NULL};

const char * fs_list_rom_programs_turtle[] = {
    "craft.lua",
    "dance.lua",
    "equip.lua",
    "excavate.lua",
    "go.lua",
    "refuel.lua",
    "tunnel.lua",
    "turn.lua",
    "unequip.lua",
    NULL
};

const char * fs_list_[] = {"rom", NULL};

char* dirname(char* path) {
	if (path[0] == '/') strcpy(path, &path[1]);
    char tch;
    if (strrchr(path, '/') != NULL) tch = '/';
    else if (strrchr(path, '\\') != NULL) tch = '\\';
    else return path;
    path[strrchr(path, tch) - path] = '\0';
	return path;
}

int fs_list(lua_State *L) {
    //struct dirent *dir;
    char * path = fixpath(lua_tostring(L, 1));
    if (strlen(path) > 1 && path[strlen(path)-1] == '/') path[strlen(path)-1] = 0;
    /*DIR * d = opendir(path);
    if (d) {
        lua_newtable(L);
        for (int i = 0; (dir = readdir(d)) != NULL; i++) {
            lua_pushinteger(L, i);
            char * str = (char*)malloc(wcslen(dir->d_name)+1);
            str[wcslen(dir->d_name)]=0;
            for (int j = 0; j < wcslen(dir->d_name); j++) str[j] = dir->d_name[j];
            lua_pushstring(L, str);
            free(str);
            lua_settable(L, -3);
        }
        closedir(d);
    } else err(L, path, "Not a directory");*/
    const char ** filelist = NULL;
    if (strcmp(path, "/") == 0) filelist = fs_list_;
    else if (strcmp(path, "/rom") == 0) filelist = fs_list_rom;
    else if (strcmp(path, "/rom/apis") == 0) filelist = fs_list_rom_apis;
    else if (strcmp(path, "/rom/apis/command") == 0) filelist = fs_list_rom_apis_command;
    else if (strcmp(path, "/rom/apis/turtle") == 0) filelist = fs_list_rom_apis_turtle;
    else if (strcmp(path, "/rom/autorun") == 0) filelist = fs_list_ignoreme;
    else if (strcmp(path, "/rom/help") == 0) filelist = fs_list_rom_help;
    else if (strcmp(path, "/rom/modules") == 0) filelist = fs_list_rom_modules;
    else if (strcmp(path, "/rom/modules/command") == 0) filelist = fs_list_ignoreme;
    else if (strcmp(path, "/rom/modules/main") == 0) filelist = fs_list_ignoreme;
    else if (strcmp(path, "/rom/modules/turtle") == 0) filelist = fs_list_ignoreme;
    else if (strcmp(path, "/rom/programs") == 0) filelist = fs_list_rom_programs;
    else if (strcmp(path, "/rom/programs/advanced") == 0) filelist = fs_list_rom_programs_advanced;
    else if (strcmp(path, "/rom/programs/command") == 0) filelist = fs_list_rom_programs_command;
    else if (strcmp(path, "/rom/programs/fun") == 0) filelist = fs_list_rom_programs_fun;
    else if (strcmp(path, "/rom/programs/fun/advanced") == 0) filelist = fs_list_rom_programs_fun_advanced;
    else if (strcmp(path, "/rom/programs/fun/advanced/levels") == 0) filelist = fs_list_rom_programs_fun_advanced_levels;
    else if (strcmp(path, "/rom/programs/http") == 0) filelist = fs_list_rom_programs_http;
    else if (strcmp(path, "/rom/programs/pocket") == 0) filelist = fs_list_rom_programs_pocket;
    else if (strcmp(path, "/rom/programs/rednet") == 0) filelist = fs_list_rom_programs_rednet;
    else if (strcmp(path, "/rom/programs/turtle") == 0) filelist = fs_list_rom_programs_turtle;
    else err(L, path, "Not a directory");
    lua_newtable(L);
    for (int i = 0; filelist[i] != NULL; i++) {
        lua_pushinteger(L, i+1);
        lua_pushstring(L, filelist[i]);
        lua_settable(L, -3);
    }
    free(path);
    return 1;
}

int fs_exists(lua_State *L) {
    char * path = fixpath(lua_tostring(L, 1));
    //struct stat st;
    //lua_pushboolean(L, stat(path, &st) == 0);
    FILE *fp = fopen(path, "r");
    lua_pushboolean(L, fp != NULL);
    if (fp != NULL) fclose(fp);
    free(path);
    return 1;
}

int fs_isDir(lua_State *L) {
    char * path = fixpath(lua_tostring(L, 1));
    //struct stat st;
    //lua_pushboolean(L, stat(path, &st) == 0 && S_ISDIR(st.st_mode));
    lua_pushboolean(L, strchr(path, '.') == NULL);
    free(path);
    return 1;
}

int fs_isReadOnly(lua_State *L) {
    char * path = fixpath(lua_tostring(L, 1));
    //lua_pushboolean(L, true);
    lua_pushboolean(L, access(path, W_OK) != 0);
    free(path);
    return 1;
}

int fs_getName(lua_State *L) {
    char * path = unconst(lua_tostring(L, 1));
    lua_pushstring(L, basename(path));
    free(path);
    return 1;
}

int fs_getDrive(lua_State *L) {
    lua_pushstring(L, "hdd");
    return 1;
}

int fs_getSize(lua_State *L) {
    char * path = fixpath(lua_tostring(L, 1));
    //struct stat st;
    //if (stat(path, &st) != 0) err(L, path, "No such file");
    //lua_pushinteger(L, st.st_size);
    FILE *fp = fopen(path, "r");
    if (fp == NULL) err(L, path, "No such file");
    int i = 0;
    char tmp[512];
    while (!feof(fp)) i += fread(tmp, 512, 1, fp);
    fclose(fp);
    lua_pushinteger(L, i);
    free(path);
    return 1;
}

int fs_getFreeSpace(lua_State *L) {
    lua_pushinteger(L, 100000000);
    return 1;
}

int recurse_mkdir(const char * path) {
    if (mkdir(path, 0777) != 0) {
        if (errno == ENOENT && strcmp(path, "/") != 0) {
            if (recurse_mkdir(dirname(unconst(path)))) return 1;
            mkdir(path, 0777);
            return 0;
        } else return 1;
    } else return 0;
}

int fs_makeDir(lua_State *L) {
    char * path = fixpath(lua_tostring(L, 1));
    if (recurse_mkdir(path) != 0) err(L, path, strerror(errno));
    free(path);
    return 0;
}

int fs_move(lua_State *L) {
    char * fromPath = fixpath(lua_tostring(L, 1));
    char * toPath = fixpath(lua_tostring(L, 2));
    if (rename(fromPath, toPath) != 0) {
        free(toPath);
        err(L, fromPath, strerror(errno));
    }
    free(fromPath);
    free(toPath);
    return 0;
}

int fs_copy(lua_State *L) {
    char * fromPath = fixpath(lua_tostring(L, 1));
    char * toPath = fixpath(lua_tostring(L, 2));

    FILE * fromfp = fopen(fromPath, "r");
    if (fromfp == NULL) {
        free(toPath);
        err(L, fromPath, "Cannot read file");
    }
    FILE * tofp = fopen(toPath, "w");
    if (tofp == NULL) {
        free(fromPath);
        fclose(fromfp);
        err(L, toPath, "Cannot write file");
    }

    char tmp[1024];
    while (!feof(fromfp)) {
        int read = fread(tmp, 1, 1024, fromfp);
        fwrite(tmp, read, 1, tofp);
        if (read < 1024) break;
    }

    fclose(fromfp);
    fclose(tofp);
    free(fromPath);
    free(toPath);
    return 0;
}

int fs_delete(lua_State *L) {
    char * path = fixpath(lua_tostring(L, 1));
    if (unlink(path) != 0) err(L, path, strerror(errno));
    free(path);
    return 0;
}

int fs_combine(lua_State *L) {
    const char * basePath = lua_tostring(L, 1);
    char * localPath = fixpath(lua_tostring(L, 2));
    if (basePath[0] == '/') basePath = basePath + 1;
    if (basePath[strlen(basePath)-1] == '/') localPath = localPath + 1;
    char * retval = (char*)malloc(strlen(basePath) + strlen(localPath) + 1);
    strcpy(retval, basePath);
    strcat(retval, localPath);
    if (basePath[strlen(basePath)-1] == '/') localPath = localPath - 1;
    free(localPath);
    lua_pushstring(L, retval);
    return 1;
}

int fs_open(lua_State *L) {
    char * path = fixpath(lua_tostring(L, 1));
    const char * mode = lua_tostring(L, 2);
    FILE * fp = fopen(path, mode);
    if (fp == NULL) err(L, path, strerror(errno));
    free(path);
    lua_newtable(L);
    lua_pushstring(L, "close");
    lua_pushlightuserdata(L, fp);
    lua_pushcclosure(L, handle_close, 1);
    lua_settable(L, -3);
    if (strcmp(mode, "r") == 0) {
        lua_pushstring(L, "readAll");
        lua_pushlightuserdata(L, fp);
        lua_pushcclosure(L, handle_readAll, 1);
        lua_settable(L, -3);

        lua_pushstring(L, "readLine");
        lua_pushlightuserdata(L, fp);
        lua_pushcclosure(L, handle_readLine, 1);
        lua_settable(L, -3);

        lua_pushstring(L, "read");
        lua_pushlightuserdata(L, fp);
        lua_pushcclosure(L, handle_readChar, 1);
        lua_settable(L, -3);
    } else if (strcmp(mode, "w") == 0 || strcmp(mode, "a") == 0) {
        lua_pushstring(L, "write");
        lua_pushlightuserdata(L, fp);
        lua_pushcclosure(L, handle_writeString, 1);
        lua_settable(L, -3);

        lua_pushstring(L, "writeLine");
        lua_pushlightuserdata(L, fp);
        lua_pushcclosure(L, handle_writeLine, 1);
        lua_settable(L, -3);

        lua_pushstring(L, "flush");
        lua_pushlightuserdata(L, fp);
        lua_pushcclosure(L, handle_flush, 1);
        lua_settable(L, -3);
    } else if (strcmp(mode, "rb") == 0) {
        lua_pushstring(L, "read");
        lua_pushlightuserdata(L, fp);
        lua_pushcclosure(L, handle_readByte, 1);
        lua_settable(L, -3);
    } else if (strcmp(mode, "wb") == 0) {
        lua_pushstring(L, "write");
        lua_pushlightuserdata(L, fp);
        lua_pushcclosure(L, handle_writeByte, 1);
        lua_settable(L, -3);

        lua_pushstring(L, "flush");
        lua_pushlightuserdata(L, fp);
        lua_pushcclosure(L, handle_flush, 1);
        lua_settable(L, -3);
    } else {
        lua_remove(L, -1);
        err(L, unconst(mode), "Invalid mode");
    }
    return 1;
}

int fs_find(lua_State *L) {
    lua_newtable(L);
    return 1;
}

int fs_getDir(lua_State *L) {
    char * path = unconst(lua_tostring(L, 1));
    lua_pushstring(L, dirname(path));
    free(path);
    return 1;
}

const char * fs_keys[16] = {
    "list",
    "exists",
    "isDir",
    "isReadOnly",
    "getName",
    "getDrive",
    "getSize",
    "getFreeSpace",
    "makeDir",
    "move",
    "copy",
    "delete",
    "combine",
    "open",
    "find",
    "getDir"
};

lua_CFunction fs_values[16] = {
    fs_list,
    fs_exists,
    fs_isDir,
    fs_isReadOnly,
    fs_getName,
    fs_getDrive,
    fs_getSize,
    fs_getFreeSpace,
    fs_makeDir,
    fs_move,
    fs_copy,
    fs_delete,
    fs_combine,
    fs_open,
    fs_find,
    fs_getDir
};

library_t fs_lib = {"fs", 16, fs_keys, fs_values};