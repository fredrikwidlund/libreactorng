#include <stdio.h>
#include <value.h>

int main()
{
  buffer_t b;

  buffer_construct(&b);
  buffer_loadz(&b, "/etc/services");
  buffer_save(&b, "/tmp/services");
  printf("%s", (char *) buffer_base(&b));
  buffer_destruct(&b);
}
