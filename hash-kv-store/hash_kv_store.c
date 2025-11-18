#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_SIZE 1024

typedef struct kv_entry
{
    char key[128];
    char value[256];
    struct kv_entry *next;
} kv_entry_t;

kv_entry_t *hash_table[HASH_SIZE];

unsigned long hash(const char *str)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash % HASH_SIZE;
}

int kv_put_local(const char *key, const char *value)
{
    unsigned long idx = hash(key);
    kv_entry_t *curr = hash_table[idx];

    while (curr)
    {
        if (strcmp(curr->key, key) == 0)
        {
            strncpy(curr->value, value, sizeof(curr->value) - 1);
            curr->value[sizeof(curr->value) - 1] = '\0';
            return 1;
        }
        curr = curr->next;
    }

    kv_entry_t *new_entry = malloc(sizeof(kv_entry_t));
    if (!new_entry)
        return 0;

    strncpy(new_entry->key, key, sizeof(new_entry->key) - 1);
    new_entry->key[sizeof(new_entry->key) - 1] = '\0';

    strncpy(new_entry->value, value, sizeof(new_entry->value) - 1);
    new_entry->value[sizeof(new_entry->value) - 1] = '\0';

    new_entry->next = hash_table[idx];
    hash_table[idx] = new_entry;
    return 1;
}

int kv_get_local(const char *key, char *out, size_t out_size)
{
    unsigned long idx = hash(key);
    kv_entry_t *curr = hash_table[idx];

    while (curr)
    {
        if (strcmp(curr->key, key) == 0)
        {
            strncpy(out, curr->value, out_size - 1);
            out[out_size - 1] = '\0';
            return 1;
        }
        curr = curr->next;
    }
    return 0;
}

int kv_del_local(const char *key)
{
    unsigned long idx = hash(key);
    kv_entry_t *curr = hash_table[idx];
    kv_entry_t *prev = NULL;

    while (curr)
    {
        if (strcmp(curr->key, key) == 0)
        {
            if (prev == NULL)
                hash_table[idx] = curr->next;
            else
                prev->next = curr->next;

            free(curr);
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }
    return 0;
}
