# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Intel Corporation.
# All rights reserved.
#
SPDK_ROOT_DIR := $(abspath $(CURDIR))/../spdk
include $(SPDK_ROOT_DIR)/mk/spdk.common.mk
include $(SPDK_ROOT_DIR)/mk/spdk.modules.mk

APP = kvstore
C_SRCS := spdk_server.c kvstore.c kv_array.c kv_rbtree.c kv_hash.c

SPDK_LIB_LIST = $(SOCK_MODULES_LIST)
SPDK_LIB_LIST += event sock

include $(SPDK_ROOT_DIR)/mk/spdk.app.mk


test_array:
	gcc -g -O0 -o kv_array kv_array.c -DKV_ARRAY_DEBUG

test_rbtree:
	gcc -g -O0 -o kv_rbtree kv_rbtree.c -DKV_RBTREE_DEBUG

test_hash:
	gcc -g -O0 -o kv_hash kv_hash.c -DKV_HASH_DEBUG


PHONY: test_array, test_rbtree, test_hash