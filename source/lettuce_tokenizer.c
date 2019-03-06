
enum
{
    TOKEN_invalid,
    TOKEN_alphanumeric_block,
    TOKEN_numeric_constant,
    TOKEN_symbolic_block, 
};

typedef struct Token
{
    int type;
    char *string;
    int string_length;
}
Token;

static Token
GetNextTokenFromBuffer(char *buffer)
{
    Token token = {0};
    
    for(int i = 0; buffer[i]; ++i)
    {
        int j;
        
        if(CharIsAlpha(buffer[i]) || buffer[i] == '_')
        {
            for(j = i+1; buffer[j]; ++j)
            {
                if(!CharIsAlpha(buffer[j]) && !CharIsNumeric(buffer[j]) &&
                   buffer[j] != '_')
                {
                    break;
                }
            }
            
            token.type = TOKEN_alphanumeric_block;
            token.string = buffer+i;
            token.string_length = j-i;
            break;
        }
        else if(CharIsNumeric(buffer[i]))
        {
            for(j = i+1; buffer[j]; ++j)
            {
                if(!CharIsAlpha(buffer[j]) && !CharIsNumeric(buffer[j]) &&
                   buffer[j] != '.')
                {
                    break;
                }
            }
            
            token.type = TOKEN_numeric_constant;
            token.string = buffer+i;
            token.string_length = j-i;
            break;
        }
        else if(CharIsSymbolic(buffer[i]))
        {
            for(j = i+1; buffer[j]; ++j)
            {
                if(!CharIsSymbolic(buffer[j]))
                {
                    break;
                }
            }
            
            token.type = TOKEN_symbolic_block;
            token.string = buffer+i;
            token.string_length = j-i;
            break;
        }
    }
    
    return token;
}

typedef struct Tokenizer
{
    char *at;
}
Tokenizer;

static int
TokenMatchCString(Token a, char *b)
{
    int matches = 1;
    for(int i = 0; i < a.string_length; ++i)
    {
        if(a.string[i] != b[i])
        {
            matches = 0;
            break;
        }
        
        if(b[i+1] == 0)
        {
            break;
        }
        
    }
    return matches;
}

static Token
PeekToken(Tokenizer *tokenizer)
{
    return GetNextTokenFromBuffer(tokenizer->at);
}

static void
NextToken(Tokenizer *tokenizer, Token *token_ptr)
{
    Token token = GetNextTokenFromBuffer(tokenizer->at);
    if(token.type)
    {
        tokenizer->at = token.string + token.string_length;
        if(token_ptr)
        {
            *token_ptr = token;
        }
    }
}

static int
RequireTokenMatch(Tokenizer *tokenizer, char *string, Token *matched_token)
{
    int match = 0;
    Token token = PeekToken(tokenizer);
    match = TokenMatchCString(token, string);
    if(match)
    {
        tokenizer->at = token.string + token.string_length;
        if(matched_token)
        {
            *matched_token = token;
        }
    }
    return match;
}

static int
RequireTokenType(Tokenizer *tokenizer, int type, Token *matched_token)
{
    int match = 0;
    Token token = PeekToken(tokenizer);
    match = token.type == type;
    if(match)
    {
        tokenizer->at = token.string + token.string_length;
        if(matched_token)
        {
            *matched_token = token;
        }
    }
    return match;
}

static double
TokenToDouble(Token token)
{
    char str[128] = {0};
    for(int i = 0; i < token.string_length && i < 127; ++i)
    {
        str[i] = token.string[i];
    }
    return atof(str);
}
