# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Intel Corporation.
# All rights reserved.
#
SPDK_ROOT_DIR := $(abspath $(CURDIR))/../spdk
include $(SPDK_ROOT_DIR)/mk/spdk.common.mk
include $(SPDK_ROOT_DIR)/mk/spdk.modules.mk

APP = kvstore

TOP_DIR    := .
SRC_DIR    := $(TOP_DIR)/src
C_SRCS := $(shell find $(SRC_DIR) -type f -name '*.c')

SPDK_LIB_LIST = $(SOCK_MODULES_LIST)
SPDK_LIB_LIST += event sock

include $(SPDK_ROOT_DIR)/mk/spdk.app.mk

test_array:
	gcc -g -O0 -o $(SRC_DIR)/engine/kv_array $(SRC_DIR)/engine/kv_array.c $(SRC_DIR)/mm/mymalloc.c -DKV_ARRAY_DEBUG

test_rbtree:
	gcc -g -O0 -o $(SRC_DIR)/engine/kv_rbtree $(SRC_DIR)/engine/kv_rbtree.c $(SRC_DIR)/mm/mymalloc.c -DKV_RBTREE_DEBUG

test_hash:
	gcc -g -O0 -o $(SRC_DIR)/engine/kv_hash $(SRC_DIR)/engine/kv_hash.c $(SRC_DIR)/mm/mymalloc.c -DKV_HASH_DEBUG

D_OBJ := $(shell find $(SRC_DIR) -type f \( -name '*.d' -o -name '*.o' \))

clean:
	@rm $(D_OBJ) $(APP)

.PHONY: test_array, test_rbtree, test_hash, clean