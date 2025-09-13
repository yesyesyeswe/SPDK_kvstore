#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "kvstore.h"
#include "mymalloc.h"

#define MAX_TABLE_SIZE  1024

#ifdef KV_ARRAY_DEBUG
void* kvstore_malloc(size_t size) {
	return mymalloc(size);
}

void kvstore_free(void *ptr) {
	return myfree(ptr);
}
#endif

int kv_array_destory = 0;

typedef struct kvpair_s {
    char* key;
    char* value;

    // struct kvpair_s *next;

} kvpair_t;

typedef struct kvstore_s {
    kvpair_t *table;
    int max_pairs;
    int num_pairs;

    pthread_mutex_t mutex;
    
} kvstore_t;

kvstore_t *store = NULL;

int kv_array_init(void) {
    
    store = kvstore_malloc(sizeof(kvstore_t));
    if(!store) {
        fprintf(stderr, "malloc store error\n");
        return -1;
    }

    store->table = (kvpair_t *)kvstore_malloc(sizeof(kvpair_t) * MAX_TABLE_SIZE);
    if(!store->table) {
        fprintf(stderr, "malloc store table error\n");
        return -1;
    }
    store->max_pairs = MAX_TABLE_SIZE;
    store->num_pairs = 0;

    pthread_mutex_init(&store -> mutex, NULL);

    return 0;
}

void kv_array_destroy(void) {
    if (!store) return; 
    pthread_mutex_lock(&store->mutex);

    if(kv_array_destory) return;
    kv_array_destory = 1;

    // free all key-value pairs
    if (store->table) {
        for (int i = 0; i < store->num_pairs; i++) {
            if (store->table[i].key) {
                kvstore_free(store->table[i].key);
            }
            if (store->table[i].value) {
                kvstore_free(store->table[i].value);
            }
        }
        
        kvstore_free(store->table);
        store->table = NULL;
    }
    pthread_mutex_unlock(&store->mutex);
    pthread_mutex_destroy(&store->mutex);
    
    kvstore_free(store);
    store = NULL; 
}

int kv_array_set(const char* key, const char *value) {
    
    if(!store || !store->table || !key || !value) {
        fprintf(stderr, "store %p, store->table %p, key %s, value %s\n", store, store->table, key, value);
        return -1;
    }

    if(store->num_pairs >= store -> max_pairs) {
        fprintf(stderr, "kv store full\n");
        return -1;
    }

    int idx = 0;
    pthread_mutex_lock(&store -> mutex);
    idx = store->num_pairs;
    store->num_pairs ++;
    pthread_mutex_unlock(&store -> mutex);

    char* kcopy = kvstore_malloc(strlen(key) + 1);
    if(!kcopy) {
        fprintf(stderr, "kcopy malloc failed\n");
        return -1;
    }

    char* vcopy = kvstore_malloc(strlen(value) + 1);
    if(!vcopy) {
        kvstore_free(kcopy);
        fprintf(stderr, "vcopy malloc failed\n");
        return -1;
    }

    strcpy(kcopy, key);
    strcpy(vcopy, value);

    store->table[idx].key = kcopy;
    store->table[idx].value = vcopy;

    return 0;
}

char* kv_array_get(const char* key) {
    if(!store || !store->table || !key) return NULL;

    int i = 0;
    for(i = 0; i < store->num_pairs; i ++) {
        if(!store->table[i].key) continue;
        if(strcmp(store->table[i].key, key) == 0) {
            return store->table[i].value;
        }
    }
    return NULL;
}

int kv_array_delete(char *key) {
    int i = 0;
    for (i = 0; i < store->num_pairs; i ++) {
        if(!store->table[i].key) continue;
        if(strcmp(store->table[i].key, key) == 0) {

            kvstore_free(store->table[i].key);
            kvstore_free(store->table[i].value);
            
            // NOTE: Breaks original insertion ordering
            if (i < store->num_pairs - 1) {
                store->table[i] = store->table[store->num_pairs - 1];
            }
            
            store->table[store->num_pairs - 1].key = NULL;
            store->table[store->num_pairs - 1].value = NULL;
            store->num_pairs--;

            return 0;
        }
    }
    return -1;
}

int kv_array_modify(char* key, char *value) {
    if(!store || !store->table || !key || !value) return -1;

    int i = 0;
    for(i = 0; i < store->num_pairs; i ++) {
        if(strcmp(store->table[i].key, key) == 0) {
            char* vcopy = kvstore_malloc(strlen(value) + 1);
            if(!vcopy) {
                return -1;
            }
            strcpy(vcopy, value);
            
            pthread_mutex_lock(&store -> mutex);
            // may be we need unlock here
            kvstore_free(store->table[i].value);
            store->table[i].value = vcopy;
            pthread_mutex_unlock(&store -> mutex);
            return 0;
        }
    }
    return i;
}


#ifdef KV_ARRAY_DEBUG
int main(int argc, char* argv[]) {
    kv_array_init();
    
    kv_array_set("name", "jjc");
    kv_array_set("city", "sz");
    kv_array_set("server", "nginx");
    kv_array_set("request url", "https://jjc.com");
    kv_array_set("status code", "200");
    kv_array_set("request method", "GET");

    char *result = kv_array_get("city");
    printf("result fot city %s\n", result);

    char *server = kv_array_get("server");
    printf("result fot server %s\n", server);

    kv_array_delete("city");
    result = kv_array_get("city");
    printf("result fot city %s\n", result);

    kv_array_destroy();

    return 0;
}
#endif