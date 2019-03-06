
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

static void
ParseContextCleanUp(ParseContext *context)
{
    free(context->first_chunk.memory);
    for(ParseContextMemoryChunk *chunk = context->first_chunk.next;
        chunk;)
    {
        ParseContextMemoryChunk *next_chunk = chunk->next;
        free(chunk);
        chunk = next_chunk;
    }
}

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
    
    if(TokenMatchCString(token, "(") ||
       TokenMatchCString(token, "["))
    {
        NextToken(tokenizer, 0);
        result = ParseExpression(tokenizer, context);
        if(TokenMatchCString(PeekToken(tokenizer), ")") ||
           TokenMatchCString(PeekToken(tokenizer), "]"))
        {
            NextToken(tokenizer, 0);
        }
        else
        {
            // NOTE(rjf): ERROR! Why is there not a following paren?
        }
    }
    else if(TokenMatchCString(token, "function"))
    {
        // TODO(rjf): Function definition.
        
        // AbstractSyntaxTreeNode *def = ParseContextAllocateNode(context);
        // def->type = ABSTRACT_SYNTAX_TREE_NODE_function_definition;
        // def->function_definition.param_name = identifier.string;
        // def->function_definition.param_name_length = identifier.string_length;
        // def->function_definition.body = ParseExpression(tokenizer, context);
        
        
    }
    else if(TokenMatchCString(token, "let"))
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
        if(TokenMatchCString(token, "true"))
        {
            // NOTE(rjf): Boolean constant of true.
            NextToken(tokenizer, 0);
            AbstractSyntaxTreeNode *val = ParseContextAllocateNode(context);
            val->type = ABSTRACT_SYNTAX_TREE_NODE_boolean_constant;
            val->boolean_constant.value = 1;
            result = val;
        }
        else if(TokenMatchCString(token, "false"))
        {
            // NOTE(rjf): Boolean constant of false.
            NextToken(tokenizer, 0);
            AbstractSyntaxTreeNode *val = ParseContextAllocateNode(context);
            val->type = ABSTRACT_SYNTAX_TREE_NODE_boolean_constant;
            val->boolean_constant.value = 0;
            result = val;
        }
        else
        {
            // NOTE(rjf): In this case, we must have an identifier being used.
            NextToken(tokenizer, 0);
            AbstractSyntaxTreeNode *val = ParseContextAllocateNode(context);
            val->type = ABSTRACT_SYNTAX_TREE_NODE_identifier;
            val->identifier.string = token.string;
            val->identifier.string_length = token.string_length;
            result = val;
        }
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
    
    if(!TokenMatchCString(token, ")") &&
       token.type == TOKEN_symbolic_block)
    {
        // NOTE(rjf): We could have a binary operator here.
        
        int operator_type = TokenToBinaryOperator(token);
        if(operator_type != BINARY_OPERATOR_invalid)
        {
            NextToken(tokenizer, 0);
            
            AbstractSyntaxTreeNode *binary_operator = ParseContextAllocateNode(context);
            binary_operator->type = ABSTRACT_SYNTAX_TREE_NODE_binary_operator;
            binary_operator->binary_operator.type = operator_type;
            binary_operator->binary_operator.left = result;
            
            int right_hand_side_is_guarded_by_parentheses =
                TokenMatchCString(PeekToken(tokenizer), "(") ||
                TokenMatchCString(PeekToken(tokenizer), "[");
            
            binary_operator->binary_operator.right = ParseExpression(tokenizer, context);
            
            AbstractSyntaxTreeNode *right = binary_operator->binary_operator.right;
            
            if(!right_hand_side_is_guarded_by_parentheses &&
               right->type == ABSTRACT_SYNTAX_TREE_NODE_binary_operator &&
               right->binary_operator.type < binary_operator->binary_operator.type)
            {
                binary_operator->binary_operator.type = right->binary_operator.type;
                right->binary_operator.type = operator_type;
                
                AbstractSyntaxTreeNode *swap = binary_operator->binary_operator.left;
                binary_operator->binary_operator.left = right->binary_operator.right;
                AbstractSyntaxTreeNode *swap2 = right->binary_operator.left;
                right->binary_operator.left = swap;
                right->binary_operator.right = swap2;
                
                swap = binary_operator->binary_operator.left;
                binary_operator->binary_operator.left = binary_operator->binary_operator.right;
                binary_operator->binary_operator.right = swap;
            }
            
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
