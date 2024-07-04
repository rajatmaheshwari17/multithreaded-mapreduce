#define _POSIX_C_SOURCE 200809L
#include "mr.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kvlist.h"

typedef struct {
  kvlist_t *input;
  kvlist_t **intermediate;
  mapper_t mapper;
  reducer_t reducer;
  pthread_mutex_t *locks;
  kvlist_t *output;
  int start_index;
  int end_index;
  size_t num_reducers;
} ThreadData;

size_t kvlist_size(kvlist_t *list) {
  size_t count = 0;
  kvlist_iterator_t *it = kvlist_iterator_new(list);
  while (kvlist_iterator_next(it) != NULL) {
    count++;
  }
  kvlist_iterator_free(&it);
  return count;
}

void *mapper_thread(void *arg) {
  ThreadData *data = (ThreadData *)arg;
  kvlist_iterator_t *it = kvlist_iterator_new(data->input);
  kvpair_t *pair;
  int index = 0;
  kvlist_t *local_output = kvlist_new();

  while ((pair = kvlist_iterator_next(it)) != NULL) {
    if (index >= data->start_index && index < data->end_index) {
      data->mapper(pair, local_output);
    }
    index++;
  }
  kvlist_iterator_free(&it);

  for (size_t i = 0; i < data->num_reducers; i++) {
    pthread_mutex_lock(&data->locks[i]);
    kvlist_extend(data->intermediate[i], local_output);
    pthread_mutex_unlock(&data->locks[i]);
  }

  kvlist_free(&local_output);
  return NULL;
}

void *reducer_thread(void *arg) {
  ThreadData *data = (ThreadData *)arg;
  kvlist_sort(data->input);
  kvlist_iterator_t *it = kvlist_iterator_new(data->input);
  kvpair_t *pair;
  kvlist_t *current_pairs = kvlist_new();
  char *last_key = NULL;

  while ((pair = kvlist_iterator_next(it)) != NULL) {
    if (last_key == NULL || strcmp(last_key, pair->key) != 0) {
      if (last_key) {
        data->reducer(last_key, current_pairs, data->output);
        kvlist_free(&current_pairs);
        current_pairs = kvlist_new();
        free(last_key);
      }
      last_key = strdup(pair->key);
    }
    kvlist_append(current_pairs, kvpair_clone(pair));
  }

  if (last_key) {
    data->reducer(last_key, current_pairs, data->output);
    free(last_key);
  }
  kvlist_free(&current_pairs);
  kvlist_iterator_free(&it);
  return NULL;
}

void map_reduce(mapper_t mapper, size_t num_mapper, reducer_t reducer,
                size_t num_reducer, kvlist_t *input, kvlist_t *output) {
  pthread_t *map_threads = malloc(num_mapper * sizeof(pthread_t));
  pthread_t *reduce_threads = malloc(num_reducer * sizeof(pthread_t));
  ThreadData *map_data = malloc(num_mapper * sizeof(ThreadData));
  ThreadData *reduce_data = malloc(num_reducer * sizeof(ThreadData));
  pthread_mutex_t *locks = malloc(num_reducer * sizeof(pthread_mutex_t));
  kvlist_t **intermediates = malloc(num_reducer * sizeof(kvlist_t *));

  for (size_t i = 0; i < num_reducer; i++) {
    pthread_mutex_init(&locks[i], NULL);
    intermediates[i] = kvlist_new();
  }

  size_t input_size = kvlist_size(input);
  int items_per_thread = input_size / num_mapper;

  for (size_t i = 0; i < num_mapper; i++) {
    map_data[i] = (ThreadData){.input = input,
                               .intermediate = intermediates,
                               .mapper = mapper,
                               .locks = locks,
                               .start_index = i * items_per_thread,
                               .end_index = (i + 1) * items_per_thread,
                               .num_reducers = num_reducer};
    if (i == num_mapper - 1) {
      map_data[i].end_index = input_size;
    }
    pthread_create(&map_threads[i], NULL, mapper_thread, &map_data[i]);
  }

  for (size_t i = 0; i < num_mapper; i++) {
    pthread_join(map_threads[i], NULL);
  }

  for (size_t i = 0; i < num_reducer; i++) {
    reduce_data[i] = (ThreadData){.input = intermediates[i],
                                  .output = output,
                                  .reducer = reducer,
                                  .locks = locks,
                                  .start_index = 0,
                                  .end_index = kvlist_size(intermediates[i])};
    pthread_create(&reduce_threads[i], NULL, reducer_thread, &reduce_data[i]);
  }

  for (size_t i = 0; i < num_reducer; i++) {
    pthread_join(reduce_threads[i], NULL);
    kvlist_free(&intermediates[i]);
    pthread_mutex_destroy(&locks[i]);
  }

  free(map_threads);
  free(map_data);
  free(reduce_threads);
  free(reduce_data);
  free(intermediates);
  free(locks);
}