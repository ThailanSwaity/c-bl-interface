#include "dict.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

Dictionary dict_new(const unsigned int max_size) {
  Entry *new_entry_list = malloc(sizeof(Entry) * max_size);
  Dictionary dictionary = { .dict = new_entry_list, .size = 0, .max_size = max_size };
  return dictionary;
}

int compare_strings(char *A, char *B) {
  int M;
  int k;

  M = strlen(B);
  if (strcmp(A, B) < 0) M = strlen(A);
  for (k = 0; k < M; k++) {
    if (A[k] != B[k]) break;
  }
  return A[k] - B[k];
}

// I am not proud of this code lol
void dict_insert_kv(char *key, void *value, Dictionary *D) {
  assert(D->size < D->max_size);

  Entry E = { key, value };
  if (D->size == 0) {
    D->dict[D->size++] = E;
    return;
  }

  int M;
  int k;
  int i;
  int j;

  for (i = 0; i < (int)D->size; i++) {
    M = strlen(D->dict[i].key);
    if (strcmp(key, D->dict[i].key) < 0) M = strlen(key);
    assert(strcmp(key, D->dict[i].key) != 0);
    for (k = 0; k < M; k++) {
      if (key[k] != D->dict[i].key[k]) break;
    }
    if (key[k] < D->dict[i].key[k]) {
      // Shift all elements after i - 1 to the right, and insert E at i
      for (j = D->size; j > i; j--) {
        D->dict[j] = D->dict[j - 1];
      }
      D->dict[i] = E;
      D->size++;
      break;
    }
  }
  if (i == (int)D->size) D->dict[D->size++] = E;
}

void bin_search(char *key, Dictionary *D, int *Middle, int *Compare) {
  int L = 0;
  int R = D->size - 1;
  int M;
  int c;

  while (L <= R) {
    M = L + ceil((R - L) / 2);
    c = compare_strings(D->dict[M].key, key);

    if (c < 0) L = M + 1;
    else if (c > 0) R = M - 1;
    else if (c == 0) break;
  }
  *Middle = M;
  *Compare = c;
}

// Uses binary search to find element
void* dict_find_kv(char *key, Dictionary *D) {
  int M;
  int c;

  bin_search(key, D, &M, &c);

  if (c != 0) {
    return NULL;
  }
  return D->dict[M].value;
}

// Uses binary search to find element to delete
void dict_delete_kv(char *key, Dictionary *D) {
  int M;
  int c;

  bin_search(key, D, &M, &c);

  if (c == 0) {
    for (int i = M; i < (int)D->size - 1; i++) {
      D->dict[i] = D->dict[i + 1];
    }
    D->size--;
  }
}
