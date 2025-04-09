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
  int freq, rank;
} Dict;

typedef List(ssize) NumList;

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

int main() {
  Arena _scratch = new_arena(1 * GiB),
        perm = new_arena(1 * GiB);
  s8 f = read_file(NULL, _scratch, s8("./text.txt"));
  NumList tokens = {0};
  Dict *dict = NULL;

  for (int i = 0; i < f.len; i++) {
    s8 c = slice(f, i, 1);
    Dict *d = dict_upsert(&perm, &dict, c);
    list_push(&tokens, dict_to_token(d));
  }

  for (int i = 0; i < tokens.len - 1; i++) {

  }

  return 0;
}

