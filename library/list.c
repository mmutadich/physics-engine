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

/**
 * Allocates memory for a new list with space for the given number of elements.
 * The list is initially empty.
 * Asserts that the required memory was allocated.
 *
 * @param initial_size the number of list elements to allocate space for
 * @return a pointer to the newly allocated list
 */
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

/**
 * Updates the memory allocated to data
 * Updated the capacity attribute of list
 */
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

/**
 * Ensures capacity and adds an item to the back of a list
 * Asserts that the item exists
 * Adds one to the size attribute of list
 *
 * @param list the list to be added to
 * @param item the item to be added
 */
void list_add(list_t *list, void *item) {
  assert(item);
  ensure_capacity(list);
  list->data[list->size] = item;
  list->size++;
}

/**
 * Returns the item at a given index
 * Asserts that the index is within range
 *
 * @param list
 * @param index you want the item from
 */
void *list_get(list_t *list, size_t index) {
  assert(index < list->size);
  assert(index >= 0);
  return list->data[index];
}

/**
 * Returns the amount of items in the list
 *
 * @param list
 * @return size_t size of list
 */
/*return the amount of items in the list*/
size_t list_size(list_t *list) {
  assert(list);
  return list->size;
}

void list_get_freer(list_t *list) {
  printf("freer: %s\n", list->freer);
}

/**
 * Removes the item from that index and shifts left
 * Assert that the index is within range
 *
 * @param list the list to be added to
 * @param item the item to be added
 * @return the item that was removed from the list
 */
void *list_remove(list_t *list, size_t index) {
  assert(index < list->size && index >= 0);
  void *answer = list->data[index];
  for (size_t i = index + 1; i < list->size; i++) {
    list->data[i - 1] = list->data[i];
  }
  list->size--;
  return answer;
}

void list_free(list_t *list) {
  // convert freer to pointer
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

/**
 * Returns true if the two lists have the same elements
 *
 * @param list first list for comparison
 * @param list second list for comparison
 * @return int
 */
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