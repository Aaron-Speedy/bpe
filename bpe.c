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
  ssize val;
} Dict;

typedef List(s8) S8List;
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

s8 s8_token_pair(Arena *perm, NumList ts, int i) {
  assert(i < ts.len);
  s8 d1 = token_to_dict(ts.buf[i])->key,
     d2 = token_to_dict(ts.buf[i + 1])->key;
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

int main() {
  Arena _scratch = new_arena(1 * GiB),
        perm = new_arena(1 * GiB);
  NumList old_ts = {0}, new_ts = {0}; // tokens
  Dict *dict = NULL;
  s8 f = read_file(NULL, _scratch, s8("./text.txt"));

  for (int i = 0; i < f.len; i++) {
    s8 c = slice(f, i, 1);
    Dict *d = dict_upsert(&perm, &dict, c);
    list_push(&new_ts, dict_to_token(d));
  }
  swap(old_ts, new_ts);
  new_ts.len = 0;

  for (int iteration = 0; ; iteration++) {
    s8 most_def = {0};
    int most_freq = 1; // We want pairs that occur more than once
    ssize most_token = 0;
    {
      Arena scratch = _scratch;
      Dict *freqs = NULL;

      for (int i = 0; i < old_ts.len - 1; i++) {
        // printf("%ld\n", old_ts.buf[i] - (ssize) perm.buf);
        s8 def = s8_token_pair(&scratch, old_ts, i);
        Dict *d = dict_upsert(&scratch, &freqs, def);
        d->val += 1;
        if (d->val > most_freq) {
          most_def = def;
          most_freq = d->val;
        }
      }

      if (most_freq == 1) break;
      most_def = s8_copy(&perm, most_def);
    }

    most_token = dict_to_token(dict_upsert(&perm, &dict, most_def));

    for (int i = 0; i < old_ts.len; i++) {
      Arena scratch = _scratch;
      s8 def = {0};
      if (i == old_ts.len - 1) def = token_to_dict(old_ts.buf[i])->key;
      else def = s8_token_pair(&scratch, old_ts, i);

      ssize t = 0;
      if (s8_equals(def, most_def)) {
        t = most_token;
        i += 1;
      } else t = old_ts.buf[i];

      list_push(&new_ts, t);
    }

    swap(old_ts, new_ts);
    new_ts.len = 0;
  }

  for (int i = 0; i < old_ts.len; i++) {
    s8 s = token_to_dict(old_ts.buf[i])->key;
    // s8_print_quoted(s);
    // printf("\n");
    s8_print(s);
    printf("|");
  }

  return 0;
}
