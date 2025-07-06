#ifndef __KVSTORE_H__
#define __KVSTORE_H__

#include <unistd.h>
#include <assert.h>

#define MAX_TOKENS	16

typedef enum {
	KVS_CMD_START = 0,
	KVS_CMD_SET = KVS_CMD_START,
	KVS_CMD_GET,
	KVS_CMD_DEL,
	KVS_CMD_MOD,
	KVS_CMD_HSET,
	KVS_CMD_HGET,
	KVS_CMD_HDEL,
	KVS_CMD_HMOD,
	KVS_CMD_RSET,
	KVS_CMD_RGET,
	KVS_CMD_RDEL,
	KVS_CMD_RMOD,
	KVS_CMD_COUNT,
} kvs_cmd_t;


int spdk_entry(int argc, char *argv[]);
int kvstore_request(char *msg, ssize_t len);

void *kvstore_malloc(size_t size);
void kvstore_free(void *ptr);

int kv_array_init(void);
void kv_array_destroy(void);
int kv_array_set(const char* key, const char *value);
char* kv_array_get(const char* key);
int kv_array_delete(char *key);
int kv_array_modify(char* key, char *value);

int kv_rbtree_init(void);
void kv_rbtree_destroy(void);
int kv_rbtree_set(const char* key, const char *value);
char* kv_rbtree_get(char* key);
int kv_rbtree_delete(char *key);
int kv_rbtree_modify(char* key, char *value);

int kv_hash_init(void);
void kv_hash_destroy(void);
int kv_hash_set(const char* key, const char *value);
char* kv_hash_get(const char* key);
int kv_hash_delete(char *key);
int kv_hash_modify(char* key, char *value); 

#endif
 