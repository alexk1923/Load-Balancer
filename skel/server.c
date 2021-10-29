/* Copyright 2021 Kullman Alexandru 313CA */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "./server.h"
#include "./load_balancer.h"
#include "./utils.h"

#define MAX_HT 20000

server_memory* init_server_memory() {
	/* Create server */
	server_memory* sv_mem = malloc(1 * sizeof(server_memory));
	DIE(sv_mem == NULL, "malloc() failed");

	/* Create hashtable */
	sv_mem->ht = ht_create(MAX_HT, hash_function_key, compare_function_strings);
	return sv_mem;
}

void server_store(server_memory* server, char* key, char* value) {
	/* Store the (key:value) pair into the hashtable */
	ht_put(server->ht, (void *)key, strlen(key) + 1, (void *)value,
			strlen(value) + 1);
}

void server_remove(server_memory* server, char* key) {
	/* Remove the (key:value) pair from the hashtable */
	ht_remove_entry(server->ht, key);
}

char* server_retrieve(server_memory* server, char* key) {
	/* Get the value corresponding to the key from the hashtable */
	char *value = (char *)ht_get(server->ht, key);
	return value;
}

void free_server_memory(server_memory* server) {
	ht_free(server->ht);
	free(server);
}
