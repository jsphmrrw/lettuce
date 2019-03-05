#include <stdlib.h>
#include <stdio.h>

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

enum
{
    BINARY_OPERATOR_invalid,
    BINARY_OPERATOR_plus,
    BINARY_OPERATOR_minus,
    BINARY_OPERATOR_multiply,
    BINARY_OPERATOR_divide,
};

static int
TokenToBinaryOperator(Token token)
{
    int type = BINARY_OPERATOR_invalid;
    if(TokenMatchCString(token, "+"))
    {
        type = BINARY_OPERATOR_plus;
    }
    else if(TokenMatchCString(token, "-"))
    {
        type = BINARY_OPERATOR_minus;
    }
    else if(TokenMatchCString(token, "*"))
    {
        type = BINARY_OPERATOR_multiply;
    }
    else if(TokenMatchCString(token, "/"))
    {
        type = BINARY_OPERATOR_divide;
    }
    return type;
}

enum
{
    ABSTRACT_SYNTAX_TREE_NODE_let,
    ABSTRACT_SYNTAX_TREE_NODE_identifier,
    ABSTRACT_SYNTAX_TREE_NODE_numeric_constant,
    ABSTRACT_SYNTAX_TREE_NODE_boolean_constant,
    ABSTRACT_SYNTAX_TREE_NODE_binary_operator,
    ABSTRACT_SYNTAX_TREE_NODE_unary_operator,
};

typedef struct AbstractSyntaxTreeNode AbstractSyntaxTreeNode;
typedef struct AbstractSyntaxTreeNode
{
    int type;
    union
    {
        
        struct Let
        {
            char *string;
            int string_length;
            AbstractSyntaxTreeNode *binding_expression;
            AbstractSyntaxTreeNode *body_expression;
        }
        let;
        
        struct Identifier
        {
            char *string;
            int string_length;
        }
        identifier;
        
        struct NumericConstant
        {
            double value;
        }
        numeric_constant;
        
        struct BooleanConstant
        {
            int value;
        }
        boolean_constant;
        
        struct BinaryOperator
        {
            int type;
            AbstractSyntaxTreeNode *left;
            AbstractSyntaxTreeNode *right;
        }
        binary_operator;
        
        struct UnaryOperator
        {
            int type;
            AbstractSyntaxTreeNode *expression;
        }
        unary_operator;
        
    };
}
AbstractSyntaxTreeNode;

#define PARSE_CONTEXT_MEMORY_CHUNK_SIZE 1024

typedef struct ParseContextMemoryChunk ParseContextMemoryChunk;
typedef struct ParseContextMemoryChunk
{
    unsigned int memory_size;
    unsigned int memory_alloc_pos;
    void *memory;
    ParseContextMemoryChunk *next;
}
ParseContextMemoryChunk;

typedef struct ParseContext
{
    ParseContextMemoryChunk first_chunk;
    ParseContextMemoryChunk *active_chunk;
}
ParseContext;

static void *
ParseContextAllocate(ParseContext *context, unsigned int size)
{
    void *result = 0;
    
    if(!context->active_chunk)
    {
        context->active_chunk = &context->first_chunk;
    }
    
    ParseContextMemoryChunk *chunk = context->active_chunk;
    
    if(!chunk->memory)
    {
        chunk->memory_size = PARSE_CONTEXT_MEMORY_CHUNK_SIZE;
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
        unsigned int needed_size = PARSE_CONTEXT_MEMORY_CHUNK_SIZE;
        if(needed_size < size)
        {
            needed_size = size;
        }
        ParseContextMemoryChunk *new_chunk = malloc(sizeof(ParseContextMemoryChunk) + needed_size);
        chunk->next = new_chunk;
        new_chunk->memory = (char *)new_chunk + sizeof(ParseContextMemoryChunk);
        new_chunk->memory_size = needed_size;
        new_chunk->memory_alloc_pos = 0;
        new_chunk->next = 0;
        chunk = chunk->next;
        context->active_chunk = chunk;
    }
    
    result = (char *)chunk->memory + chunk->memory_alloc_pos;
    chunk->memory_alloc_pos += size;
    
    return result;
}

static AbstractSyntaxTreeNode *
ParseContextAllocateNode(ParseContext *context)
{
    return ParseContextAllocate(context, sizeof(AbstractSyntaxTreeNode));
}

static AbstractSyntaxTreeNode *
ParseExpression(Tokenizer *tokenizer, ParseContext *context)
{
    AbstractSyntaxTreeNode *result = 0;
    
    Token token = PeekToken(tokenizer);
    
    if(TokenMatchCString(token, "let"))
    {
        // NOTE(rjf): A let binding.
        NextToken(tokenizer, 0);
        Token identifier;
        
        if(RequireTokenType(tokenizer, TOKEN_alphanumeric_block, &identifier) &&
           RequireTokenMatch(tokenizer, "=", 0))
        {
            
        }
        else
        {
            // NOTE(rjf): ERROR, identifier not found for let expression
        }
        
        AbstractSyntaxTreeNode *let = ParseContextAllocateNode(context);
        let->type = ABSTRACT_SYNTAX_TREE_NODE_let;
        let->let.string = identifier.string;
        let->let.string_length = identifier.string_length;
        let->let.binding_expression = ParseExpression(tokenizer, context);
        
        Token in;
        
        if(RequireTokenMatch(tokenizer, "in", &in))
        {
            let->let.body_expression = ParseExpression(tokenizer, context);
        }
        else
        {
            // NOTE(rjf): ERROR, required "in"
        }
        
        result = let;
    }
    else if(token.type == TOKEN_alphanumeric_block)
    {
        // NOTE(rjf): In this case, we must have an identifier being used.
        
        NextToken(tokenizer, 0);
        
        AbstractSyntaxTreeNode *val = ParseContextAllocateNode(context);
        val->type = ABSTRACT_SYNTAX_TREE_NODE_identifier;
        val->identifier.string = token.string;
        val->identifier.string_length = token.string_length;
        result = val;
    }
    else if(token.type == TOKEN_numeric_constant)
    {
        // NOTE(rjf): This can either be a naked numeric constant (some leaf
        //            of the AST) or it can be part of a larger expression.
        
        NextToken(tokenizer, 0);
        
        AbstractSyntaxTreeNode *val = ParseContextAllocateNode(context);
        val->type = ABSTRACT_SYNTAX_TREE_NODE_numeric_constant;
        val->numeric_constant.value = TokenToDouble(token);
        result = val;
    }
    else if(token.type == TOKEN_symbolic_block)
    {
        // NOTE(rjf): A symbolic block that exists independently of a preceding
        //            identifier or numeric constant must be a unary operator,
        //            so we'll handle those here.
        NextToken(tokenizer, 0);
    }
    else
    {
        // NOTE(rjf): We have no idea what we are looking at, so ERROR
    }
    
    token = PeekToken(tokenizer);
    
    if(token.type == TOKEN_symbolic_block)
    {
        // NOTE(rjf): We could have a binary operator here.
        NextToken(tokenizer, 0);
        
        int operator_type = TokenToBinaryOperator(token);
        if(operator_type != BINARY_OPERATOR_invalid)
        {
            AbstractSyntaxTreeNode *binary_operator = ParseContextAllocateNode(context);
            binary_operator->type = ABSTRACT_SYNTAX_TREE_NODE_binary_operator;
            binary_operator->binary_operator.type = operator_type;
            binary_operator->binary_operator.left = result;
            binary_operator->binary_operator.right = ParseExpression(tokenizer, context);
            result = binary_operator;
        }
        else
        {
            // NOTE(rjf): ERROR! Symbol was not a binary operator, so we really aren't
            //            sure what it is.
        }
    }
    
    return result;
}

static void
PrintAbstractSyntaxTree(AbstractSyntaxTreeNode *root)
{
    switch(root->type)
    {
        case ABSTRACT_SYNTAX_TREE_NODE_let:
        {
            printf("let %.*s = (", root->let.string_length, root->let.string);
            PrintAbstractSyntaxTree(root->let.binding_expression);
            printf(") in (");
            PrintAbstractSyntaxTree(root->let.body_expression);
            printf(")\n\n");
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_identifier:
        {
            printf("%.*s", root->identifier.string_length, root->identifier.string);
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_numeric_constant:
        {
            printf("%f", root->numeric_constant.value);
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_binary_operator:
        {
            printf("(");
            PrintAbstractSyntaxTree(root->binary_operator.left);
            
            if(root->binary_operator.type == BINARY_OPERATOR_plus)
            { printf(" + "); }
            else if(root->binary_operator.type == BINARY_OPERATOR_minus)
            { printf(" - "); }
            else if(root->binary_operator.type == BINARY_OPERATOR_multiply)
            { printf(" * "); }
            else if(root->binary_operator.type == BINARY_OPERATOR_divide)
            { printf(" / "); }
            
            PrintAbstractSyntaxTree(root->binary_operator.right);
            printf(")");
            break;
        }
        default: break;
    }
}

enum
{
    EVALUATION_RESULT_number,
    EVALUATION_RESULT_boolean,
};

typedef struct EvaluationResult
{
    int type;
    union
    {
        double number;
        int boolean;
    };
}
EvaluationResult;

static EvaluationResult
EvaluateAbstractSyntaxTree(AbstractSyntaxTreeNode *root)
{
    EvaluationResult result = {0};
    
    switch(root->type)
    {
        case ABSTRACT_SYNTAX_TREE_NODE_let:
        {
            
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_identifier:
        {
            
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_numeric_constant:
        {
            result.type = EVALUATION_RESULT_number;
            result.number = root->numeric_constant.value;
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_binary_operator:
        {
            EvaluationResult left_eval = EvaluateAbstractSyntaxTree(root->binary_operator.left);
            EvaluationResult right_eval = EvaluateAbstractSyntaxTree(root->binary_operator.right);
            
            if(root->binary_operator.type == BINARY_OPERATOR_plus)
            { 
                result.type = EVALUATION_RESULT_number;
                result.number = left_eval.number + right_eval.number;
            }
            else if(root->binary_operator.type == BINARY_OPERATOR_minus)
            {
                result.type = EVALUATION_RESULT_number;
                result.number = left_eval.number - right_eval.number;
            }
            else if(root->binary_operator.type == BINARY_OPERATOR_multiply)
            {
                result.type = EVALUATION_RESULT_number;
                result.number = left_eval.number * right_eval.number;
            }
            else if(root->binary_operator.type == BINARY_OPERATOR_divide)
            {
                result.type = EVALUATION_RESULT_number;
                result.number = left_eval.number / right_eval.number;
            }
            
            break;
        }
        default: break;
    }
    
    return result;
}

static void
InterpretCode(char *code)
{
    Tokenizer tokenizer_ = {0};
    ParseContext context_ = {0};
    
    Tokenizer *tokenizer = &tokenizer_;
    tokenizer->at = code;
    ParseContext *context = &context_;
    
    AbstractSyntaxTreeNode *root = ParseExpression(tokenizer, context);
    PrintAbstractSyntaxTree(root);
    printf("\n");
    
    EvaluationResult result = EvaluateAbstractSyntaxTree(root);
    if(result.type == EVALUATION_RESULT_number)
    {
        printf("Program was evaluated to numeric value %f.\n", result.number);
    }
    else if(result.type == EVALUATION_RESULT_boolean)
    {
        printf("Program was evaluated to boolean value %s.\n", result.boolean ? "true" : "false");
    }
}

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

int
main(int argument_count, char **arguments)
{
    if(argument_count > 1)
    {
        char *lettuce_file = LoadEntireFileAndNullTerminate(arguments[1]);
        InterpretCode(lettuce_file);
    }
    else
    {
        fprintf(stderr, "Usage: %s <lettuce file>\n", arguments[0]);
    }
    return 0;
}