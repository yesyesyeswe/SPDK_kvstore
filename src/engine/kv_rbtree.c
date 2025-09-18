#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "../kvstore.h"
#include "../mm/mymalloc.h"

#define RED				1
#define BLACK 			2

#define KEYTYPE_ENABLE 1

#if KEYTYPE_ENABLE
typedef char* KEY_TYPE;
#else
typedef int KEY_TYPE;
#endif

typedef struct _rbtree_node {
	unsigned char color;
	struct _rbtree_node *right;
	struct _rbtree_node *left;
	struct _rbtree_node *parent;
#if KEYTYPE_ENABLE
	char* key;
	char* value;
#else
	KEY_TYPE key;
	void *value;
#endif
} rbtree_node;

typedef struct _rbtree_node rbtree_node_t;

typedef struct _rbtree {
	rbtree_node *root;
	rbtree_node *nil;
	pthread_mutex_t lock;
} rbtree;

typedef struct _rbtree rbtree_t;
rbtree_t *tree = NULL;

static rbtree_node *rbtree_mini(rbtree *T, rbtree_node *x) {
	while (x->left != T->nil) {
		x = x->left;
	}
	return x;
}

static rbtree_node *rbtree_maxi(rbtree *T, rbtree_node *x) {
	while (x->right != T->nil) {
		x = x->right;
	}
	return x;
}

static rbtree_node *rbtree_successor(rbtree *T, rbtree_node *x) {
	rbtree_node *y = x->parent;

	if (x->right != T->nil) {
		return rbtree_mini(T, x->right);
	}

	while ((y != T->nil) && (x == y->right)) {
		x = y;
		y = y->parent;
	}
	return y;
}


static void rbtree_left_rotate(rbtree *T, rbtree_node *x) {

	rbtree_node *y = x->right;  // x  --> y  ,  y --> x,   right --> left,  left --> right

	x->right = y->left; //1 1
	if (y->left != T->nil) { //1 2
		y->left->parent = x;
	}

	y->parent = x->parent; //1 3
	if (x->parent == T->nil) { //1 4
		T->root = y;
	} else if (x == x->parent->left) {
		x->parent->left = y;
	} else {
		x->parent->right = y;
	}

	y->left = x; //1 5
	x->parent = y; //1 6
}


static void rbtree_right_rotate(rbtree *T, rbtree_node *y) {

	rbtree_node *x = y->left;

	y->left = x->right;
	if (x->right != T->nil) {
		x->right->parent = y;
	}

	x->parent = y->parent;
	if (y->parent == T->nil) {
		T->root = x;
	} else if (y == y->parent->right) {
		y->parent->right = x;
	} else {
		y->parent->left = x;
	}

	x->right = y;
	y->parent = x;
}

static void rbtree_insert_fixup(rbtree *T, rbtree_node *z) {

	while (z->parent->color == RED) { //z ---> RED
		if (z->parent == z->parent->parent->left) {
			rbtree_node *y = z->parent->parent->right;
			if (y->color == RED) {
				z->parent->color = BLACK;
				y->color = BLACK;
				z->parent->parent->color = RED;

				z = z->parent->parent; //z --> RED
			} else {

				if (z == z->parent->right) {
					z = z->parent;
					rbtree_left_rotate(T, z);
				}

				z->parent->color = BLACK;
				z->parent->parent->color = RED;
				rbtree_right_rotate(T, z->parent->parent);
			}
		}else {
			rbtree_node *y = z->parent->parent->left;
			if (y->color == RED) {
				z->parent->color = BLACK;
				y->color = BLACK;
				z->parent->parent->color = RED;

				z = z->parent->parent; //z --> RED
			} else {
				if (z == z->parent->left) {
					z = z->parent;
					rbtree_right_rotate(T, z);
				}

				z->parent->color = BLACK;
				z->parent->parent->color = RED;
				rbtree_left_rotate(T, z->parent->parent);
			}
		}
		
	}

	T->root->color = BLACK;
}


static void rbtree_insert(rbtree *T, rbtree_node *z) {

	rbtree_node *y = T->nil;
	rbtree_node *x = T->root;

	// strcmp(s1, s2) -1 <, 0 =, 1 >
	while (x != T->nil) {
		y = x;
#if KEYTYPE_ENABLE
		if(strcmp(z->key, x->key) < 0) {
			x = x->left;
		} else if(strcmp(z->key, x->key) > 0) {
			x = x->right;
		} else {
			return;
		}

#else
		if (z->key < x->key) {
			x = x->left;
		} else if (z->key > x->key) {
			x = x->right;
		} else { //Exist
			return ;
		}
#endif
	}

	z->parent = y;
#if KEYTYPE_ENABLE
	if (y == T->nil) {
		T->root = z;
	} else if (strcmp(z->key, y->key) < 0) {
		y->left = z;
	} else {
		y->right = z;
	}
#else
	if (y == T->nil) {
		T->root = z;
	} else if (z->key < y->key) {
		y->left = z;
	} else {
		y->right = z;
	}
#endif
	z->left = T->nil;
	z->right = T->nil;
	z->color = RED;

	rbtree_insert_fixup(T, z);
}

static void rbtree_delete_fixup(rbtree *T, rbtree_node *x) {

	while ((x != T->root) && (x->color == BLACK)) {
		if (x == x->parent->left) {

			rbtree_node *w= x->parent->right;
			if (w->color == RED) {
				w->color = BLACK;
				x->parent->color = RED;

				rbtree_left_rotate(T, x->parent);
				w = x->parent->right;
			}

			if ((w->left->color == BLACK) && (w->right->color == BLACK)) {
				w->color = RED;
				x = x->parent;
			} else {

				if (w->right->color == BLACK) {
					w->left->color = BLACK;
					w->color = RED;
					rbtree_right_rotate(T, w);
					w = x->parent->right;
				}

				w->color = x->parent->color;
				x->parent->color = BLACK;
				w->right->color = BLACK;
				rbtree_left_rotate(T, x->parent);

				x = T->root;
			}

		} else {

			rbtree_node *w = x->parent->left;
			if (w->color == RED) {
				w->color = BLACK;
				x->parent->color = RED;
				rbtree_right_rotate(T, x->parent);
				w = x->parent->left;
			}

			if ((w->left->color == BLACK) && (w->right->color == BLACK)) {
				w->color = RED;
				x = x->parent;
			} else {

				if (w->left->color == BLACK) {
					w->right->color = BLACK;
					w->color = RED;
					rbtree_left_rotate(T, w);
					w = x->parent->left;
				}

				w->color = x->parent->color;
				x->parent->color = BLACK;
				w->left->color = BLACK;
				rbtree_right_rotate(T, x->parent);

				x = T->root;
			}

		}
	}

	x->color = BLACK;
}

static rbtree_node *rbtree_delete(rbtree *T, rbtree_node *z) {

	rbtree_node *y = T->nil;
	rbtree_node *x = T->nil;

	if ((z->left == T->nil) || (z->right == T->nil)) {
		y = z;
	} else {
		y = rbtree_successor(T, z);
	}

	if (y->left != T->nil) {
		x = y->left;
	} else if (y->right != T->nil) {
		x = y->right;
	}

	x->parent = y->parent;
	if (y->parent == T->nil) {
		T->root = x;
	} else if (y == y->parent->left) {
		y->parent->left = x;
	} else {
		y->parent->right = x;
	}

	if (y != z) {
#if KEYTYPE_ENABLE
	strcpy(z->key, y->key);
	strcpy(z->value, y->value);
#else
		z->key = y->key;
		z->value = y->value;
#endif
	}

	if (y->color == BLACK) {
		rbtree_delete_fixup(T, x);
	}

	return y;
}

static rbtree_node *rbtree_search(rbtree *T, KEY_TYPE key) {

	rbtree_node *node = T->root;
	while (node != T->nil) {
#if KEYTYPE_ENABLE
		int ret = strcmp(key, node->key);
		if (ret < 0) {
			node = node->left;
		} else if (ret > 0) {
			node = node->right;
		} else {
			return node;
		}	
#else
		if (key < node->key) {
			node = node->left;
		} else if (key > node->key) {
			node = node->right;
		} else {
			return node;
		}	
#endif
	}
	return T->nil;
}


static void rbtree_traversal(rbtree *T, rbtree_node *node) {
	if (node != T->nil) {
		rbtree_traversal(T, node->left);
#if KEYTYPE_ENABLE
		printf("key:%s, color:%d\n", node->key, node->color);
#else
		printf("key:%d, color:%d\n", node->key, node->color);
#endif
		rbtree_traversal(T, node->right);
	}
}

int kv_rbtree_init(void) {
	tree = (rbtree *)mymalloc(sizeof(rbtree));
	if (!tree) {
		printf("malloc failed\n");
		return -1;
	}

	tree->nil = (rbtree_node*)mymalloc(sizeof(rbtree_node));
	tree->nil->color = BLACK;
	tree->nil->left = tree->nil;
	tree->nil->right = tree->nil;
	tree->root = tree->nil;

	pthread_mutex_init(&tree->lock, NULL);
	return 0;
}

void kv_rbtree_destroy(void) {
	if(!tree) return;

	rbtree_node_t *node =  tree->root;
	while (node != tree->nil) {
		node = rbtree_mini(tree, tree->root);
		if (node == tree->nil) break;

		pthread_mutex_lock(&tree->lock);
		node = rbtree_delete(tree, node);
		pthread_mutex_unlock(&tree->lock);
		
		myfree(node);
	}

	myfree(tree->nil);
}

int kv_rbtree_set(const char* key, const char *value) {
	if(!tree || !key || !value) return -1;

	rbtree_node *node = (rbtree_node*)mymalloc(sizeof(rbtree_node));
	if(!node) return -1;

	char* kcopy = mymalloc(strlen(key) + 1);
    if(!kcopy) {
        fprintf(stderr, "kcopy malloc failed\n");
        return -1;
    }

    char* vcopy = mymalloc(strlen(value) + 1);
    if(!vcopy) {
        myfree(kcopy);
        fprintf(stderr, "vcopy malloc failed\n");
        return -1;
    }

    strcpy(kcopy, key);
    strcpy(vcopy, value);

	node->key = kcopy;
	node->value = vcopy;

	pthread_mutex_lock(&tree->lock);
	rbtree_insert(tree, node);
	pthread_mutex_unlock(&tree->lock);

	return 0;
}

char *kv_rbtree_get(char* key) {
	if(!tree || !key) return NULL;

	rbtree_node *node = rbtree_search(tree, key);
	if(node == tree->nil) return NULL;

	return node->value;
}

int kv_rbtree_delete(char *key) {
	if(!tree || !key) return -1;

	rbtree_node *node = rbtree_search(tree, key);

	if(node == tree->nil) return -1;
	
	pthread_mutex_lock(&tree->lock);
	node = rbtree_delete(tree, node);
	if(node == tree->nil) {
		myfree(node->key);
		myfree(node->value);
		myfree(node);
	}
	pthread_mutex_unlock(&tree->lock);
	
	return 0;
}

int kv_rbtree_modify(char *key, char* value) {
	if(!tree || !key || !value) return -1;
	rbtree_node *node = rbtree_search(tree, key);
	if(node == tree->nil) {
		return -1;
	}

	char* vcopy = mymalloc(strlen(value) + 1);
    if(!vcopy) {
        fprintf(stderr, "vcopy malloc failed\n");
        return -1;
    }
	strcpy(vcopy, value);

	pthread_mutex_lock(&tree->lock);
	myfree(node->value);
	node->value = vcopy;
	pthread_mutex_unlock(&tree->lock);

	return 0;
}

#ifdef KV_RBTREE_DEBUG
int main() {

#if 1
	kv_rbtree_init();

	kv_rbtree_set("city", "sz");
    kv_rbtree_set("server", "nginx");
    kv_rbtree_set("request url", "https://jjc.com");
    kv_rbtree_set("status code", "200");
    kv_rbtree_set("request method", "GET");

	char *result = kv_rbtree_get("city");
    printf("result fot city %s\n", result);

    char *server = kv_rbtree_get("server");
    printf("result fot server %s\n", server);

	kv_rbtree_modify("city", "SHENZHEN");

	result = kv_rbtree_get("city");
    printf("result fot city %s\n", result);

	kv_rbtree_delete("city");

	result = kv_rbtree_get("city");
    printf("result fot city %s\n", result);

    kv_rbtree_destroy();
	return 0;

#elif KEYTYPE_ENABLE
	rbtree *T = (rbtree *)mymalloc(sizeof(rbtree));
	if (T == NULL) {
		printf("malloc failed\n");
		return -1;
	}

	T->nil = (rbtree_node*)mymalloc(sizeof(rbtree_node));
	T->nil->color = BLACK;
	T->root = T->nil;

	rbtree_node *node1 = (rbtree_node*)mymalloc(sizeof(rbtree_node));

	strncpy(node1->key, "city", MAX_KEY_LEN);
	strncpy(node1->value, "sz", MAX_VALUE_LEN);
	rbtree_insert(T, node1);

	rbtree_node *node2 = (rbtree_node*)mymalloc(sizeof(rbtree_node));

	strncpy(node2->key, "server", MAX_KEY_LEN);
	strncpy(node2->value, "nginx", MAX_VALUE_LEN);
	rbtree_insert(T, node2);

	// put_kvhash(&hash, "city", "sz");
    // put_kvhash(&hash, "server", "nginx");
    // put_kvhash(&hash, "request url", "https://jjc.com");
    // put_kvhash(&hash, "status code", "200");
    // put_kvhash(&hash, "request method", "GET");

    rbtree_node *n1 = rbtree_search(T, "city");
    printf("result fot city %s\n", n1 -> value);

    rbtree_node *n2 = rbtree_search(T, "server");
    printf("result fot server %s\n", n2 -> value);

#else
	int keyArray[20] = {24,25,13,35,23, 26,67,47,38,98, 20,19,17,49,12, 21,9,18,14,15};

	rbtree *T = (rbtree *)mymalloc(sizeof(rbtree));
	if (T == NULL) {
		printf("malloc failed\n");
		return -1;
	}
	
	T->nil = (rbtree_node*)mymalloc(sizeof(rbtree_node));
	T->nil->color = BLACK;
	T->root = T->nil;

	rbtree_node *node = T->nil;
	int i = 0;
	for (i = 0;i < 20;i ++) {
		node = (rbtree_node*)mymalloc(sizeof(rbtree_node));
		node->key = keyArray[i];
		node->value = NULL;

		rbtree_insert(T, node);
		
	}

	rbtree_traversal(T, T->root);
	printf("----------------------------------------\n");

	for (i = 0;i < 20;i ++) {

		rbtree_node *node = rbtree_search(T, keyArray[i]);
		rbtree_node *cur = rbtree_delete(T, node);
		myfree(cur);

		rbtree_traversal(T, T->root);
		printf("----------------------------------------\n");
	}
#endif
	
}

#endif



