#ifndef DS_H
#define DS_H

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
typedef ptrdiff_t ssize;

#define arrlen(arr) (ssize) (sizeof(arr)/sizeof(arr[0]))
#define slice(l, i, _len) { .buf = &(l).buf[i], .len = (_len), }
#define strify(c) #c
#define KiB (ssize) (1 << 10)
#define MiB (ssize) (1 << 20)
#define GiB (ssize) (1 << 30)

typedef struct {
  u8 *buf;
  ssize cap, len;
} Arena;

typedef struct {
  u8 *buf;
  ssize len;
} s8;

#define List(T) struct { Arena *a; T *buf; ssize cap, len; }
#define new_list(arena, T) { .a = (arena), .buf = new((arena), T, 0), }
// #define list_push(l, x)
//   do {
//     assert(&(l)->a->buf[(l)->a->len] == (u8 *) &(l)->buf[(l)->len]);
//     (l)->len += 1;
//     (void) new((l)->a, typeof(*(l)->buf), 1);
//     (l)->buf[(l)->len - 1] = (x);
//   } while (0)
// // this is needed because of the assert in list_push
// #define list_set_len(l, nlen)
// do {
//   typeof(nlen) llen = (nlen); /* to avoid recomputing the value */
//   assert((l)->len >= llen);
//   (l)->a->len -= (l)->len - llen;
//   (l)->len = llen;
// } while (0);
#define swap(x1, x2)         \
  do {                       \
    typeof((x1)) tmp = (x1); \
    (x1) = (x2);             \
    (x2) = tmp;              \
  } while (0);

Arena new_arena(ssize cap);
#define new(a, T, c) arena_alloc(a, sizeof(T), _Alignof(T), c)
void *arena_alloc(Arena *a, ssize size, ssize align, ssize count);

#define s8(lit) (s8){ .buf = (u8 *) lit, .len = arrlen(lit) - 1, }

s8 new_s8(Arena *perm, ssize len);
s8 s8_copy(Arena *perm, s8 s);
void s8_modcat(Arena *perm, s8 *a, s8 b);
s8 s8_newcat(Arena *perm, s8 a, s8 b);
bool s8_equals(s8 a, s8 b);
s8 s8_replace_all(Arena *perm, s8 s, s8 old, s8 new);
void s8_print(s8 s);
void s8_fprint(FILE *restrict stream, s8 s);
u64 s8_hash(s8 s);
s8 s8_errno();
s8 s8_err(s8 s);

s8 open_and_read(Arena *perm, s8 p,
           FILE *(*open)(const char *p, const char *m),
           int (*close)(FILE *stream));
s8 read_file(Arena *perm, Arena scratch, s8 p);
s8 run(Arena *perm, s8 p);

#endif // DS_H

#ifdef DS_IMPL
#ifndef DS_IMPL_GUARD
#define DS_IMPL_GUARD

Arena new_arena(ssize cap) {
  return (Arena) { .buf = malloc(cap), .cap = cap, };
}

void *arena_alloc(Arena *a, ssize size, ssize align, ssize count) {
  assert(align >= 0);
  assert(size >= 0);
  assert(count >= 0);

  if (a == NULL) return calloc(count, size);

  ssize pad = -(uintptr_t)(&a->buf[a->len]) & (align - 1);
  ssize available = a->cap - a->len - pad;

  assert(available >= 0);
  assert(count <= available/size);

  void *p = &a->buf[a->len] + pad;
  a->len += pad + count * size;
  return memset(p, 0, count * size);
}

s8 new_s8(Arena *perm, ssize len) {
  return (s8) { .buf = new(perm, u8, len), .len = len, };
}

s8 s8_copy(Arena *perm, s8 s) {
  s8 copy = s;
  copy.buf = new(perm, u8, copy.len);
  if (s.buf) memmove(copy.buf, s.buf, copy.len * sizeof(u8));
  return copy;
}

void s8_modcat(Arena *perm, s8 *a, s8 b) {
  assert(&a->buf[a->len] == &perm->buf[perm->len]);
  s8_copy(perm, b);
  a->len += b.len;
}

s8 s8_newcat(Arena *perm, s8 a, s8 b) {
  s8 c = s8_copy(perm, a);
  s8_modcat(perm, &c, b);
  return c;
}

bool s8_equals(s8 a, s8 b) {
  if (a.len != b.len) return false;
  if (a.buf == NULL || b.buf == NULL) return false;
  for (int i = 0; i < a.len; i++) {
    if (a.buf[i] != b.buf[i]) return false;
  }
  return true;
}

s8 s8_replace_all(Arena *perm, s8 s, s8 old, s8 new) {
  s8 ret = new_s8(perm, 0);
  for (ssize i = 0; i < s.len; i++) {
    s8 cat = {0};
    s8 cmp = slice(s, i, old.len);
    if (i + old.len <= s.len && s8_equals(old, cmp)) cat = new;
    else cat = (s8) slice(s, i, 1);
    s8_modcat(perm, &ret, cat);
  }
  return ret;
}

void s8_print(s8 s) {
  for (ssize i = 0; i < s.len; i++) {
    printf("%c", s.buf[i]);
  }
}

void s8_fprint(FILE *restrict stream, s8 s) {
  for (ssize i = 0; i < s.len; i++) {
    fprintf(stream, "%c", s.buf[i]);
  }
}

u64 s8_hash(s8 s) {
  u64 h = 0x100;
  for (ssize i = 0; i < s.len; i++) {
    h ^= s.buf[i];
    h *= 1111111111111111111u;
  }
  return h;
}

s8 s8_errno() {
  char *err = strerror(errno);
  return (s8) { .buf = (u8 *) err, .len = -1 * strlen(err), };
}

s8 s8_err(s8 s) {
  s.len *= -1;
  return s;
}

s8 open_and_read(Arena *perm, s8 p,
           FILE *(*open)(const char *p, const char *m),
           int (*close)(FILE *stream)) {
  assert(perm != NULL);

  FILE *fp = NULL;
  {
    Arena scratch = *perm;

    s8 n = s8_newcat(&scratch, p, s8("\0"));

    fp = open((char *) n.buf, "r");
    if (fp == NULL) {
      perror("open");
      fprintf(stderr, "file: ");
      s8_fprint(stderr, p);
      fprintf(stderr, "\n");
      exit(1);
    }
  }

  s8 ret = new_s8(perm, 0);

  char c = 0;
  while ((c = fgetc(fp)) != EOF) {
    char *n = new(perm, u8, 1);
    *n = c;
    ret.len += 1;
  }

  close(fp);

  return ret;
}

s8 read_file(Arena *perm, Arena scratch, s8 p) {
  s8 ret = {0};

  FILE *fp = NULL;
  {
    s8 n = s8_newcat(&scratch, p, s8("\0"));
    fp = fopen((char *) n.buf, "r");
  }
  if (fp == NULL) return s8_errno();

  if (fseek(fp, 0, SEEK_END) < 0) { ret = s8_errno(); goto end; }

  ret.len = ftell(fp);
  if (ret.len < 0) { ret = s8_errno(); goto end; }

  if (fseek(fp, 0, SEEK_SET) < 0) { ret = s8_errno(); goto end; }

  ret = new_s8(perm, ret.len);
  fread(ret.buf, 1, ret.len, fp);
  if (ferror(fp)) { ret = s8_err(s8("fread")); goto end; }

end:
  fclose(fp);

  return ret;
}

// s8 read_file(Arena *perm, s8 p) {
//   s8 ret = open_and_read(perm, p, fopen, fclose);
//   return ret;
// }

s8 run(Arena *perm, s8 p) {
  s8 ret = open_and_read(perm, p, popen, fclose);
  return ret;
}

#endif // DS_IMPL_GUARD
#endif // DS_IMPL
