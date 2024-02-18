typedef enum OPTION {
    OPTION_NONE,
    OPTION_ERROR,
    OPTION_SOME,
} OPTION;

#define AS_OPTION(OPTION_TYPE) struct { OPTION option; OPTION_TYPE; }

#define OPTION_UNION(SOME_TYPE) union { void *none; SOME_TYPE some; }
#define AS_OPTION_TYPE(SOME_TYPE) AS_OPTION(OPTION_UNION(SOME_TYPE))

#define ERROR_UNION(ERROR_TYPE, VALUE_TYPE) union { ERROR_TYPE error; VALUE_TYPE value; }
#define AS_ERROR_TYPE(ERROR_TYPE, VALUE_TYPE) AS_OPTION(ERROR_UNION(ERROR_TYPE, VALUE_TYPE))

#define AS_NONE() { .option = OPTION_NONE, .none = NULL }
#define AS_SOME(...) { .option = OPTION_SOME, .some = __VA_ARGS__ }

#define AS_ERROR(...) { .option = OPTION_ERROR, .error = __VA_ARGS__ }
#define AS_VALUE(...) { .option = OPTION_SOME, .value = __VA_ARGS__ }
