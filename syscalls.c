#include <sys/stat.h>
#include "hal.h"

void _init(void) {
}

void _fini(void) {
}

int _fstat(int fd, struct stat *st) {
  (void) fd, (void) st;
  return -1;
}

void *_sbrk(int incr) {
  extern char _end;
  static unsigned char *heap = NULL;
  unsigned char *prev_heap;
  if (heap == NULL) heap = (unsigned char *) &_end;
  prev_heap = heap;
  heap += incr;
  return prev_heap;
}

int _close(int fd) {
  (void) fd;
  return -1;
}

int _isatty(int fd) {
  (void) fd;
  return 1;
}

int _read(int fd, char *ptr, int len) {
  (void) fd, (void) ptr, (void) len;
  return -1;
}

int _lseek(int fd, int ptr, int dir) {
  (void) fd, (void) ptr, (void) dir;
  return 0;
}

int _write(int fd, char *ptr, int len) {
  if (fd == 1 || fd == 2) {
    uart_write_buf(USART1, ptr, (size_t) len);
    return len;
  }
  return -1;
}
