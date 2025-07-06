#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "kvstore.h"

#define BUFFER_SIZE			1024
static const char *commands[] = {
	"SET", "GET", "DEL", "MOD", 
	"HSET", "HGET", "HDEL", "HMOD", 
	"RSET", "RGET", "RDEL", "RMOD", 
};

int spdk_entry(int argc, char *argv[]);

void* kvstore_malloc(size_t size) {
	return malloc(size);
}

void kvstore_free(void *ptr) {
	return free(ptr);
}


static int kvs_split_tokens(char **tokens, char *msg) {
	
	int count = 0;
	char *token = token = strtok(msg, " ");
	while(token) {
		 tokens[count ++] = token;
		 token = strtok(NULL, " ");
	};
	
	return count;
}

static int kvs_proto_parser(char *msg, char **tokens, int count) {
	if(!msg || !tokens || count <= 0) return -1;

	int cmd = 0;

	for(cmd = 0; cmd < KVS_CMD_COUNT; cmd ++) {
		if(strcmp(tokens[0], commands[cmd]) == 0) break;
	}

	int res = 0;
	char *value = NULL;
	switch(cmd) {
		case KVS_CMD_SET:
			res = kv_array_set(tokens[1], tokens[2]);
			if(res) {
				snprintf(msg, BUFFER_SIZE, "SET FAILED");
			} else {
				snprintf(msg, BUFFER_SIZE, "SET SUCCESS");
			}
			return 11 + (res == 0);
		case KVS_CMD_GET:
			value = kv_array_get(tokens[1]);
			if(!value) {
				snprintf(msg, BUFFER_SIZE, "GET FAILED");
				return 12;
			} else {
				snprintf(msg, BUFFER_SIZE, "%s", value);
				return strlen(value) + 1;
			}
			assert(0);
			break;
		case KVS_CMD_DEL:
			res = kv_array_delete(tokens[1]);
			if(res) {
				snprintf(msg, BUFFER_SIZE, "DEL FAILED");
			} else {
				snprintf(msg, BUFFER_SIZE, "DEL SUCCESS");
			}
			return 11 + (res == 0);
		case KVS_CMD_MOD:
			res = kv_array_modify(tokens[1], tokens[2]);
			if(res) {
				snprintf(msg, BUFFER_SIZE, "MOD FAILED");	
			} else {
				snprintf(msg, BUFFER_SIZE, "MOD SUCCESS");
			}
			return 11 + (res == 0);
		case KVS_CMD_HSET:
			res = kv_hash_set(tokens[1], tokens[2]);
			if(res) {
				snprintf(msg, BUFFER_SIZE, "HSET FAILED");
			} else {
				snprintf(msg, BUFFER_SIZE, "HSET SUCCESS");
			}
			return 12 + (res == 0);
		case KVS_CMD_HGET:
			value = kv_hash_get(tokens[1]);
			if(!value) {
				snprintf(msg, BUFFER_SIZE, "HGET FAILED");
				return 13;
			} else {
				snprintf(msg, BUFFER_SIZE, "%s", value);
				return strlen(value) + 1;
			}
			assert(0);
			break;
		case KVS_CMD_HDEL:
			res = kv_hash_delete(tokens[1]);
			if(res) {
				snprintf(msg, BUFFER_SIZE, "HDEL FAILED");
			} else {
				snprintf(msg, BUFFER_SIZE, "HDEL SUCCESS");
			}
			return 12 + (res == 0);
		case KVS_CMD_HMOD:
			res = kv_hash_modify(tokens[1], tokens[2]);
			if(res) {
				snprintf(msg, BUFFER_SIZE, "HMOD FAILED");	
			} else {
				snprintf(msg, BUFFER_SIZE, "HMOD SUCCESS");
			}
			return 12 + (res == 0);
			break;
		case KVS_CMD_RSET:
			res = kv_rbtree_set(tokens[1], tokens[2]);
			if(res) {
				snprintf(msg, BUFFER_SIZE, "SET FAILED");
			} else {
				snprintf(msg, BUFFER_SIZE, "SET SUCCESS");
			}
			return 11 + (res == 0);
		case KVS_CMD_RGET:
			value = kv_rbtree_get(tokens[1]);
			if(!value) {
				snprintf(msg, BUFFER_SIZE, "GET FAILED");
				return 12;
			} else {
				snprintf(msg, BUFFER_SIZE, "%s", value);
				return strlen(value) + 1;
			}
			assert(0);
			break;
		case KVS_CMD_RDEL:
			res = kv_rbtree_delete(tokens[1]);
			if(res) {
				snprintf(msg, BUFFER_SIZE, "DEL FAILED");
			} else {
				snprintf(msg, BUFFER_SIZE, "DEL SUCCESS");
			}
			return 11 + (res == 0);
		case KVS_CMD_RMOD:
			res = kv_rbtree_modify(tokens[1], tokens[2]);
			if(res) {
				snprintf(msg, BUFFER_SIZE, "MOD FAILED");	
			} else {
				snprintf(msg, BUFFER_SIZE, "MOD SUCCESS");
			}
			return 11 + (res == 0);
			break;
	}
	return 0;
}


int kvstore_request(char *msg, ssize_t len) {
	
	char *tokens[MAX_TOKENS] = {0};
	
	int count = kvs_split_tokens(tokens, msg);
	int i = 0;
	for(i = 0; i < count; i ++) {
		printf("token %d : %s\n", i, tokens[i]);
	}
	return kvs_proto_parser(msg, tokens, count); 
}


static int kvstore_response(void) {
    return 0;
}


int main(int argc, char *argv[]) {
	int ret = 0;
	ret = kv_array_init();
	if(ret) {
		fprintf(stderr, "Failed initial array\n");
		return 1;
	}
	ret = kv_rbtree_init();
	if(ret) {
		fprintf(stderr, "Failed initial rbtree\n");
		return 1;
	}
	ret = kv_hash_init();
	if(ret) {
		fprintf(stderr, "Failed initial hash\n");
		return 1;
	}
    spdk_entry(argc, argv);
}