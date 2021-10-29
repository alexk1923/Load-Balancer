/* Copyright 2021 Kullman Alexandru 313CA */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./utils.h"

#include "./Hashtable.h"

#define MAX_BUCKET_SIZE 64

/*
 * Functii de comparare a cheilor:
 */
int
compare_function_ints(void *a, void *b)
{
	int int_a = *((int *)a);
	int int_b = *((int *)b);

	if (int_a == int_b) {
		return 0;
	} else if (int_a < int_b) {
		return -1;
	} else {
		return 1;
	}
}

int
compare_function_strings(void *a, void *b)
{
	char *str_a = (char *)a;
	char *str_b = (char *)b;
	return strcmp(str_a, str_b);
}

/*
 * Functii de hashing:
 */
unsigned int
hash_function_int(void *a)
{
	/*
	 * Credits: https://stackoverflow.com/a/12996028/7883884
	 */
	unsigned int uint_a = *((unsigned int *)a);

	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = (uint_a >> 16u) ^ uint_a;
	return uint_a;
}

// unsigned int
// hash_function_string(void *a)
// {
// 	/*
// 	 * Credits: http://www.cse.yorku.ca/~oz/hash.html
// 	 */
// 	unsigned char *puchar_a = (unsigned char*) a;
// 	unsigned long hash = 5381;
// 	int c;

// 	while ((c = *puchar_a++))
// 		hash = ((hash << 5u) + hash) + c; /* hash * 33 + c */

// 	return hash;
// }

/*
 * Functie apelata dupa alocarea unui hashtable pentru a-l initializa.
 * Trebuie alocate si initializate si listele inlantuite.
 */
hashtable_t *
ht_create(unsigned int hmax, unsigned int (*hash_function)(void*),
		int (*compare_function)(void*, void*))
{
	hashtable_t *ht = malloc(sizeof(hashtable_t));
	ht->compare_function = compare_function;
	ht->hash_function = hash_function;
	ht->buckets = malloc(sizeof(linked_list_t *) * hmax);
	ht->size = 0;
	ht->hmax = hmax;

	for(unsigned int i = 0; i < ht->hmax; i++) {
		ht->buckets[i] = ll_create(sizeof(struct info));
	}

	return ht;
}

/*
 * Atentie! Desi cheia este trimisa ca un void pointer (deoarece nu se impune tipul ei), in momentul in care
 * se creeaza o noua intrare in hashtable (in cazul in care cheia nu se gaseste deja in ht), trebuie creata o copie
 * a valorii la care pointeaza key si adresa acestei copii trebuie salvata in structura info asociata intrarii din ht.
 * Pentru a sti cati octeti trebuie alocati si copiati, folositi parametrul key_size_bytes.
 *
 * Motivatie:
 * Este nevoie sa copiem valoarea la care pointeaza key deoarece dupa un apel put(ht, key_actual, value_actual),
 * valoarea la care pointeaza key_actual poate fi alterata (de ex: *key_actual++). Daca am folosi direct adresa
 * pointerului key_actual, practic s-ar modifica din afara hashtable-ului cheia unei intrari din hashtable.
 * Nu ne dorim acest lucru, fiindca exista riscul sa ajungem in situatia in care nu mai stim la ce cheie este
 * inregistrata o anumita valoare.
 */
void
ht_put(hashtable_t *ht, void *key, unsigned int key_size,
	void *value, unsigned int value_size)
{
	unsigned int pos = ht->hash_function(key);
	/* calculate in range position */
	pos = pos % ht->hmax;

	linked_list_t *bucket = ht->buckets[pos];
	ll_node_t *current = bucket->head;

	while(current) {
		/* modify value if the key is already used */
		if(ht->compare_function(((struct info *)current->data)->key, key) == 0) {
			memcpy(((struct info *)current->data)->value, value, value_size);
			return;
		}
		current = current->next;
	}

	/* key copy */
	void *key_copy = malloc(key_size);
	memcpy(key_copy, key, key_size);

	struct info *new_info = malloc(sizeof(struct info));
	new_info->key = malloc(key_size);
	new_info->value = malloc(value_size);

	/* complete key:value pair */
	memcpy(new_info->value, value, value_size);
	memcpy(new_info->key, key_copy, key_size);

	free(key_copy);

	ll_add_nth_node(bucket, bucket->size, new_info);
	ht->size++;

	free(new_info);
}

void *
ht_get(hashtable_t *ht, void *key)
{
	int pos = ht->hash_function(key);
	/* calculate in range position */
	pos = pos % ht->hmax;
	linked_list_t *bucket = ht->buckets[pos];
	ll_node_t *current = bucket->head;

	while(current) {
		if(ht->compare_function(((struct info *)current->data)->key, key) == 0) {
			return ((struct info*)current->data)->value;
		}
		current = current->next;
	}
	return NULL;
}

void
ht_get_bucket(hashtable_t *ht, void *key)
{
	int pos = ht->hash_function(key);
	pos = pos % ht->hmax;
	linked_list_t *bucket = ht->buckets[pos];
	ll_node_t *current = bucket->head;
	while(current) {
		printf("Name:%s.\n", (char *)(((struct info *)current->data)->value));
		current = current->next;
	}
}

/*
 * Functie care intoarce:
 * 1, daca pentru cheia key a fost asociata anterior o valoare in hashtable folosind functia put
 * 0, altfel.
 */
int
ht_has_key(hashtable_t *ht, void *key)
{
	if(ht_get(ht, key) != NULL) {
		return 1;
	}
	return 0;
}

/*
 * Procedura care elimina din hashtable intrarea asociata cheii key.
 * Atentie! Trebuie avuta grija la eliberarea intregii memorii folosite pentru o intrare din hashtable (adica memoria
 * pentru copia lui key --vezi observatia de la procedura put--, pentru structura info si pentru structura Node din
 * lista inlantuita).
 */
void
ht_remove_entry(hashtable_t *ht, void *key)
{
	int pos = ht->hash_function(key);

	/* calculate in range position */
	pos = pos % ht->hmax;
	linked_list_t *bucket = ht->buckets[pos];

	ll_node_t *current = bucket->head;

	int index = 0; /* to-be-removed index */
	while(current) {
		if(ht->compare_function(((struct info *)current->data)->key, key) == 0) {
			ll_node_t *removed_node = ll_remove_nth_node(bucket, index);
			struct info *node_info = ((struct info *)removed_node->data);
			free(node_info->key);
			free(node_info->value);
			free(removed_node->data);
			free(removed_node);
			break;
		}
		index++;
		current = current->next;
	}
}

/*
 * Procedura care elibereaza memoria folosita de toate intrarile din hashtable, dupa care elibereaza si memoria folosita
 * pentru a stoca structura hashtable.
 */
void
ht_free(hashtable_t *ht)
{
	for(unsigned int i = 0; i < ht->hmax; i++) {
		linked_list_t *bucket = ht->buckets[i]; /* get each list */
		while (ll_get_size(bucket) > 0) {
			ll_node_t *currNode = ll_remove_nth_node(bucket, 0);
			struct info *node_info = ((struct info *)currNode->data);
			free(node_info->value);
			free(node_info->key);
			free(currNode->data);
			free(currNode);
    	}
		free(bucket);
	}

	free(ht->buckets);
	free(ht);
}

unsigned int
ht_get_size(hashtable_t *ht)
{
	if (ht == NULL)
		return 0;

	return ht->size;
}

unsigned int
ht_get_hmax(hashtable_t *ht)
{
	if (ht == NULL)
		return 0;

	return ht->hmax;
}

linked_list_t *
ht_get_bucket_by_id(hashtable_t *ht, int bucket_id){
	int pos = bucket_id;
	linked_list_t *bucket = ht->buckets[pos];
	return bucket;
}
