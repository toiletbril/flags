/* MIT License

   Copyright (c) 2024 toiletbril

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE. */

#if !defined FLAGS_H
#define FLAGS_H

#if defined __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

/* FT_BOOL does not require a value. It expects *value to be a pointer to bool
   variable and sets it to true of this flag is present. They can be combined,
   e.g -vAsn will enable -v -A -s -n.

   FT_STRING expects a value, and *value must be a pointer to char *. If a
   string flag's short name is 'k', long_name is "key", there is five ways to
   set it to "value", i.e:
     --key value
     --key=value
      -k value
      -k=value
      -kvalue
   This library also assumes that quotes with spaces are handled by the shell,
   i.e:
     --key="value value2"
     -k"value value2"

   This way, -vAsnkvalue will enable -v -A -s -n and set -k to "value". It will
   not parse letters after -k as flags, parsing as the value instead.

   After encountering two dashes (--), the rest of the input will be treated as
   arguments. */
typedef enum
{
  FT_BOOL,
  FT_STRING,
} flag_type_t;

/* As there is no memory allocation done, after calling flag_parse() *value will
   point to the read-only char arrays passed by the environment. long_name can
   be disabled with NULL, short_name can be disabled with '\0'.

   There can't be multiple flags with the same short_name or long_name. */
struct flag
{
  flag_type_t flag_type;
  char        short_name;
  const char *long_name;
  void       *value;
};

/* A function that will be called on error from parse_flags(). */
typedef void (*flag_err_callback)(const char *cause, const char *msg);

/* Mask to decipher flags and their values from actual arguments, to avoid
   allocations. For up to sizeof(flag_mask_t) * CHAR_BIT arguments. */
typedef unsigned long long flag_mask_t;

/* Returns a flag mask on success. Otherwise, calls err_callback() with
   appropriate arguments and returns 0. */
flag_mask_t flag_parse(struct flag *flags, size_t flag_count, int argc,
                       char **argv, flag_err_callback err_callback);

/* Returns true if an argument on argc_index is an actual argument and is not a
   flag or a flag's value from a flag mask. Returns false otherwise. */
bool flag_is_arg(flag_mask_t mask, int argv_index);

/* Get the number of arguments from a flag mask. */
size_t flag_arg_count(flag_mask_t mask);

/* End of header file. */
#if defined FLAGS_IMPLEMENTATION

#include <limits.h>
#include <string.h>

static bool
find_flag(struct flag *flags, size_t flag_count, char *flag_name, bool is_long,
          struct flag **result_flag, char **value_start)
{
  size_t longest_length = 0;

  *value_start = NULL;
  *result_flag = NULL;

  for (size_t i = 0; i < flag_count; ++i)
  {
    if (!is_long)
    {
      if (flags[i].short_name != '\0' && flags[i].short_name == *flag_name)
      {
        *result_flag = &flags[i];
        *value_start = flag_name + 1;
        return true;
      }
    }
    else
    {
      if (flags[i].long_name != NULL)
      {
        /* There might be flags that are prefixes of other flags. Go through all
           flags first and pick the longest match. */
        size_t flag_length = strlen(flags[i].long_name);

        if (flag_length > longest_length &&
            strncmp(flags[i].long_name, flag_name, flag_length) == 0)
        {
          *result_flag = &flags[i];
          *value_start = flag_name + flag_length;
          longest_length = flag_length;
        }
      }
    }
  }

  return longest_length > 0;
}

#define F__MASK_BITS (sizeof(flag_mask_t) * CHAR_BIT)

flag_mask_t
flag_parse(struct flag *flags, size_t flag_count, int argc, char **argv,
           flag_err_callback err_callback)
{
  if ((size_t) argc > F__MASK_BITS)
  {
    if (err_callback)
      err_callback("argc", "too many arguments");
    return 0;
  }

  if (flags == NULL || flag_count <= 0 || argc <= 0 || argv == NULL)
  {
    if (err_callback)
      err_callback("parse_flags()", "one or more arguments are invalid");
    return 0;
  }

  struct flag *prev_flag = NULL;
  bool next_arg_is_value = false, prev_is_long = false, ignore_rest = false;

  /* Store argument indexes as a bit mask. */
  flag_mask_t mask = 0;

  for (int i = 0; i < argc; ++i)
  {
    if (next_arg_is_value)
    {
      next_arg_is_value = false;
      prev_flag->value = argv[i];
      continue;
    }

    if (ignore_rest || argv[i][0] != '-')
    {
      mask |= (flag_mask_t) 1 << i;
      continue;
    }

    bool  is_long = false;
    char *flag_start;

    if (argv[i][1] != '-')
      flag_start = &argv[i][1];
    else
    {
      flag_start = &argv[i][2];
      is_long = true;
    }

    if (*flag_start == '\0')
    {
      if (is_long)
      {
        /* Skip the rest of the flags after '--'. */
        ignore_rest = true;
      }
      else
      {
        /* Treat '-' as an argument. */
        mask |= (flag_mask_t) 1 << i;
      }

      continue;
    }

    struct flag *flag;
    char        *value_start;

found_another_short_flag:
    if (find_flag(flags, flag_count, flag_start, is_long, &flag, &value_start))
    {
      switch (flag->flag_type)
      {
      case FT_BOOL: {
        *(bool *) flag->value = true;

        /* Check for combined flags, e.g -vAsn. */
        if (!is_long && *value_start != '\0')
        {
          ++flag_start;
          goto found_another_short_flag;
        }
      }
      break;
      case FT_STRING: {
        if (*value_start == '\0')
          next_arg_is_value = true;
        else
        {
          /* Check for a separator. Short flags do not require a separator
             between the flag and the value, but long flags do. Treat missing
             separator for long flags as an error. */
          if (*value_start == '=')
            ++value_start;
          else if (is_long)
            goto unknown_flag;

          flag->value = value_start;
        }
      }
      break;
      }
    }
    else
    {
unknown_flag:
      if (err_callback)
      {
        if (is_long)
          err_callback(flag_start, "unknown flag");
        else
        {
          char n[2];
          n[0] = *flag_start;
          n[1] = '\0';
          err_callback(n, "unknown flag");
        }
      }
      return 0;
    }

    prev_flag = flag;
    prev_is_long = is_long;
  }

  if (next_arg_is_value)
  {
    if (err_callback)
    {
      if (prev_is_long)
        err_callback(prev_flag->long_name, "no value provided");
      else
      {
        char n[2];
        n[0] = prev_flag->short_name;
        n[1] = '\0';
        err_callback(n, "no value provided");
      }
    }
    return 0;
  }

  return mask;
}

bool
flag_is_arg(flag_mask_t mask, int argv_index)
{
  if ((size_t) argv_index > F__MASK_BITS ||
      !(mask & (flag_mask_t) 1 << argv_index))
    return false;

  return true;
}

size_t
flag_arg_count(flag_mask_t mask)
{
#if defined __GNUC__ || defined __clang__
  return (size_t) __builtin_popcountll(mask);
#else
  size_t c = 0;
  for (size_t i = 0; i < F__MASK_BITS; ++i)
  {
    if (mask >> i & 1)
      ++c;
  }
  return c;
#endif
}
#endif /* FLAGS_IMPLEMENTATION */

#if defined __cplusplus
}
#endif

#endif /* FLAGS_H */
