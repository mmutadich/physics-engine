#include <assert.h>
#include <list.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct list {
  void **data;
  size_t size;
  size_t capacity;
  free_func_t freer;
} list_t;

list_t *list_init(size_t capacity, free_func_t freer) {
  assert(capacity >= 0);
  list_t *result = malloc(sizeof(list_t));
  assert(result);
  result->data = malloc(sizeof(void *) * capacity);
  assert(result->data);
  result->size = 0;
  result->capacity = capacity;
  result->freer = freer;
  return result;
}

void ensure_capacity(list_t *list) {
  if (list->capacity == 0) {
    list->capacity = 1;
    void **new_data = malloc(sizeof(void *) * list->capacity);
    free(list->data);
    list->data = new_data;
    assert(list->data);
  } else if (list->size >= list->capacity) {
    void **new_data = malloc(sizeof(void *) * list->capacity * 2);
    assert(new_data);
    for (size_t i = 0; i < list->capacity; i++) {
      new_data[i] = list->data[i];
    }
    list->capacity *= 2;
    free(list->data);
    list->data = new_data;
    assert(list->data);
  }
  assert(list->capacity > 0);
}

void list_add(list_t *list, void *item) {
  assert(item);
  ensure_capacity(list);
  list->data[list->size] = item;
  list->size++;
}

void *list_get(list_t *list, size_t index) {
  assert(index < list->size);
  assert(index >= 0);
  return list->data[index];
}

size_t list_size(list_t *list) {
  assert(list);
  return list->size;
}

void *list_remove(list_t *list, size_t index) {
  assert(index < list->size && index >= 0);
  void *answer = list->data[index];
  for (size_t i = index + 1; i < list->size; i++) {
    list->data[i - 1] = list->data[i];
  }
  list->size--;
  return answer;
}

void list_free(void *to_free) {
  list_t *list = (list_t *)to_free;
  if (list->freer != NULL) {
    for (size_t i = 0; i < list->size; i++) {
      if (list->data[i] != NULL)
        list->freer(list->data[i]);
    }
  }
  free(list->data);
  assert(list);
  free(list);
}

int list_equal(list_t *list1, list_t *list2) {
  if (list_size(list1) != list_size(list2)) {
    return 0;
  }
  for (size_t i = 0; i < list_size(list1); i++) {
    if (list_get(list1, i) != list_get(list2, i)) {
      return 0;
    }
  }
  return 1;
}

void *list_remove_back(list_t *list) {
  return list_remove(list, list_size(list) - 1);
}

bool list_contains(list_t *list, void *item) {
  if (list == NULL)
    return false;
  for (size_t i = 0; i < list->size; i++) {
    if (list->data[i] == item)
      return true;
  }
  return false;
}