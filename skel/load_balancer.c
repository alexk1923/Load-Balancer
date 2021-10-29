/* Copyright 2021 Kullman Alexandru 313CA */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "./load_balancer.h"
#include "./server.h"
#include "./Hashtable.h"
#include "./utils.h"

#define MAX_SV 99999
#define MAX_HT 20000
#define REPLICA_MULTIPLIER 100000

struct load_balancer
{
    server_memory **svm; /* array of servers */
    int no_servers;
    int no_labels;
    int *label_array; /* array of labels aka hashring */
};

unsigned int hash_function_servers(void *a)
{
    unsigned int uint_a = *((unsigned int *)a);

    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = (uint_a >> 16u) ^ uint_a;
    return uint_a;
}

unsigned int hash_function_key(void *a)
{
    unsigned char *puchar_a = (unsigned char *)a;
    unsigned int hash = 5381;
    int c;

    while ((c = *puchar_a++))
        hash = ((hash << 5u) + hash) + c;

    return hash;
}

load_balancer *init_load_balancer()
{
    load_balancer *lb = malloc(sizeof(load_balancer));
    DIE(lb == NULL, "malloc() failed");

    lb->svm = calloc(MAX_SV, sizeof(server_memory *));
    DIE(lb->svm == NULL, "calloc() failed");

    lb->label_array = malloc(3 * MAX_SV * sizeof(int));
    DIE(lb->label_array == NULL, "malloc() failed");
    lb->no_servers = 0;
    lb->no_labels = 0;
    return lb;
}

/* Function to print hashring array from a load_balancer */
void print_labels(load_balancer *main)
{
    for (int i = 0; i < main->no_labels; i++)
    {
        printf("%d ", main->label_array[i]);
    }
}

void loader_store(load_balancer *main, char *key, char *value, int *server_id)
{
    unsigned int hash_key = hash_function_key(key);
    int i;
    int server_id_store;

    for (i = 0; i < main->no_labels; i++)
    {
        /* Compare the hash of the key with each hash from the labels array and get the
     * server. Add the pair to the found server
     */
        if (hash_key < hash_function_servers(&main->label_array[i]))
        {
            server_id_store = main->label_array[i] % REPLICA_MULTIPLIER;
            server_store(main->svm[server_id_store], key, value);
            break;
        }
    }

    /* If the key's hash is greater than all hashes from the hashring, add it
     * into the server corresponding to the first label
     */
    if (i == main->no_labels)
    {
        server_id_store = main->label_array[0] % REPLICA_MULTIPLIER;
        server_store(main->svm[server_id_store], key, value);
    }
    *server_id = server_id_store;
}

char *loader_retrieve(load_balancer *main, char *key, int *server_id)
{
    unsigned int hash_key = hash_function_key(key);
    int i, server_id_ret;
    char *value;
    for (i = 0; i < main->no_labels; i++)
    {
        /* Compare the hash of the key with each hash from the labels array and get the
     * server. Get the value from the hashtable of the server corrsponding to
     * the key
     */
        if (hash_key < hash_function_servers(&main->label_array[i]))
        {
            server_id_ret = main->label_array[i] % REPLICA_MULTIPLIER;
            value = server_retrieve(main->svm[server_id_ret], key);
            break;
        }
    }

    /* If the key's hash is greater than all hashes from the hashring, retrieve
     * that value from the server corresponding to the first label
     */
    if (i == main->no_labels)
    {
        server_id_ret = main->label_array[0] % REPLICA_MULTIPLIER;
        value = (char *)ht_get(main->svm[server_id_ret]->ht, key);
    }

    *server_id = server_id_ret;

    return value;
}

/* Function to verify if an object needs to be moved from one server
 * to another
 */
void update_objects(load_balancer *main, int pos)
{
    int server_id;

    /* Get the server from which it will be verified a possible update */
    if (pos == main->no_labels - 1)
    {
        server_id = main->label_array[0] % REPLICA_MULTIPLIER;
    }
    else
    {
        server_id = main->label_array[pos + 1] % REPLICA_MULTIPLIER;
    }

    server_memory *sv = main->svm[server_id];
    for (unsigned int i = 0; i < sv->ht->hmax; i++)
    {
        linked_list_t *list = ht_get_bucket_by_id(sv->ht, i);
        ll_node_t *current = list->head;
        while (current)
        {
            ll_node_t *next_node = current->next;
            /* Get each pair of (key:value) from the server's hashtable
             * from bucket "i"
             */
            struct info *info = ((struct info *)current->data);
            char *key = (char *)info->key;
            char *value = (char *)info->value;
            unsigned int hash_key = hash_function_key(key);

            /* ok = 1 if the object will be moved, 0 otherwise */
            int ok = 1;

            /* If the new_label is added as the first one in
             * the hashring array
             */
            if (pos == 0)
            {
                for (int k = 1; k < main->no_labels; k++)
                {
                    if (hash_key <
                        hash_function_servers(&main->label_array[k]))
                    {
                        /* If the hash of the key is NOT greater than all
                         * hashes from the hashring, then it will remain on
                         * the same server
                         */
                        ok = 0;
                        break;
                    }
                }
                /* Verify if hash_key is lower than the hash of the new label
                 * added as the first one in hashring array
                 */
                if (hash_key < hash_function_servers(&main->label_array[0]))
                    ok = 1;
            }
            else
            {
                for (int k = 0; k < pos; k++)
                {
                    if (hash_key <
                        hash_function_servers(&main->label_array[k]))
                    {
                        /* If it found that the hash of the key is lower than a
                        * hash of a label from the hashring which is before the
                        * new_label, then the object will remain on the same
                        * server. (If the condition is not met, then it means
                        * that hash_key > all hashes of the labels before the
                        * new label)
                        */
                        ok = 0;
                        break;
                    }
                }
            }
            /* If the object is likely to be moved/updated */
            if (ok == 1)
            {
                unsigned int hash_new_label =
                    hash_function_servers(&main->label_array[pos]);
                /* If ok = 1 and pos = 0, then the object will be moved or
                 * if ok = 1, pos != 0, then verify if hash of the key is
                 * lower than hash of the new label, because it could also be
                 * greater than the hash of the new label (then is no need to
                 * be moved on another server)
                 */
                if (hash_key < hash_new_label || pos == 0)
                {
                    char *new_key = malloc(strlen(key) + 1);
                    DIE(new_key == NULL, "malloc() failed");
                    char *new_value = malloc(strlen(value) + 1);
                    DIE(new_value == NULL, "malloc() failed");

                    /* Make copies for key and value, old key
                     * and value will be freed
                     */
                    memcpy(new_value, value, strlen(value) + 1);
                    memcpy(new_key, key, strlen(key) + 1);

                    /* Get the new server id */
                    int new_server_id = main->label_array[pos] % REPLICA_MULTIPLIER;
                    /* If the new server is NOT the same as the old one */
                    if (new_server_id != server_id)
                    {
                        server_store(main->svm[new_server_id],
                                     new_key, new_value);
                        server_remove(sv, key);
                    }

                    /* Free new_key and new_value after they have been added as an object
                     * in the new server
                     */
                    free(new_key);
                    free(new_value);
                }
            }
            /* Go to the next object to be verified */
            current = next_node;
        }
    }
}

void loader_add_server(load_balancer *main, int server_id)
{
    /* Initialise new server */
    server_memory *new_server = init_server_memory();

    /* Add server into the array */
    main->svm[server_id] = new_server;

    /* Add labels for the server itself and its replicas */
    for (int i = 0; i < 3; i++)
    {
        int label = i * REPLICA_MULTIPLIER + server_id;
        unsigned int label_hash = hash_function_servers(&label);
        int j;
        for (j = 0; j < main->no_labels; j++)
        {
            if (label_hash < hash_function_servers(&main->label_array[j]))
            {
                /* Swap elements of the hashring array to the right */
                for (int k = main->no_labels - 1; k >= j; k--)
                {
                    main->label_array[k + 1] = main->label_array[k];
                }

                /* Add the label on the hashring */
                main->label_array[j] = label;
                main->no_labels++;
                update_objects(main, j);
                break;
            }
        }

        /* Verify if the label's hash value is greater
         * than all hashes from the hashring
         */
        if (j == main->no_labels)
        {
            main->label_array[j] = label;
            main->no_labels++;
            update_objects(main, j);
        }
    }
    main->no_servers++;
}

void loader_remove_server(load_balancer *main, int server_id)
{
    /* Remove all labels corresponding to the server*/
    for (int i = 0; i < main->no_labels; i++)
    {
        if (main->label_array[i] % REPLICA_MULTIPLIER == server_id)
        {
            for (int j = i; j < main->no_labels - 1; j++)
            {
                main->label_array[j] = main->label_array[j + 1];
            }
            i--;
            main->no_labels--;
        }
    }

    server_memory *sv = main->svm[server_id];

    for (unsigned int i = 0; i < sv->ht->hmax; i++)
    {
        linked_list_t *list = ht_get_bucket_by_id(sv->ht, i);
        ll_node_t *current = list->head;
        while (current)
        {
            ll_node_t *next_node = current->next;
            /* Get each pair of (key:value) from the server's hashtable
             * from bucket "i"
             */
            struct info *info = ((struct info *)current->data);
            char *key = (char *)info->key;
            char *value = (char *)info->value;

            char *new_key = malloc(strlen(key) + 1);
            DIE(new_key == NULL, "malloc() failed");
            char *new_value = malloc(strlen(value) + 1);
            DIE(new_value == NULL, "malloc() failed");

            /* Make copies for key and value, old key and value will be freed */
            memcpy(new_value, value, strlen(value) + 1);
            memcpy(new_key, key, strlen(key) + 1);

            /* Remove object from server's hashtable */
            server_remove(sv, key);

            /* Find the new server to store the pair of new_key:new_value */
            int j;
            for (j = 0; j < main->no_labels; j++)
            {
                unsigned int hash_key = hash_function_key(new_key);
                if (hash_key < hash_function_servers(&main->label_array[j]))
                {
                    int server_id_new = main->label_array[j] %
                                        REPLICA_MULTIPLIER;
                    /* Store the object on the new server */
                    server_store(main->svm[server_id_new], new_key, new_value);
                    break;
                }
            }

            /* If the hash of the new_key is greater than all
             * hashes of the elements from the hashring
             */
            if (j == main->no_labels)
            {
                int server_id_new = main->label_array[0] % REPLICA_MULTIPLIER;
                server_store(main->svm[server_id_new], new_key, new_value);
            }
            current = next_node;

            /* Free new_key and new_value after they have been added as an object
        * in the new server
        */
            free(new_key);
            free(new_value);
        }
    }
}

void free_load_balancer(load_balancer *main)
{
    free(main->label_array);

    /* Free each server from the array */
    for (int i = 0; i < MAX_SV; i++)
    {
        if (main->svm[i] != NULL)
            free_server_memory(main->svm[i]);
    }
    free(main->svm);
    free(main);
}
