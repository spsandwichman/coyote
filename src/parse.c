#include "parse.h"

static struct {
    TypeBufSlot* at;
    u32 len;
    u32 cap;
} type_buffer;

