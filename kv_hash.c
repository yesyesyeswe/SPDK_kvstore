#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "kvstore.h"

#define MAX_TABLE_SIZE 1024

typedef struct hashnode_s {
    
    char *key;
    char *value;

    struct hashnode_s *next;

} hashnode_t;

typedef struct hashtable_s {
    hashnode_t **nodes;

    int max_slots;
    int count;

    pthread_mutex_t lock;

} hashtable_t;

hashtable_t *hash = NULL;

#ifdef KV_HASH_DEBUG
void* kvstore_malloc(size_t size) {
	return malloc(size);
}

void kvstore_free(void *ptr) {
	return free(ptr);
}
#endif

static int _hash(const char* key, int size) {
    if(!key) return -1;
    int sum = 0;
    int i = 0;
    while (key[i] != 0) {
        sum += key[i];
        i ++;
    }
    return sum % size;
}

static hashnode_t *_create_node(const char *key, const char *value) {
    hashnode_t *node = (hashnode_t *)kvstore_malloc(sizeof(hashnode_t));
    if(!node) return NULL;

    char* kcopy = kvstore_malloc(strlen(key) + 1);
    if(!kcopy) {
        fprintf(stderr, "kcopy malloc failed\n");
        return NULL;
    }

    char* vcopy = kvstore_malloc(strlen(value) + 1);
    if(!vcopy) {
        kvstore_free(kcopy);
        fprintf(stderr, "vcopy malloc failed\n");
        return NULL;
    }

    strcpy(kcopy, key);
    strcpy(vcopy, value);

	node->key = kcopy;
	node->value = vcopy;

    return node;
}

int kv_hash_init(void) {
    
    hash = (hashtable_t *)kvstore_malloc(sizeof(hashtable_t));
    if(!hash) return -1;

    hash->nodes = (hashnode_t **)kvstore_malloc(sizeof(hashnode_t*) * MAX_TABLE_SIZE);

    if (!hash->nodes) return -1;

    hash->max_slots = MAX_TABLE_SIZE;
    hash->count = 0;

    pthread_mutex_init(&hash->lock, NULL);

    return 0;
}

// 
void kv_hash_destroy(void) {
    if(!hash) return;

    pthread_mutex_lock(&hash->lock);
    int i = 0;
    for (i = 0; i < hash->max_slots; i ++) {
        hashnode_t *node = hash->nodes[i];
        while(node) {
            hashnode_t *prev = node;
            node = node->next;
            kvstore_free(prev->key);
            kvstore_free(prev->value);
            kvstore_free(prev);
        }
    }
    kvstore_free(hash->nodes);
    pthread_mutex_unlock(&hash->lock);
    pthread_mutex_destroy(&hash->lock);

    kvstore_free(hash);
}

int kv_hash_set(const char* key, const char *value) {
    if(!hash || !key || !value) return -1;

    int idx = _hash(key, MAX_TABLE_SIZE);

    pthread_mutex_lock(&hash->lock);
    hashnode_t *node = hash->nodes[idx];
    while(node) {
        if(strcmp(node->key, key) == 0) {
            pthread_mutex_unlock(&hash->lock);
            return 0;
        }
        node = node->next;
    }

    hashnode_t *new_node =_create_node(key, value);

    new_node->next = hash->nodes[idx];
    hash->nodes[idx] = new_node;

    hash->count ++;

    pthread_mutex_unlock(&hash->lock);

    return 0;
}

char* kv_hash_get(const char* key) {
    if(!hash || !key) return NULL;

    pthread_mutex_lock(&hash->lock);
    int idx = _hash(key, MAX_TABLE_SIZE);
    hashnode_t *node = hash->nodes[idx];

    while(node) {
        if(strcmp(node->key, key) == 0) {
            pthread_mutex_unlock(&hash->lock);
            return node->value;
        }
        node = node->next;
    }
    pthread_mutex_unlock(&hash->lock);

    return NULL;
}

int kv_hash_delete(char *key) {
    if(!hash || !key) return -1;

    pthread_mutex_lock(&hash->lock);
    int idx = _hash(key, MAX_TABLE_SIZE);
    hashnode_t *node = hash->nodes[idx];
    hashnode_t *prev = node;
    while(node) {
        if(strcmp(node->key, key) == 0) {
            kvstore_free(node->key);
            kvstore_free(node->value);

            // not the first
            if(node != hash->nodes[idx]) {
                prev->next = node->next;
            } else {
                hash->nodes[idx] = node->next;
            }      

            kvstore_free(node);  

            pthread_mutex_unlock(&hash->lock);
            return 0;
        }
        prev = node;
        node = node->next;
    }
    pthread_mutex_unlock(&hash->lock);

    return -1;
}

int kv_hash_modify(char *key, char* value) {
	if(!hash || !key || !value) return -1;

    int idx = _hash(key, MAX_TABLE_SIZE);
    hashnode_t *node = hash->nodes[idx];
    while(node) {
        if(strcmp(node->key, key) == 0) {

            char* vcopy = (char *)kvstore_malloc(strlen(value) + 1);
            if(!vcopy) {
                fprintf(stderr, "vcopy malloc failed\n");
                return -1;
            }
            strcpy(vcopy, value);

            pthread_mutex_lock(&hash->lock);
            kvstore_free(node->value);
            node->value = vcopy;
            pthread_mutex_unlock(&hash->lock);

            return 0;
        }
        node = node->next;
    }
    return -1;
}


#ifdef KV_HASH_DEBUG
int main() {
    
    kv_hash_init();

    kv_hash_set("city", "sz");
    kv_hash_set("server", "nginx");
    kv_hash_set("request url", "https://jjc.com");
    kv_hash_set("status code", "200");
    kv_hash_set("request method", "GET");

    char *result = kv_hash_get("city");
    printf("result fot city %s\n", result);

    char *server = kv_hash_get("server");
    printf("result fot server %s\n", server);

    kv_hash_modify("city", "shenzhen");
    result = kv_hash_get("city");
    printf("result fot city %s\n", result);

    kv_hash_delete("city");
    result = kv_hash_get("city");
    printf("result fot city %s\n", result);

    kv_hash_destroy();

    return 0;
}
#endif