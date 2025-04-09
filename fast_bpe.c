#define DS_IMPL
#include "ds.h"

#define list_push(l, x)                                         \
do {                                                            \
  if ((l)->len >= (l)->cap) {                                   \
    (l)->cap = (l)->cap == 0 ? 10 : (l)->cap * 2;               \
    (l)->buf = realloc((l)->buf, (l)->cap * sizeof(*(l)->buf)); \
    assert((l)->buf != NULL && "Realloc failed");               \
  }                                                             \
  (l)->buf[(l)->len++] = (x);                                   \
} while (0);

typedef struct Dict {
  struct Dict *child[4];
  s8 key;
  int n, i; // for DictRanking
} Dict;

typedef List(s8) S8List;
typedef List(ssize) NumList;

typedef struct {
  Dict **buf; // array
  ssize cap, len;
  NumList marks;
  Dict *map;
} DictRanking;

Dict *dict_upsert(Arena *perm, Dict **d, s8 key) {
  for (u64 h = s8_hash(key); *d; h <<= 2) {
    if (s8_equals(key, (*d)->key)) return *d;
    d = &(*d)->child[h>>62];
  }

  if (!perm) return NULL;

  *d = new(perm, Dict, 1);
  (*d)->key = s8_copy(perm, key);
  return *d;
}

Dict *token_to_dict(ssize t) {
  return (Dict *) t;
}

ssize dict_to_token(Dict *d) {
  return (ssize) d;
}

s8 s8_token_pair(Arena *perm, NumList tokens, int i) {
  assert(i < tokens.len);
  s8 d1 = token_to_dict(tokens.buf[i])->key,
     d2 = token_to_dict(tokens.buf[i + 1])->key;
  return s8_newcat(perm, d1, d2);
}

void s8_print_quoted(s8 s) {
  printf("\"");
  for (int i = 0; i < s.len; i++) {
    u8 c = s.buf[i];
    switch (c) {
    case '\0': printf("\\0"); break;
    case '\a': printf("\\a"); break;
    case '\b': printf("\\b"); break;
    case '\t': printf("\\t"); break;
    case '\n': printf("\\n"); break;
    case '\v': printf("\\v"); break;
    case '\f': printf("\\f"); break;
    case '\r': printf("\\r"); break;
    case '\\': printf("\\"); break;
    case '"': printf("\""); break;
    default:
      if (c < ' ' || c > '~') printf("\\%xd\n", c);
      else printf("%c", c);
    }
  }
  printf("\"");
}

void increment_dict_rank(DictRanking dr, int i) {
  Dict *d = dr.buf[i];
  if (d->n == dr.marks.len) list_push(&dr.marks, 0);
  dr.buf[i] = dr.marks.buf[d->n - 1];
  dr.buf[dr.marks.buf[d->n - 1]] = d;
  dr.marks.buf[d->n - 1] += 1;
}

int main() {
  Arena _scratch = new_arena(1 * GiB),
        perm = new_arena(1 * GiB);
  NumList tokens = {0};
  Dict *dict = NULL;
  s8 f = read_file(NULL, _scratch, s8("./text.txt"));

  for (int i = 0; i < f.len; i++) {
    s8 c = slice(f, i, 1);
    Dict *d = dict_upsert(&perm, &dict, c);
    list_push(&tokens, dict_to_token(d));
  }

  DictRanking dr = {0};
  list_push(&dr.marks, 0);

  for (int i = 0; i < tokens.len - 1; i++) {
    s8 pair = s8_token_pair(&perm, tokens, i);
    Dict *d = dict_upsert(&perm, &dr.map, pair);
    if (d->n == 0) {
      d->n = 1;
      d->i = dr.len;
      list_push(&dr, d);
    }
  }

  return 0;
}
