#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    float *data;
    size_t size;
    size_t capacity;
} Array;

Array newArray(size_t initialCapacity) {
    Array arr;
    arr.data = (float*)malloc(initialCapacity * sizeof(float));
    arr.size = 0;
    arr.capacity = initialCapacity;
    return arr;
}

void resizeArray(Array *arr, size_t newCapacity) {
    arr->data = (float*)realloc(arr->data, newCapacity * sizeof(float));
    arr->capacity = newCapacity;
}

void push(Array *arr, float element) {
    if (arr->size == arr->capacity) {
        resizeArray(arr, arr->capacity * 2);
    }
    arr->data[arr->size] = element;
    arr->size++;
}

void freeArray(Array *arr) {
    free(arr->data);
    arr->size = 0;
    arr->capacity = 0;
}