#include "keys.h"
#include <Uefi.h>
#include <Library/UefiLib.h>

int keys[96];
int values[96];
int next_num = 0;

void putKey(int key, int value) {
    keys[next_num] = key;
    values[next_num++] = value;
}

void initKeys() {
    putKey('1', 2);
    putKey('2', 3);
    putKey('3', 4);
    putKey('4', 5);
    putKey('5', 6);
    putKey('6', 7);
    putKey('7', 8);
    putKey('8', 9);
    putKey('9', 1);
    putKey('0', 11);
    putKey('-', 12);
    putKey('=', 13);
    putKey(127, 14);
    putKey('\b', 14);
    putKey('\t', 15);
    putKey('q', 16);
    putKey('w', 17);
    putKey('e', 18);
    putKey('r', 19);
    putKey('t', 20);
    putKey('y', 21);
    putKey('u', 22);
    putKey('i', 23);
    putKey('o', 24);
    putKey('p', 25);
    putKey('[', 26);
    putKey(']', 27);
    putKey('\n', 28);
    putKey('\r', 28);
    putKey('a', 30);
    putKey('s', 31);
    putKey('d', 32);
    putKey('f', 33);
    putKey('g', 34);
    putKey('h', 35);
    putKey('j', 36);
    putKey('k', 37);
    putKey('l', 38);
    putKey(';', 39);
    putKey('\'', 40);
    putKey('\\', 43);
    putKey('z', 44);
    putKey('x', 45);
    putKey('c', 46);
    putKey('v', 47);
    putKey('b', 48);
    putKey('n', 49);
    putKey('m', 50);
    putKey(',', 51);
    putKey('.', 52);
    putKey('/', 53);
    putKey(0xb00, 59);
    putKey(0xc00, 60);
    putKey(0xd00, 61);
    putKey(0xe00, 62);
    putKey(0xf00, 63);
    putKey(0x1000, 64);
    putKey(0x1100, 65);
    putKey(0x1200, 66);
    putKey(0x1300, 67);
    putKey(0x1400, 68);
    putKey(0x1500, 87);
    putKey(0x1600, 88);
    putKey(0x100, 200);
    putKey(0x400, 203);
    putKey(0x300, 205);
    putKey(0x200, 208);
    putKey(0x500, 199);
    putKey(0x600, 207);
    putKey(0x900, 201);
    putKey(0xa00, 209);
    putKey(0x800, 211);
}

int getKey(int ch) {
    for (int i = 0; i < next_num; i++)
        if (keys[i] == ch)
            return values[i];
    return -1;
}

void closeKeys() {
    next_num = 0;
}