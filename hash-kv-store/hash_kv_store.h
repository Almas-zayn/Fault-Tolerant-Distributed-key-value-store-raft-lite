#ifndef HASH_KV_STORE
#define HASH_KV_STORE

#define HASH_SIZE 1024

typedef struct kv_store_t
{
    char key[256];
    char value[768];
    struct kv_store_t *next;
} kv_store_t;

extern kv_store_t *hash_table[HASH_SIZE];
unsigned long hash(const char *str);
int kv_put_local(const char *key, const char *value);
int kv_get_local(const char *key, char *out, size_t out_size);
int kv_del_local(const char *key);

#endif