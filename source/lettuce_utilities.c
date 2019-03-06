#define MemoryCopy memcpy

static char *
LoadEntireFileAndNullTerminate(char *filename)
{
    char *result = 0;
    
    FILE *file = fopen(filename, "r");
    if(file)
    {
        fseek(file, 0, SEEK_END);
        unsigned int file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        result = malloc(file_size+1);
        if(result)
        {
            fread(result, file_size, 1, file);
            result[file_size] = 0;
        }
    }
    
    return result;
}

static int
CharIsAlpha(int c)
{
    return ((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z'));
}

static int
CharIsNumeric(int c)
{
    return (c >= '0' && c <= '9');
}

static int
CharIsSymbolic(int c)
{
    return (c == '!' ||
            c == '@' ||
            c == '#' ||
            c == '$' ||
            c == '%' ||
            c == '^' ||
            c == '&' ||
            c == '*' ||
            c == '(' ||
            c == ')' ||
            c == '-' ||
            c == '+' ||
            c == '=' ||
            c == '[' ||
            c == ']' ||
            c == '{' ||
            c == '}' ||
            c == '|' ||
            c == '\\' ||
            c == ';' ||
            c == ':' ||
            c == '<' ||
            c == '>' ||
            c == ',' ||
            c == '.' ||
            c == '?' ||
            c == '/');
}

static int
StringMatch(char *string1, int string1_length,
            char *string2, int string2_length)
{
    int result = 0;
    
    if(string1 && string2 && string1_length == string2_length)
    {
        result = 1;
        for(int i = 0; i < string1_length; ++i)
        {
            if(string1[i] != string2[i])
            {
                result = 0;
                break;
            }
        }
    }
    
    return result;
}

static unsigned int
CalculateCStringLength(char *str)
{
    unsigned int result = 0;
    for(; str[result++];);
    --result;
    return result;
}

#define MEMORY_ARENA_CHUNK_SIZE 1024

typedef struct MemoryArenaChunk MemoryArenaChunk;
typedef struct MemoryArenaChunk
{
    unsigned int memory_size;
    unsigned int memory_alloc_pos;
    void *memory;
    MemoryArenaChunk *next;
}
MemoryArenaChunk;

typedef struct MemoryArena
{
    MemoryArenaChunk first_chunk;
    MemoryArenaChunk *active_chunk;
}
MemoryArena;

static void
MemoryArenaCleanUp(MemoryArena *arena)
{
    free(arena->first_chunk.memory);
    for(MemoryArenaChunk *chunk = arena->first_chunk.next;
        chunk;)
    {
        MemoryArenaChunk *next_chunk = chunk->next;
        free(chunk);
        chunk = next_chunk;
    }
}

static void *
MemoryArenaAllocate(MemoryArena *arena, unsigned int size)
{
    void *result = 0;
    
    if(!arena->active_chunk)
    {
        arena->active_chunk = &arena->first_chunk;
    }
    
    MemoryArenaChunk *chunk = arena->active_chunk;
    
    if(!chunk->memory)
    {
        chunk->memory_size = MEMORY_ARENA_CHUNK_SIZE;
        if(chunk->memory_size < size)
        {
            chunk->memory_size = size;
        }
        chunk->memory = malloc(chunk->memory_size);
        chunk->memory_alloc_pos = 0;
        chunk->next = 0;
    }
    
    if(chunk->memory_alloc_pos + size > chunk->memory_size)
    {
        unsigned int needed_size = MEMORY_ARENA_CHUNK_SIZE;
        if(needed_size < size)
        {
            needed_size = size;
        }
        MemoryArenaChunk *new_chunk = malloc(sizeof(MemoryArenaChunk) + needed_size);
        chunk->next = new_chunk;
        new_chunk->memory = (char *)new_chunk + sizeof(MemoryArenaChunk);
        new_chunk->memory_size = needed_size;
        new_chunk->memory_alloc_pos = 0;
        new_chunk->next = 0;
        chunk = chunk->next;
        arena->active_chunk = chunk;
    }
    
    result = (char *)chunk->memory + chunk->memory_alloc_pos;
    chunk->memory_alloc_pos += size;
    
    return result;
}
