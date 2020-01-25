#include "lib.h"
#include <Uefi.h>
#include <Library/UefiLib.h>
extern library_t term_lib;
extern void termInit(EFI_SYSTEM_TABLE *);
extern void termClose();