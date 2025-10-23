#ifndef DICTIONARY_H
#define DICTIONARY_H

typedef struct Entry {
  char* key;
  void *value;
} Entry;

typedef struct Dictionary {
  Entry *dict;
  unsigned int size;
  const unsigned int max_size;
} Dictionary;

int compare_strings(char *A, char *B);
Dictionary dict_new(const unsigned int max_size);
void dict_insert_kv(char *key, void *value, Dictionary *D);
void* dict_find_kv(char *key, Dictionary *D);
void dict_delete_kv(char *key, Dictionary *D);

#endif // DICTIONARY_H
