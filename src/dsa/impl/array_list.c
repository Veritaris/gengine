//
// Created by Георгий Имешкенов on 22.01.2024.
//
#include <string.h>

#include "array_list.h"

#ifndef ARRAYLIST_H_INCLUDED

#endif

static void
IndexError(ArrayList_t *self, size_t index) {
    printf(
        "[error] IndexError: list index (index=%lu) out of range [0, %lu]\n",
        index,
        self->capacity
    );
}

/*
 * Prints ArrayList as array of ints and NULLs if void pointer met
 */
__attribute__((unused)) static void
repr(ArrayList_t *self) {
    putchar('[');
    for (int i = 0; i < self->_size; i++) {
        if (self->data[i] != NULL) {
            printf("%d, ", *(int *) self->data[i]);
        } else {
            printf("NULL, ");
        }
    }

    if (self->_size > 0) putchar(8);
    putchar(']');
    putchar('\n');
}

/*
 * Resizes ArrayList from capacity to 2 * capacity
 */
static void
resize(ArrayList_t *self) {
#ifdef STRICT_ACCESS_CHECK
    any tmp;
    cmalloc_safe(tmp, self->capacity * sizeof(size_t) * 2)
    memcpy(tmp, self->data, self->capacity * sizeof(size_t));
    if (self->data == NULL) {
        printf("unable to reallocate memory for ArrayList\n");
        free(tmp);
        free_al(self);
        exit(-1);
    }
#else
    self->data = realloc(self->data, self->capacity * sizeof(size_t) * 2);
    if (self->data == NULL) {
        printf("unable to reallocate memory for ArrayList\n");
        exit(-1);
    }
#endif
    free(self->data);
    self->data = tmp;
    self->capacity *= 2;
}

/*
 * Shifts left all values at right of list[start] so all values from list[start] to list[start+len]
 * are lost but list[start+len]
 * Example:
 * ```
 *  ArrayList_t *list = ArrayList(NULL);
    int idata[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    for (int i = 0; i < 10; i++) {
        list->push(list, &idata[i]);
    }

    repr(list);
    shl(list, 3, 2);
    repr(list);
 * ```
 * [0, 1, 2, 3, 4, 5, 6, 7, 8, 9,]
 * [0, 1, 2, 5, 6, 7, 8, 9,]
 */
static void
shl(ArrayList_t *self, size_t start, size_t len) {
    memmove(
        self->data + start,
        self->data + start + len,
        (self->_size - (start + len)) * sizeof(size_t)
    );
    self->_size -= len;
}

/*
 * Shifts right all values at right of list[start]. All empty places are filled with NULL pointers so accessing them
 * may result to unsafe void pointer dereference
 * Example:
 * ```
    ArrayList_t *list = ArrayList(NULL);
    int idata[10] = {0, 1, 2, 5, 6, 7, 8, 9};

    for (int i = 0; i < 10; i++) {
        list->push(list, &idata[i]);
    }

    repr(list);
    shr(list, 3, 2);
    repr(list);
 * ```
 * Output:
 * [0, 1, 2, 5, 6, 7, 8, 9,]
 * [0, 1, 2, NULL, NULL, 5, 6, 9, 8, 9,]
 */
static void
shr(ArrayList_t *self, size_t start, size_t len) {
    if (self->_size + len + 1 >= self->capacity) {
        resize(self);
    }

    memmove(
        self->data + start + len,
        self->data + start,
        (self->_size - start) * sizeof(size_t)
    );

    for (size_t i = start; i < start + len; i++) {
        self->data[i] = NULL;
    }

    self->_size += 1;
}

/*
 * Set element at `index` to `element`
 */
static void *
set(ArrayList_t *self, size_t index, void *element) {
    if (index >= self->_size) {
        IndexError(self, index);
#ifdef STRICT_ACCESS_CHECK
        exit(-1);
#else
        return NULL;
#endif
    }
    return self->data[index] = element;
}

/*
 * Inserts `element` at `index` with `shr(self, index, 1)` - moving all elements right from `index` by 1
 */
static void *
insert(ArrayList_t *self, size_t index, void *element) {
    if (index >= self->capacity) {
        IndexError(self, index);
#ifdef STRICT_ACCESS_CHECK
        exit(-1);
#else
        return NULL;
#endif
    }
    if (index < self->_size)
    shr(self, index, 1);
    self->data[index] = element;

    return element;
}

/*
 * Return element at `index`
 */
static void *
get(ArrayList_t *self, size_t index) {
    if (index >= self->capacity) {
        IndexError(self, index);
#ifdef STRICT_ACCESS_CHECK
        exit(-1);
#else
        return NULL;
#endif
    }
    if (index >= self->_size) {
        return NULL;
    }
    return self->data[index];
}

/*
 * Removes element at `index`
 */
static void *
removeAt(ArrayList_t *self, size_t index) {
    if (index >= self->_size) {
        IndexError(self, index);
#ifdef STRICT_ACCESS_CHECK
        exit(-1);
#else
        return NULL;
#endif
    }
    void *elem = self->data[index];
    shl(self, index, 1);

    return elem;
}

/*
 * Push `element` at the end of ArrayList. Resizes array if 1 or 0 places left for new elements
 */
static void *
push(ArrayList_t *self, void *element) {
    if (self->_size + 1 >= self->capacity) {
        resize(self);
    }

    self->data[self->_size++] = element;

    return element;
}

/*
 * Pop (get last) element from the end of the ArrayList. Does not remove element from list thus it can still
 * be accessed via internal `self->data[self->_size]`, instead it moves last element "pointer" by -1
 */
static void *
pop(ArrayList_t *self) {
    return self->data[self->_size--];
}

/*
 * Return index of first element in ArrayList that equals to `element`, -1 otherwise
 */
static ssize_t
indexOf(ArrayList_t *self, void *element) {
    for (int i = 0; i < self->_size;) {
        if (self->data[i++] == element) {
            return i;
        }
    }
    return -1;
}

/*
 * Return number of elements in ArrayList
 */
static size_t
size(ArrayList_t *self) {
    return self->_size;
}

/*
 * Clean ArrayList from data. Does not remove elements from list, instead sets _size to 0.
 */
static size_t
clear(ArrayList_t *self) {
    size_t old_size = self->_size;
    self->_size = 0;

    return old_size;
}

/*
 * Completely frees memory allocated for ArrayList together with all it's elements
 */
static void
free_al(ArrayList_t *self) {
    for (int i = 0; i < self->_size; i++) {
        free(self->data[i]);
    }
    free(self);
}

ArrayList_t *
ArrayList(const size_t *init_capacity) {
    size_t capacity = 16;

    if (init_capacity != NULL) {
        capacity = *init_capacity;
    }

    void **data = calloc(capacity, sizeof(size_t));

    if (data == NULL) {
        free(data);
#ifdef STRICT_ACCESS_CHECK
        exit(-1);
#else
        return NULL;
#endif
    }
    ArrayList_t *array_list;
    cmalloc_safe(array_list, sizeof(ArrayList_t))

    array_list->capacity = capacity;
    array_list->_size = 0;
    array_list->data = data;
    array_list->type_size = sizeof(size_t);
    array_list->get = get;
    array_list->set = set;
    array_list->push = push;
    array_list->pop = pop;
    array_list->insert = insert;
    array_list->removeAt = removeAt;
    array_list->size = size;
    array_list->clean = clear;
    array_list->indexOf = indexOf;

    return array_list;
}
