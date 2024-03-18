#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define FLAGS_IMPLEMENTATION
#include "flags.h"

#define countof(arr) (sizeof(arr) / sizeof(*arr))

bool  flag_a = false;
char *flag_b = NULL;
bool  flag_c = false;
char *flag_d = NULL;
bool  flag_e = false;

struct flag flags[] = {
    {FT_BOOL,   'a', "first",     &flag_a},
    {FT_STRING, 'b', "seconded",  &flag_b},
    {FT_BOOL,   'c', "third",     &flag_c},
    {FT_STRING, 'd', "second",    &flag_d},
    {FT_BOOL,   'e', "something", &flag_e},
};

void
show_help(void)
{
  printf("example [-ace] [-b=<value>] [-d=<value>] [arg1, arg2, ...]\n"
         "  -a, --first\n"
         "  -b, --seconded=<...>\n"
         "  -c, --third\n"
         "  -d, --second=<...>\n"
         "  -e, --something\n");
  exit(0);
}

void
handle_error(const char *cause, const char *msg)
{
  printf("%s: %s\n", cause, msg);
  exit(1);
}

int
main(int argc, char **argv)
{
  flag_mask_t mask =
      flag_parse(flags, countof(flags), argc, argv, handle_error);
  size_t arg_count = flag_arg_count(mask);

  printf("arg count: %zu\n", arg_count);
  printf("mask:\n");
  for (size_t i = 0; i < sizeof(flag_mask_t) * CHAR_BIT; ++i)
    printf("%d", (int) ((mask >> i) & 1));
  printf("\n");

  /* Only program name. */
  if (argc == 1)
    show_help();

  for (size_t i = 0; i < countof(flags); ++i)
  {
    switch (flags[i].flag_type)
    {
    case FT_BOOL:
      printf("-%c, --%-10s is %s\n", flags[i].short_name, flags[i].long_name,
             *(bool *) flags[i].value ? "true" : "false");
      break;
    case FT_STRING:
      printf("-%c, --%-10s is %s\n", flags[i].short_name, flags[i].long_name,
             *(void **) flags[i].value ? (char *) flags[i].value : "empty");
      break;
    }
  }

  if (mask != 0)
  {
    printf("arguments:");
    for (int i = 0; i < argc; ++i)
    {
      if (flag_is_arg(mask, i))
        printf(" %s", argv[i]);
    }
    printf("\n");
  }

  return 0;
}
