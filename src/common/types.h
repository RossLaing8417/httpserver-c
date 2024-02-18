#include <stddef.h>
#include <stdint.h>

#include "option.h"

typedef struct CharSlice {
    size_t len;
    char * ptr;
} CharSlice;

typedef AS_OPTION_TYPE(char) CharOpt;
typedef AS_OPTION_TYPE(CharSlice) StrOpt;

typedef AS_OPTION_TYPE(uint16_t) u16Opt;
typedef AS_OPTION_TYPE(size_t) SizeTOpt;
