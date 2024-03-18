#include <klee/klee.h>

#define FLAGS_IMPLEMENTATION
#include "flags.h"

#define countof(arr) (sizeof(arr) / sizeof(*arr))

bool  flag_a = false;
bool  flag_b = false;
char *flag_c = NULL;
bool *flag_d = false;
bool *flag_e = false;
char *flag_f = NULL;
char *flag_g = NULL;

#define LONG_NAME_SIZE 16

char first_long[LONG_NAME_SIZE];
char second_long[LONG_NAME_SIZE];
char third_long[LONG_NAME_SIZE];

struct flag flags[] = {
    {FT_BOOL,   'a', first_long,  &flag_a},
    {FT_BOOL,   'b', second_long, &flag_b},
    {FT_STRING, 'c', third_long,  &flag_c},
    {FT_BOOL,   'd',  NULL,        &flag_d},
    {FT_BOOL,   '\0', "fifth",     &flag_e},
    {FT_STRING, 'f',  NULL,        &flag_f},
    {FT_STRING, 'g',  "seventh",   &flag_g},
};

int
main(int argc, char **argv)
{
  klee_make_symbolic(first_long, sizeof(first_long), "first_long");
  klee_make_symbolic(second_long, sizeof(second_long), "second_long");
  klee_make_symbolic(third_long, sizeof(third_long), "third_long");

  klee_assume(first_long[LONG_NAME_SIZE - 1] == '\0');
  klee_assume(second_long[LONG_NAME_SIZE - 1] == '\0');
  klee_assume(third_long[LONG_NAME_SIZE - 1] == '\0');

  flag_parse(flags, countof(flags), argc, argv, NULL);
  return 0;
}
