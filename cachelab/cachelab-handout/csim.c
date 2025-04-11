#include "cachelab.h"

#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

struct Cache_line // Each cache set contains E cache lines
{
    int valid_bit; // cache line is valid or not
    unsigned long long tag; // check for a match
    unsigned long long lru; // save loaded data
};

struct Cache_set // S, (2 ^ s) cahe sets
{
    struct Cache_line *lines; // size(cache_line) * E
};

struct Cache
{
    int set_bits; // s
    int line_per_set; // E
    int block_bits; // b
    struct Cache_set *sets; // size(cache_set) * (2 ^ s)
};

struct Result
{
    int hits;
    int misses;
    int evictions;
};


struct Cache *cache_init(struct Cache *cache, int s_set_index_bits, int E_lines_per_set, int b_block_offset_bits);
void simulate_cache(FILE* fp, struct Cache* cache, bool verbose_flag, struct Result *result_count);
void update_result_count(int result, struct Result *result_count);
int access_cache(struct Cache* cache, unsigned long long tag, unsigned long long set_index, unsigned long long block_offset, unsigned int* global_lru);
void free_all_cache(struct Cache *cache);
void print_help();
void display_result(int num_result_type);

int main(int argc, char *argv[])
{
    bool help_flag;
    bool verbose_flag;
    char *set_index_bits_string;
    char *lines_per_set;
    char *block_offset_bits;
    char *trace_filename;
    int s_set_index_bits;
    int E_lines_per_set;
    int b_block_offset_bits;
    FILE *trace_file;

    int get_opt_result;
    struct Result result = {0, 0, 0};

    help_flag = false;
    verbose_flag = false;
    
    while ((get_opt_result = getopt(argc, argv, "hvs:E:b:t:")) != -1) // get arguments
    {
        switch (get_opt_result)
        {
        case 'h':
            help_flag = true;
            break;
        case 'v':
            verbose_flag = true;
            break;
        case 's':
            set_index_bits_string = optarg;
            break;
        case 'E':
            lines_per_set = optarg;
            break;
        case 'b':
            block_offset_bits = optarg;
            break;
        case 't':
            trace_filename = optarg;
            break;
        case '?':
            if (optopt == 's' || optopt == 'E' || optopt == 'b' || optopt == 't')
            {
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                print_help();
            }
            else
                fprintf(stderr, "Unknown option -%c. \n", optopt);
            return 1;
        default:
            print_help();
            abort();
        }
    }

    if (help_flag)
    {
        print_help();
        return 0;
    }

    s_set_index_bits = strtol(set_index_bits_string, NULL, 10);
    if (s_set_index_bits <= 0)
    {
        fprintf(stderr, "Invalid number of set index bits.\n");
        return 1;
    }

    E_lines_per_set = strtol(lines_per_set, NULL, 10);
    if (E_lines_per_set <= 0)
    {
        fprintf(stderr, "Invalid number of lines per set.\n");
        return 1;
    }

    b_block_offset_bits = strtol(block_offset_bits, NULL, 10);
    if (b_block_offset_bits <= 0)
    {
        fprintf(stderr, "Invalid number of block offset bits.\n");
        return 1;
    }

    trace_file = fopen(trace_filename, "r");
    if (trace_file == NULL)
    {
        fprintf(stderr, "Cannot open file %s.\n", trace_filename);
        return 1;
    }

    struct Cache temp;
    struct Cache *store = cache_init(&temp, s_set_index_bits, E_lines_per_set, b_block_offset_bits);

    simulate_cache(trace_file, store, verbose_flag, &result);

    free_all_cache(store);

    printSummary(result.hits, result.misses, result.evictions);
    
}

void simulate_cache(FILE* fp, struct Cache* cache, bool verbose_flag, struct Result *result_count)
{
    char operation;
    unsigned long long address;
    unsigned int size;
    int result;
    unsigned int global_lru = 0;
    unsigned long long tag, set_index, block_offset;
    unsigned long long mask_of_set_index = 0, mask_of_block_offset = 0;

    mask_of_set_index = (~0) << (64 - cache->set_bits - cache->block_bits);
    mask_of_set_index = mask_of_set_index >> (64 - cache->set_bits);
    mask_of_set_index = mask_of_set_index << (cache->block_bits);

    // | 000...000 | set index | 000...000 |
    
    mask_of_block_offset = (~0) << (64 - cache->block_bits);
    mask_of_block_offset = mask_of_block_offset >> (64 - cache->block_bits);

    // | 000...000 | 000...000 | block offset |
    
    // read file
    while (fscanf(fp, " %c %llx, %u" , &operation, &address, &size) != EOF) 
    {
        tag = (address) >> (cache->set_bits + cache->block_bits);  // | tag | 000...000 | 000...000 |
        set_index = (address & mask_of_set_index) >> (cache->block_bits);
        block_offset = address & mask_of_block_offset;

        if (operation == 'I') // pass
            continue;

        else if (operation == 'L') 
        { // load
            result = access_cache(cache, tag, set_index, block_offset, &global_lru);
            update_result_count(result, result_count);
            
            if (verbose_flag)
            {
                printf("%c %llx, %u ", operation, address, size);
                display_result(result);
                printf("\n");
            }
        }

        else if (operation == 'M') 
        { // modify
            result = access_cache(cache, tag, set_index, block_offset, &global_lru);
            update_result_count(result, result_count);
            if (verbose_flag) 
            {
                printf("%c %llx, %u ", operation, address, size);
                display_result(result);
            }
            result = access_cache(cache, tag, set_index, block_offset, &global_lru);
            update_result_count(result, result_count);
            if (verbose_flag) 
            {
                display_result(result);
                printf("\n");
            }
        }

        else if (operation == 'S') 
        { // store
            result = access_cache(cache, tag, set_index, block_offset, &global_lru);
            update_result_count(result, result_count);
            if (verbose_flag) 
            {
                printf("%c %llx, %u ", operation, address, size);
                display_result(result);
                printf("\n");
            }
        }
    }
    return;
}

int access_cache(struct Cache* cache, unsigned long long tag, unsigned long long set_index, unsigned long long block_offset, unsigned int* global_lru) 
{
    unsigned int min_lru = 0xFFFFFFFF;
    int target_index = 0;
    // hit
    for (int i = 0; i < cache->line_per_set; i++)
    {
        if (cache->sets[set_index].lines[i].valid_bit == 1)
        {
            if (cache->sets[set_index].lines[i].tag == tag)
            {
                cache->sets[set_index].lines[i].lru = *global_lru;
                (*global_lru)++;
                return 0;
            }
        }
    }
    
    // cold miss
    for (int i = 0; i < cache->line_per_set; i++)
    {
        if (cache->sets[set_index].lines[i].valid_bit == 0)
        {
            cache->sets[set_index].lines[i].valid_bit = 1;
            cache->sets[set_index].lines[i].tag = tag;
            cache->sets[set_index].lines[i].lru = *global_lru;
            (*global_lru)++;
            return 1;
        }
    }
    
    // eviction miss
    for (int i = 0; i < cache->line_per_set; i++)
    {
        if (cache->sets[set_index].lines[i].lru < min_lru)
        {
            min_lru = cache->sets[set_index].lines[i].lru;
            target_index = i;
        }
    }
    
    cache->sets[set_index].lines[target_index].tag = tag;
    cache->sets[set_index].lines[target_index].lru = *global_lru;
    (*global_lru)++;
    
    return 2;
}

struct Cache *cache_init(struct Cache *cache, int s_set_index_bits, int E_lines_per_set, int b_block_offset_bits)
{
    // allocate memory for cache
    cache = malloc(sizeof(struct Cache));

    // metadata
    cache->set_bits = s_set_index_bits;
    cache->line_per_set = E_lines_per_set;
    cache->block_bits = b_block_offset_bits;

    // allocate memory for cache sets
    cache->sets = calloc(1 << s_set_index_bits, sizeof(struct Cache_set));

    // allocate memory for cache lines
    for (int i = 0; i < (1 << s_set_index_bits); i++)
    {
        cache->sets[i].lines = calloc(E_lines_per_set, sizeof(struct Cache_line));
    }

    return cache;
}

void update_result_count(int result, struct Result *result_count) 
{
    if (result == 0) 
    {
        result_count->hits += 1;
    } 
    else if (result == 1) 
    {
        result_count->misses += 1;
    } 
    else if (result == 2) 
    {
        result_count->misses += 1;
        result_count->evictions += 1;
    }
}

void display_result(int num_result_type)
{
    if (num_result_type == 0)
        printf("hit\n");
    else if (num_result_type == 1)
        printf("miss\n");
    else if (num_result_type == 2)
        printf("miss eviction\n");
}

void free_all_cache(struct Cache *cache) 
{
    for (int i = 0; i < (1 << cache->set_bits); i++) 
    {
        free(cache->sets[i].lines);
    }
    free(cache->sets);
    free(cache);
}

void print_help()
{
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\n");
    printf("Examples:\n");
    printf("  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}