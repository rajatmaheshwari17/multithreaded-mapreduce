# mutithreaded-mapreduce

This project implements a MapReduce-style multi-threaded data processing library in C. The goal is to provide experience with building a system that leverages multiple threads to perform parallel computations efficiently. The library enables processing large datasets by distributing tasks across multiple threads using the MapReduce programming model.

## Programming Model

In the MapReduce model, the computation involves two main functions: `Map` and `Reduce`.

-   **Map**: Takes an input key-value pair and produces a set of intermediate key-value pairs.
-   **Reduce**: Takes an intermediate key and a set of values and merges them to produce a set of output key-value pairs.

### Example: Word Counting

To illustrate, consider a word-counting problem where the goal is to count the occurrences of each word in a list of text files. The Map function splits the text into words and produces key-value pairs (word, 1). The Reduce function then sums the values for each word.

## Project Details

We implemented the `map_reduce` function in `mr.c` with the following signature:

`void map_reduce(mapper_t mapper, size_t num_mapper, reducer_t reducer,
                size_t num_reducer, kvlist_t* input, kvlist_t* output);` 

-   **mapper**: Function pointer for the map operation.
-   **num_mapper**: Number of threads for the map operation.
-   **reducer**: Function pointer for the reduce operation.
-   **num_reducer**: Number of threads for the reduce operation.
-   **input**: List of input key-value pairs.
-   **output**: List to store the result key-value pairs.

### Data Structures

-   **kvpair_t**: Represents a key-value pair.
-   **kvlist_t**: Represents a list of key-value pairs, implemented as a singly-linked list.
-   **kvlist_iterator_t**: Iterator for traversing a `kvlist_t`.

### Phases of MapReduce

1.  **Split Phase**: Split the input list into smaller lists for each mapper thread.
2.  **Map Phase**: Execute the map function on each smaller list using multiple threads.
3.  **Shuffle Phase**: Group the intermediate key-value pairs by key for the reducers.
4.  **Reduce Phase**: Execute the reduce function on each group using multiple threads.
5.  **Output Phase**: Combine the results into the output list.

## Testing

A sample word-count program is provided to test the `map_reduce` implementation. It counts the occurrences of words in text files using specified numbers of mapper and reducer threads.

### Usage

`./word-count NUM_MAPPER NUM_REDUCER file ...` 

-   `NUM_MAPPER`: Number of threads for the map function.
-   `NUM_REDUCER`: Number of threads for the reduce function.
-   `file ...`: One or more text files.

### Example

`./word-count 1 1 hello.txt`

`hello,1`
`world,1` 

## Setup

**1.  Clone the repository**:
        
    `git clone git@github.com:rajatmaheshwari17/multithreaded-mapreduce.git` 
    
**2.  Build the project**:
    `make` 
    
**3.  Format the source files**:
    `make format`

#
_This README is a part of the multithreaded-mapreduce Project by Rajat Maheshwari._
