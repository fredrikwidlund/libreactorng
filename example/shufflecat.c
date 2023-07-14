#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <reactor.h>

struct state
{
  list_t files;
  size_t output_offset;
};

struct file
{
  struct state *state;
  const char *name;
  int fd;
  uint8_t byte;
  size_t input_offset;
};

static void file_read(reactor_event_t *);

static void file_write(reactor_event_t *event)
{
  struct file *file = event->state;

  reactor_read(file_read, file, file->fd, &file->byte, 1, file->input_offset);
}

static void file_read(reactor_event_t *event)
{
  struct file *file = event->state;
  int result = event->data;

  if (result <= 0)
  {
    reactor_close(NULL, NULL, file->fd);
    return;
  }

  file->input_offset++;
  reactor_write(file_write, file, STDOUT_FILENO, &file->byte, 1, file->state->output_offset);
  file->state->output_offset++;
}

static void file_open(reactor_event_t *event)
{
  struct file *file = event->state;
  int result = event->data;

  if (result <= 0)
  {
    fprintf(stderr, "error: open %s: %s\n", file->name, strerror(-result));
    return;
  }

  file->fd = result;
  reactor_read(file_read, file, file->fd, &file->byte, 1, file->input_offset);
}

void add_file(struct state *state, char *name)
{
  struct file *file;

  file = list_push_back(&state->files, NULL, sizeof *file);
  file->state = state;
  file->name = name;
  file->input_offset = 0;
  reactor_openat(file_open, file, AT_FDCWD, file->name, O_RDONLY, 0);
}

int main(int argc, char **argv)
{
  struct state state = {0};
  int i;

  list_construct(&state.files);
  reactor_construct();
  for (i = 1; i < argc; i++)
    add_file(&state, argv[i]);
  reactor_loop();
  reactor_destruct();
  list_destruct(&state.files, NULL);
}
