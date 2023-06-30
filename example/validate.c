#include <stdio.h>
#include <err.h>
#include <reactor.h>

int main(int argc, char **argv)
{
  extern char *__progname;
  buffer_t buffer;
  value_t *value;
  const char *end;
  bool status;
  string_t string;

  if (argc != 2)
  {
    fprintf(stderr, "usage: %s FILE\n", __progname);
    exit(1);
  }

  buffer_construct(&buffer);
  status = buffer_loadz(&buffer, argv[1]);
  if (!status)
    err(1, "buffer_loadz");

  value = value_decode(buffer_base(&buffer), &end);
  if (value_is_undefined(value) || end != (char *) buffer_base(&buffer) + buffer_size(&buffer))
  {
    buffer_destruct(&buffer);
    value_release(value);
    printf("invalid\n");
    exit(1);
  }

  buffer_destruct(&buffer);
  string = value_encode(value, VALUE_ENCODE_PRETTY);
  value_release(value);
  printf("%s\n", string_base(string));
  string_release(string);
}
