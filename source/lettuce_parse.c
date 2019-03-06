
static AbstractSyntaxTreeNode *
ParseExpression(Tokenizer *tokenizer, MemoryArena *arena)
{
    AbstractSyntaxTreeNode *result = 0;
    
    Token token = PeekToken(tokenizer);
    
    if(TokenMatchCString(token, "(") ||
       TokenMatchCString(token, "["))
    {
        NextToken(tokenizer, 0);
        result = ParseExpression(tokenizer, arena);
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
        // NOTE(rjf): Function definition.
        NextToken(tokenizer, 0);
        Token identifier = {0};
        
        if(RequireTokenMatch(tokenizer, "(", 0) &&
           RequireTokenType(tokenizer, TOKEN_alphanumeric_block, &identifier) &&
           RequireTokenMatch(tokenizer, ")", 0))
        {
            AbstractSyntaxTreeNode *def = MemoryArenaAllocateNode(arena);
            def->type = ABSTRACT_SYNTAX_TREE_NODE_function_definition;
            def->function_definition.param_name = identifier.string;
            def->function_definition.param_name_length = identifier.string_length;
            def->function_definition.body = ParseExpression(tokenizer, arena);
            result = def;
        }
        else
        {
            // NOTE(rjf): ERROR, expected (param_name)
        }
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
        
        AbstractSyntaxTreeNode *let = MemoryArenaAllocateNode(arena);
        let->type = ABSTRACT_SYNTAX_TREE_NODE_let;
        let->let.string = identifier.string;
        let->let.string_length = identifier.string_length;
        let->let.binding_expression = ParseExpression(tokenizer, arena);
        
        Token in;
        
        if(RequireTokenMatch(tokenizer, "in", &in))
        {
            let->let.body_expression = ParseExpression(tokenizer, arena);
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
            AbstractSyntaxTreeNode *val = MemoryArenaAllocateNode(arena);
            val->type = ABSTRACT_SYNTAX_TREE_NODE_boolean_constant;
            val->boolean_constant.value = 1;
            result = val;
        }
        else if(TokenMatchCString(token, "false"))
        {
            // NOTE(rjf): Boolean constant of false.
            NextToken(tokenizer, 0);
            AbstractSyntaxTreeNode *val = MemoryArenaAllocateNode(arena);
            val->type = ABSTRACT_SYNTAX_TREE_NODE_boolean_constant;
            val->boolean_constant.value = 0;
            result = val;
        }
        else
        {
            // NOTE(rjf): In this case, we must have an identifier being used.
            NextToken(tokenizer, 0);
            
            // NOTE(rjf): Is it a function call? If so, we should expect a (param).
            if(TokenMatchCString(PeekToken(tokenizer), "("))
            {
                AbstractSyntaxTreeNode *call = MemoryArenaAllocateNode(arena);
                call->type = ABSTRACT_SYNTAX_TREE_NODE_function_call;
                call->function_call.name = token.string;
                call->function_call.name_length = token.string_length;
                call->function_call.parameter = ParseExpression(tokenizer, arena);
                result = call;
            }
            else
            {
                // NOTE(rjf): This is just an identifier being used.
                AbstractSyntaxTreeNode *val = MemoryArenaAllocateNode(arena);
                val->type = ABSTRACT_SYNTAX_TREE_NODE_identifier;
                val->identifier.string = token.string;
                val->identifier.string_length = token.string_length;
                result = val;
            }
        }
    }
    else if(token.type == TOKEN_numeric_constant)
    {
        // NOTE(rjf): This can either be a naked numeric constant (some leaf
        //            of the AST) or it can be part of a larger expression.
        
        NextToken(tokenizer, 0);
        
        AbstractSyntaxTreeNode *val = MemoryArenaAllocateNode(arena);
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
            
            AbstractSyntaxTreeNode *binary_operator = MemoryArenaAllocateNode(arena);
            binary_operator->type = ABSTRACT_SYNTAX_TREE_NODE_binary_operator;
            binary_operator->binary_operator.type = operator_type;
            binary_operator->binary_operator.left = result;
            
            int right_hand_side_is_guarded_by_parentheses =
                TokenMatchCString(PeekToken(tokenizer), "(") ||
                TokenMatchCString(PeekToken(tokenizer), "[");
            
            binary_operator->binary_operator.right = ParseExpression(tokenizer, arena);
            
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
