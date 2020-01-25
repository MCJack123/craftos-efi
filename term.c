#include "term.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

EFI_SYSTEM_TABLE * SystemTable;
UINTN width, height;
unsigned char * screenBuffer;
unsigned char * colorBuffer;

int cctoefi[16] = {EFI_WHITE, EFI_LIGHTCYAN, EFI_LIGHTMAGENTA, EFI_LIGHTBLUE, EFI_YELLOW, EFI_LIGHTGREEN, EFI_LIGHTRED, EFI_DARKGRAY, EFI_LIGHTGRAY, EFI_CYAN, EFI_MAGENTA, EFI_BLUE, EFI_BROWN, EFI_GREEN, EFI_RED, EFI_BLACK};
int efitocc[16] = {0xf, 0xb, 0xd, 9, 0xe, 0xa, 0xc, 8, 7, 3, 5, 1, 6, 2, 4, 0};
int colorswap(int x) {return cctoefi[x];}

CHAR16* towstr(const char * str) {
    CHAR16* retval = (CHAR16*)malloc(strlen(str)*2+2);
    retval[strlen(str)] = 0;
    for (int i = 0; i < strlen(str); i++) retval[i] = str[i];
    return retval;
}

void termInit(EFI_SYSTEM_TABLE * st) {
    SystemTable = st;
    SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
    SystemTable->ConOut->Reset(SystemTable->ConOut, TRUE);
    SystemTable->ConIn->Reset(SystemTable->ConIn, TRUE);
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    SystemTable->ConOut->QueryMode(SystemTable->ConOut, SystemTable->ConOut->Mode->Mode, &width, &height);
    screenBuffer = (unsigned char*)malloc(width*height);
    colorBuffer = (unsigned char*)malloc(width*height);
    memset(screenBuffer, ' ', width*height);
    memset(colorBuffer, 0x0F, width*height);
}

void termClose() {
    free(screenBuffer);
    free(colorBuffer);
}

int term_write(lua_State *L) {
    size_t len;
    const char * str = lua_tolstring(L, 1, &len); 
    CHAR16 ch[2];
    ch[1] = 0;
    for (size_t i = 0; i < len && SystemTable->ConOut->Mode->CursorColumn < width - 1; i++) {
        ch[0] = str[i];
        screenBuffer[SystemTable->ConOut->Mode->CursorColumn + SystemTable->ConOut->Mode->CursorRow*width] = str[i];
        colorBuffer[SystemTable->ConOut->Mode->CursorColumn + SystemTable->ConOut->Mode->CursorRow*width] = SystemTable->ConOut->Mode->Attribute;
        SystemTable->ConOut->OutputString(SystemTable->ConOut, ch);
        SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, SystemTable->ConOut->Mode->CursorColumn, SystemTable->ConOut->Mode->CursorRow);
    }
    return 0;
}

int term_scroll(lua_State *L) {
    int scroll = lua_tointeger(L, 1);
    // first we have to scroll the screen buffer
    for (int i = scroll; i < height; i++) memcpy(&screenBuffer[(i-scroll)*width], &screenBuffer[i*width], width);
    memset(&screenBuffer[(height-scroll)*width], ' ', width*scroll);
    for (int i = scroll; i < height; i++) memcpy(&colorBuffer[(i-scroll)*width], &colorBuffer[i*width], width);
    memset(&colorBuffer[(height-scroll)*width], SystemTable->ConOut->Mode->Attribute, width*scroll);
    // then we have to update the actual console
    int oldx = SystemTable->ConOut->Mode->CursorColumn, oldy = SystemTable->ConOut->Mode->CursorRow;
    bool oldblink = SystemTable->ConOut->Mode->CursorVisible;
    SystemTable->ConOut->EnableCursor(SystemTable->ConOut, false);
    CHAR16 ch[2];
    ch[1] = 0;
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width - (y == height - 1); x++) {
            ch[0] = screenBuffer[y*width+x];
            SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, x, y);
            SystemTable->ConOut->SetAttribute(SystemTable->ConOut, colorBuffer[y*width+x]);
            SystemTable->ConOut->OutputString(SystemTable->ConOut, ch);
        }
    }
    SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, oldx, oldy);
    SystemTable->ConOut->EnableCursor(SystemTable->ConOut, oldblink);
    return 0;
}

int term_setCursorPos(lua_State *L) {
    int x = lua_tointeger(L, 1)-1;
    int y = lua_tointeger(L, 2)-1;
    SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, x, y);
    return 0;
}

int term_setCursorBlink(lua_State *L) {
    SystemTable->ConOut->EnableCursor(SystemTable->ConOut, lua_toboolean(L, 1));
    return 0;
}

int term_getCursorPos(lua_State *L) {
    lua_pushinteger(L, SystemTable->ConOut->Mode->CursorColumn+1);
    lua_pushinteger(L, SystemTable->ConOut->Mode->CursorRow+1);
    return 2;
}

int term_getSize(lua_State *L) {
    lua_pushinteger(L, width);
    lua_pushinteger(L, height);
    return 2;
}

int term_clear(lua_State *L) {
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    memset(screenBuffer, 0, width*height);
    return 0;
}

int term_clearLine(lua_State *L) {
    SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, 0, SystemTable->ConOut->Mode->CursorRow);
    for (int i = 0; i < width; i++) SystemTable->ConOut->OutputString(SystemTable->ConOut, L" ");
    memset(&screenBuffer[SystemTable->ConOut->Mode->CursorRow*width], 0, width);
    memset(&colorBuffer[SystemTable->ConOut->Mode->CursorRow*width], SystemTable->ConOut->Mode->Attribute, width);
    return 0;
}

/*unsigned int log2( unsigned int x ) {
    unsigned int ans = 0;
    while (x >>= 1) ans++;
    return ans;
}*/

int term_setTextColor(lua_State *L) {
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, (SystemTable->ConOut->Mode->Attribute & ~0x0F) | colorswap((int)log2(lua_tointeger(L, 1))));
    return 0;
}

int term_setBackgroundColor(lua_State *L) {
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, (SystemTable->ConOut->Mode->Attribute & ~0x70) | (colorswap((int)log2(lua_tointeger(L, 1)) & 7) << 4));
    return 0;
}

int term_isColor(lua_State *L) {
    lua_pushboolean(L, true);
    return 1;
}

int term_getTextColor(lua_State *L) {
    lua_pushinteger(L, (1 << efitocc[SystemTable->ConOut->Mode->Attribute & 0x0F]));
    return 1;
}

int term_getBackgroundColor(lua_State *L) {
    lua_pushinteger(L, (1 << efitocc[(SystemTable->ConOut->Mode->Attribute & 0x70) >> 4]));
    return 1;
}

int hexch(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    else return -1;
}

int term_blit(lua_State *L) {
    size_t len, blen, flen;
    const char * str = lua_tolstring(L, 1, &len);
    const char * fg = lua_tolstring(L, 2, &flen);
    const char * bg = lua_tolstring(L, 3, &blen);
    if (len != flen || flen != blen) {
        lua_pushstring(L, "arguments must be the same length");
        lua_error(L);
    }
    CHAR16 ch[2];
    ch[1] = 0;
    for (size_t i = 0; i < len && SystemTable->ConOut->Mode->CursorColumn < width; i++) {
        ch[0] = str[i];
        screenBuffer[SystemTable->ConOut->Mode->CursorColumn + SystemTable->ConOut->Mode->CursorRow*width] = str[i];
        colorBuffer[SystemTable->ConOut->Mode->CursorColumn + SystemTable->ConOut->Mode->CursorRow*width] = colorswap(hexch(bg[i])) << 4 | colorswap(hexch(fg[i]));
        SystemTable->ConOut->SetAttribute(SystemTable->ConOut, colorswap(hexch(bg[i])) << 4 | colorswap(hexch(fg[i])));
        SystemTable->ConOut->OutputString(SystemTable->ConOut, ch);
        SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, SystemTable->ConOut->Mode->CursorColumn, SystemTable->ConOut->Mode->CursorRow);
    }
    return 0;
}

int term_getPaletteColor(lua_State *L) {
    return 0;
}

int term_setPaletteColor(lua_State *L) {
    return 0;
}

const char * term_keys[23] = {
    "write",
    "scroll",
    "setCursorPos",
    "setCursorBlink",
    "getCursorPos",
    "getSize",
    "clear",
    "clearLine",
    "setTextColour",
    "setTextColor",
    "setBackgroundColour",
    "setBackgroundColor",
    "isColour",
    "isColor",
    "getTextColour",
    "getTextColor",
    "getBackgroundColour",
    "getBackgroundColor",
    "blit",
    "getPaletteColor",
    "getPaletteColour",
    "setPaletteColor",
    "setPaletteColour"
};

lua_CFunction term_values[23] = {
    term_write,
    term_scroll,
    term_setCursorPos,
    term_setCursorBlink,
    term_getCursorPos,
    term_getSize,
    term_clear,
    term_clearLine,
    term_setTextColor,
    term_setTextColor,
    term_setBackgroundColor,
    term_setBackgroundColor,
    term_isColor,
    term_isColor,
    term_getTextColor,
    term_getTextColor,
    term_getBackgroundColor,
    term_getBackgroundColor,
    term_blit,
    term_getPaletteColor,
    term_getPaletteColor,
    term_setPaletteColor,
    term_setPaletteColor
};

library_t term_lib = {"term", 23, term_keys, term_values};