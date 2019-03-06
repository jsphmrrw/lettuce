
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
