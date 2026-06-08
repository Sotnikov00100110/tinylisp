#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// структури, буфер, токени, а також функції до них
#define BUFFER 2048
#define MAX_TOKENS 128
static char buffer[BUFFER];

typedef enum { VAL, LIST, SYMBOL } TypeLisp;

typedef struct {
  TypeLisp type;
  union {
    long number;
    char *symbol;
    struct ConsCells *list;
  };
} LispVal;

struct ConsCells {
  LispVal *head;
  struct ConsCells *tail;
};

typedef struct Env {
  char *name;
  LispVal *value;
  struct Env *next;
} Env;

struct Env *globalEnv = NULL;

LispVal *env_get(char *key);
void env_define(char *key, LispVal *val);
LispVal *EVAL(LispVal *expr);
LispVal *number_obj(long n);
LispVal *symb_obj(char *s);
LispVal *cons(LispVal *head, LispVal *tail_list);
LispVal *read_list(char **tokens, int *pos, int count);
LispVal *read_expr(char **tokens, int *pos, int count);
void lisp_print(LispVal *obj);
char **tokenize(char *input, int *count);

// повертаємо останню ноду поки curr != NULL
LispVal *env_get(char *key) {
  struct Env *curr = globalEnv;
  while (curr) {
    if (strcmp(curr->name, key) == 0)
      return curr->value;
    curr = curr->next;
  }
  return NULL;
}

// глобально визначаємо структурі нову ноду
void env_define(char *key, LispVal *val) {
  struct Env *curr = globalEnv;
  while (curr) {
    if (strcmp(curr->name, key) == 0) {
      curr->value = val; // Оновлюємо, якщо існує
      return;
    }
    curr = curr->next;
  }
  // якщо не знайшли то додаємо новий вузол
  struct Env *new_node = malloc(sizeof(struct Env));
  new_node->name = strdup(key);
  new_node->value = val;
  new_node->next = globalEnv;
  globalEnv = new_node;
}

// читамємо вираз і обробляємо його
LispVal *read_expr(char **tokens, int *pos, int count) {
  if (*pos >= count)
    return NULL;

  char *t = tokens[*pos];
  (*pos)++;

  if (strcmp(t, "(") == 0) {
    return read_list(tokens, pos, count);
  }

  if (isdigit(t[0]) || (t[0] == '-' && isdigit(t[1]))) {
    return number_obj(atoll(t));
  }

  return symb_obj(t);
}

// читаємо сам список і додаємо токени в нього
LispVal *read_list(char **tokens, int *pos, int count) {
  LispVal *root = NULL;
  LispVal *last = NULL;

  while (*pos < count && strcmp(tokens[*pos], ")") != 0) {
    LispVal *item = read_expr(tokens, pos, count);

    LispVal *cell = cons(item, NULL);
    if (root == NULL) {
      root = cell;
    } else {
      last->list->tail = cell->list;
    }
    last = cell;
  }

  if (*pos < count)
    (*pos)++; // Пропускаємо ")"
  return root;
}

// в залежності від константи показує printf() чи це символ/число/список
void lisp_print(LispVal *obj) {
  if (obj == NULL) {
    printf("nil");
    return;
  }

  switch (obj->type) {
  case SYMBOL: {
    printf("%s", obj->symbol);
    break;
  }
  case VAL: {
    printf("%ld", obj->number);
    break;
  }
  case LIST: {
    printf("(");
    struct ConsCells *curr = obj->list;
    while (curr) {
      lisp_print(curr->head);
      if (curr->tail)
        printf(" ");
      curr = curr->tail;
    }
    printf(")");
    break;
  }
  default: {
    printf("Unknown type\n");
    break;
  }
  }
}

LispVal *cons(LispVal *head, LispVal *tail_list) {
  LispVal *res = (LispVal *)malloc(sizeof(LispVal));

  res->type = LIST;
  res->list = malloc(sizeof(struct ConsCells));
  res->list->head = head;
  res->list->tail = (tail_list) ? tail_list->list : NULL;

  return res;
}

// чи це число ?
LispVal *number_obj(long n) {
  LispVal *new_n = (LispVal *)malloc(sizeof(LispVal));
  if (!new_n)
    return NULL;

  new_n->type = VAL;
  new_n->number = n;
  return new_n;
}

// чи це символ ?
LispVal *symb_obj(char *s) {
  LispVal *new_s = (LispVal *)malloc(sizeof(LispVal));
  if (!new_s)
    return NULL;
  new_s->type = SYMBOL;
  new_s->symbol = strdup(s);
  return new_s;
}

// токенізуємо вираз, обробляємо його і повертаємо
char **tokenize(char *input, int *count) {
  // Масив пам'яті на токени
  char **tokens = malloc(MAX_TOKENS * sizeof(char *));
  int n = strlen(input);
  int t_idx = 0; // Індекс токена
  int i = 0;     // Позиція в рядку

  while (i < n && t_idx < MAX_TOKENS) {
    // Обробка пробілів, дужок, і символів(розбиття на токени)
    if (isspace(input[i])) {
      i++;
      continue;
    }

    if (input[i] == '(' || input[i] == ')' || input[i] == '[' ||
        input[i] == ']' || input[i] == '{' || input[i] == '}') {

      tokens[t_idx] = malloc(2 * sizeof(char));
      tokens[t_idx][0] = input[i];
      tokens[t_idx][1] = '\0';
      t_idx++;
      i++;
      continue;
    }

    int start = i;
    while (i < n && !isspace(input[i]) && input[i] != '(' && input[i] != ')' &&
           input[i] != '[' && input[i] != ']' && input[i] != '{' &&
           input[i] != '}') {
      i++;
    }

    int len = i - start;
    tokens[t_idx] = malloc((len + 1) * sizeof(char));
    strncpy(tokens[t_idx], &input[start], len);
    tokens[t_idx][len] = '\0';
    t_idx++;
  }

  *count = t_idx;
  return tokens;
}

// основна функція на обробку і розрахунок виразу
LispVal *EVAL(LispVal *expr) {
  if (!expr)
    return NULL;

  if (expr->type == SYMBOL) {
    LispVal *val = env_get(expr->symbol);
    if (val)
      return val;
  }

  if (expr->type == VAL)
    return expr;

  if (expr->type == LIST) {
    LispVal *op = expr->list->head;
    struct ConsCells *args = expr->list->tail;
    // обробляємо вираз як функційю по типу (def x 10) (def y 10) (def + x y) =
    // 20
    if (op->type == SYMBOL && strcmp(op->symbol, "def") == 0) {
      char *s = args->head->symbol;
      LispVal *val = EVAL(args->tail->head);
      env_define(s, val);

      return val;
    }
    // обробляємо сипволи як математичний вираз
    if (op->type == SYMBOL && strcmp(op->symbol, "+") == 0) {
      long sum = 0;
      struct ConsCells *curr = args;

      while (curr) {
        LispVal *res = EVAL(curr->head);
        if (res->type == VAL) {
          sum += res->number;
        }
        curr = curr->tail;
      }

      return number_obj(sum);
    }

    if (op->type == SYMBOL && strcmp(op->symbol, "*") == 0) {
      long prod = 1;
      struct ConsCells *curr = args;

      while (curr) {
        LispVal *res = EVAL(curr->head);
        if (res->type == VAL) {
          prod *= res->number;
        }
        curr = curr->tail;
      }

      return number_obj(prod);
    }

    if (op->type == SYMBOL && strcmp(op->symbol, "-") == 0) {
      if (!args)
        return number_obj(0);

      LispVal *first = EVAL(args->head);
      long res_val = (first->type == VAL) ? first->number : 0;

      struct ConsCells *curr = args->tail;

      while (curr) {
        LispVal *res = EVAL(curr->head);
        if (res->type == VAL)
          res_val -= res->number;
        curr = curr->tail;
      }

      return number_obj(res_val);
    }
    // перевіряємо список на наявність голови або аргумент на наявність голови
    if (op->type == SYMBOL && strcmp(op->symbol, "quote") == 0) {
      return args->head;
    }

    if (op->type == SYMBOL && strcmp(op->symbol, "car") == 0) {
      LispVal *arg = EVAL(args->head);

      if (!arg || arg->type != LIST)
        return NULL;

      return arg->list->head;
    }

    if (op->type == SYMBOL && strcmp(op->symbol, "cdr") == 0) {
      // обчислюємо аргумент
      LispVal *arg = EVAL(args->head);

      if (!arg || arg->type != LIST)
        return NULL;

      // тут новий обгортковий об'єкт
      LispVal *res = (LispVal *)malloc(sizeof(LispVal));
      res->type = LIST;

      // tail — це вказівник на наступну ланку (ConsCell)
      res->list = arg->list->tail;

      return res;
    }
  }

  return expr;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Need two argc...\n");
    exit(0);
  }

  printf("lisp v0.0.0.1\n");
  int count = 0;
  char **tokens = tokenize(argv[1], &count);
  int pos = 0;

  LispVal *fin = NULL;
  while (pos < count) {
    LispVal *expr = read_expr(tokens, &pos, count);
    LispVal *res = EVAL(expr);
    fin = res;
  }
  lisp_print(fin);
  printf("\n");

  for (int i = 0; i < count; i++)
    free(tokens[i]);
  free(tokens);

  return EXIT_SUCCESS;
}
