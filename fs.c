#include "fs.h"
#include "fs_handle.h"
#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>
#include <errno.h>
#include <dirent.h>
#include <wchar.h>
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

int fs_list(lua_State *L) {
    struct dirent *dir;
    char * path = fixpath(lua_tostring(L, 1));
    DIR * d = opendir(path);
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
    } else err(L, path, "Not a directory");
    free(path);
    return 1;
}

int fs_exists(lua_State *L) {
    char * path = fixpath(lua_tostring(L, 1));
    struct stat st;
    lua_pushboolean(L, stat(path, &st) == 0);
    free(path);
    return 1;
}

int fs_isDir(lua_State *L) {
    char * path = fixpath(lua_tostring(L, 1));
    struct stat st;
    lua_pushboolean(L, stat(path, &st) == 0 && S_ISDIR(st.st_mode));
    free(path);
    return 1;
}

int fs_isReadOnly(lua_State *L) {
    char * path = fixpath(lua_tostring(L, 1));
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
    struct stat st;
    if (stat(path, &st) != 0) err(L, path, "No such file");
    lua_pushinteger(L, st.st_size);
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