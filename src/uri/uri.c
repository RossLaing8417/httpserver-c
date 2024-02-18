/**
 * Uri parser, followed zig std uri
 */

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uri.h"

typedef struct {
    CharSlice slice;
    size_t offset;
} Reader;

CharOpt get(Reader *self) {
    if (self->offset >= self->slice.len) {
        CharOpt none = AS_NONE();
        return none;
    }
    CharOpt some = AS_SOME(self->slice.ptr[self->offset]);
    self->offset += 1;
    return some;
}

CharOpt peek(Reader *self) {
    if (self->offset >= self->slice.len) {
        CharOpt none = AS_NONE();
        return none;
    }
    CharOpt some = AS_SOME(self->slice.ptr[self->offset]);
    return some;
}

CharSlice readWhile(Reader *self, bool (*predicate)(char)) {
    const size_t start = self->offset;
    size_t end = start;
    while (end < self->slice.len && predicate(self->slice.ptr[end])) {
        end += 1;
    }
    self->offset = end;
    CharSlice result = {
        .ptr = self->slice.ptr + start,
        .len = end - start,
    };
    return result;
}

CharSlice readUntil(Reader *self, bool (*predicate)(char)) {
    const size_t start = self->offset;
    size_t end = start;
    while (end < self->slice.len && !predicate(self->slice.ptr[end])) {
        end += 1;
    }
    self->offset = end;
    CharSlice result = {
        .ptr = self->slice.ptr + start,
        .len = end - start,
    };
    return result;
}

CharSlice readUntilEof(Reader *self) {
    const size_t start = self->offset;
    self->offset = self->slice.len;
    CharSlice result = {
        .ptr = self->slice.ptr + start,
        .len = self->slice.len - start,
    };
    return result;
}

bool peekPrefix(Reader *self, const char * prefix) {
    size_t len = strlen(prefix);
    if (self->offset + len > self->slice.len) {
        return false;
    }
    return strncmp(self->slice.ptr + self->offset, prefix, len) == 0;
}

bool isSchemeChar(const char c) {
    return (isalnum(c) || c == '+' || c == '-' || c == '.');
}

bool isAuthoritySeparator(const char c) {
    return (c == '/' || c == '?' || c == '#');
}

bool isPathSeparator(const char c) {
    return (c == '?' || c == '#');
}

bool isQuerySeparator(const char c) {
    return (c == '#');
}

SizeTOpt indexOf(CharSlice slice, char * c) {
    char * c_ptr = memmem(slice.ptr, slice.len, c, strlen(c));
    if (c_ptr == NULL) {
        SizeTOpt none = AS_NONE();
        return none;
    }
    SizeTOpt some = AS_SOME(c_ptr - slice.ptr);
    return some;
}

SizeTOpt lastIndexOf(CharSlice slice, char * c) {
    // Does a right index exist?  memrmem?
    size_t len = strlen(c);

    if (slice.len > len) {
        char * c_ptr = NULL;
        for (int i = slice.len - len - 1; i >= 0; i--) {
            c_ptr = memmem(slice.ptr + i, slice.len - i, c, len);
            if (c_ptr != NULL) {
                SizeTOpt some = AS_SOME(c_ptr - slice.ptr);
                return some;
            }
        }
    }

    SizeTOpt none = AS_NONE();
    return none;
}

UriOrErr parseUri(char *source, size_t len) {
    Reader reader = { .slice = { .ptr = source, .len = len }, .offset = 0 };

    CharSlice scheme = readWhile(&reader, &isSchemeChar);

    if (scheme.len == len || source[scheme.len] != ':') {
        UriOrErr error = AS_ERROR(URI_ERROR_BAD_FORMAT);
        return error;
    }

    CharOpt char_opt = get(&reader);
    if (char_opt.option == OPTION_SOME) {
        if (char_opt.some != ':') {
            UriOrErr error = AS_ERROR(URI_ERROR_BAD_FORMAT);
            return error;
        }
    } else {
        UriOrErr error = AS_ERROR(URI_ERROR_BAD_FORMAT);
        return error;
    }

    CharSlice remaining = readUntilEof(&reader);
    UriOrErr uri_err = parseUriNoScheme(remaining.ptr, remaining.len);

    if (uri_err.option == OPTION_SOME) {
        uri_err.value.scheme = scheme;
    }

    return uri_err;
}

UriOrErr parseUriNoScheme(char *source, size_t len) {
    Uri uri = {
        .scheme = {0, ""},
        .user = AS_NONE(),
        .password = AS_NONE(),
        .host = AS_NONE(),
        .port = AS_NONE(),
        .path = AS_NONE(),
        .query = AS_NONE(),
        .fragment = AS_NONE(),
    };

    Reader reader = { .slice = { .ptr = source, .len = len }, .offset = 0 };

    if (peekPrefix(&reader, "//")) {
        assert(get(&reader).some == '/');
        assert(get(&reader).some == '/');

        CharSlice authority = readUntil(&reader, isAuthoritySeparator);
        if (authority.len == 0) {
            UriOrErr error = AS_ERROR(URI_ERROR_BAD_FORMAT);
            return error;
        }

        size_t start_of_host = 0;
        SizeTOpt index_opt = indexOf(authority, "@");
        if (index_opt.option == OPTION_SOME) {
            const size_t index = index_opt.some;
            start_of_host = index + 1;
            CharSlice user_info = { .ptr = authority.ptr, .len = index };

            SizeTOpt idx_opt = indexOf(user_info, ":");
            if (idx_opt.option == OPTION_SOME) {
                const size_t idx = idx_opt.some;
                StrOpt user = AS_SOME({ .ptr = user_info.ptr, .len = idx });
                uri.user = user;
                if (idx < user_info.len - 1) {
                    StrOpt password = AS_SOME({
                        .ptr = user_info.ptr + idx + 1,
                        .len = user_info.len - idx - 1,
                    });
                    uri.password = password;
                }
            } else {
                StrOpt user = AS_SOME(user_info);
                uri.user = user;
            }
        }

        size_t end_of_host = authority.len;

        if (authority.ptr[start_of_host] == '[') {
            SizeTOpt end_opt = lastIndexOf(authority, "]");
            if (end_opt.option == OPTION_NONE) {
                UriOrErr error = AS_ERROR(URI_ERROR_BAD_FORMAT);
                return error;
            }
            end_of_host = end_opt.some + 1;

            SizeTOpt index_opt = lastIndexOf(authority, ":");
            if (index_opt.option == OPTION_SOME) {
                const size_t index = index_opt.some;
                if (index >= end_of_host) {
                    end_of_host = (index < end_of_host ? index : end_of_host);
                    char buffer[authority.len - index];
                    strncpy(buffer, authority.ptr + index + 1, authority.len - index - 1);
                    buffer[authority.len - index - 1] = '\0';
                    u16Opt port = AS_SOME(atoi(buffer));
                    uri.port = port;
                }
            }
        } else {
            SizeTOpt idx_opt = lastIndexOf(authority, ":");
            if (idx_opt.option == OPTION_SOME) {
                const size_t index = idx_opt.some;
                if (index >= start_of_host) {
                    end_of_host = (index < end_of_host ? index : end_of_host);
                    char buffer[authority.len - index];
                    strncpy(buffer, authority.ptr + index + 1, authority.len - index - 1);
                    buffer[authority.len - index - 1] = '\0';
                    u16Opt port = AS_SOME(atoi(buffer));
                    uri.port = port;
                }
            }
        }

        StrOpt host = AS_SOME({
            .ptr = authority.ptr + start_of_host,
            .len = end_of_host - start_of_host
        });
        uri.host = host;
    }

    StrOpt path = AS_SOME(readUntil(&reader, isPathSeparator));
    uri.path = path;

    CharOpt peek_opt = peek(&reader);
    if (peek_opt.option == OPTION_SOME && peek_opt.some == '?') {
        assert(get(&reader).some == '?');
        StrOpt query = AS_SOME(readUntil(&reader, isQuerySeparator));
        uri.query = query;
    }

    peek_opt = peek(&reader);
    if (peek_opt.option == OPTION_SOME && peek_opt.some == '#') {
        assert(get(&reader).some == '#');
        StrOpt fragment = AS_SOME(readUntilEof(&reader));
        uri.fragment = fragment;
    }

    UriOrErr uri_val = AS_VALUE(uri);
    return uri_val;
}
