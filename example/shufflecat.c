#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <reactor.h>

struct file
{
  const char *name;
  int fd;
  char buffer[1];
  size_t read_offset;
  size_t *write_offset;
};

static void read_callback(reactor_event_t *);

static void write_callback(reactor_event_t *event)
{
  struct file *file = event->state;

  reactor_read(read_callback, file, file->fd, file->buffer, sizeof file->buffer, file->read_offset);
}

static void read_callback(reactor_event_t *event)
{
  struct file *file = event->state;
  int result = event->data;

  if (result > 0)
  {
    file->read_offset += result;
    reactor_write(write_callback, file, STDOUT_FILENO, file->buffer, result, *file->write_offset);
    *file->write_offset += result;
  }
  else
    reactor_close(NULL, NULL, file->fd);
}

static void open_callback(reactor_event_t *event)
{
  struct file *file = event->state;
  int result = event->data;

  if (result >= 0)
  {
    file->fd = result;
    reactor_read(read_callback, file, file->fd, file->buffer, sizeof file->buffer, file->read_offset);
  }
  else
    fprintf(stderr, "error: %s: %s\n", file->name, strerror(-result));
}

int main(int argc, char **argv)
{
  int i;
  list_t files;
  struct file *file;
  size_t write_offset = 0;

  list_construct(&files);
  reactor_construct();
  for (i = 1; i < argc; i++)
  {
    file = list_push_back(&files, NULL, sizeof *file);
    file->name = argv[i];
    file->write_offset = &write_offset;
    reactor_openat(open_callback, file, AT_FDCWD, file->name, O_RDONLY, 0);
  }
  reactor_loop();
  reactor_destruct();
  list_destruct(&files, NULL);
}
