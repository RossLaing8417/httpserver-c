#include <stddef.h>

#include "../common/types.h"

typedef enum URI_ERROR {
    URI_ERROR_BAD_FORMAT,
} URI_ERROR;

typedef struct Uri {
    CharSlice scheme;
    StrOpt user;
    StrOpt password;
    StrOpt host;
    u16Opt port;
    StrOpt path;
    StrOpt query;
    StrOpt fragment;
} Uri;

typedef AS_OPTION_TYPE(Uri) UriOpt;
typedef AS_ERROR_TYPE(URI_ERROR, Uri) UriOrErr;

UriOrErr parseUri(char * source, size_t len);
UriOrErr parseUriNoScheme(char * source, size_t len);
