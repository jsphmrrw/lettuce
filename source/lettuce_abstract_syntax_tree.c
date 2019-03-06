
#define BINARY_OPERATOR_LIST \
BinaryOperator(or, "||") \
BinaryOperator(and, "&&") \
BinaryOperator(less_than, "<") \
BinaryOperator(greater_than, ">") \
BinaryOperator(less_than_equal_to, "<=") \
BinaryOperator(greater_than_equal_to, ">=") \
BinaryOperator(equal_to, "==") \
BinaryOperator(not_equal_to, "!=") \
BinaryOperator(plus, "+") \
BinaryOperator(minus, "-") \
BinaryOperator(multiply, "*") \
BinaryOperator(divide, "/") \

enum
{
    BINARY_OPERATOR_invalid,
#define BinaryOperator(name, str) BINARY_OPERATOR_##name,
    BINARY_OPERATOR_LIST
#undef BinaryOperator
};

static int
TokenToBinaryOperator(Token token)
{
    int type = BINARY_OPERATOR_invalid;
    
#define BinaryOperator(name, str) if(TokenMatchCString(token, str)) { type = BINARY_OPERATOR_##name; } else
    BINARY_OPERATOR_LIST
#undef BinaryOperator
    {}
    
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
    ABSTRACT_SYNTAX_TREE_NODE_if_then_else,
    ABSTRACT_SYNTAX_TREE_NODE_function_definition,
    ABSTRACT_SYNTAX_TREE_NODE_function_call,
};

enum
{
    EVALUATION_RESULT_number,
    EVALUATION_RESULT_boolean,
    EVALUATION_RESULT_closure,
};

typedef struct InterpreterEnvironment InterpreterEnvironment;
typedef struct AbstractSyntaxTreeNode AbstractSyntaxTreeNode;

typedef struct EvaluationResult
{
    int type;
    union
    {
        double number;
        int boolean;
        struct
        {
            char *param_name;
            int param_name_length;
            AbstractSyntaxTreeNode *body;
            InterpreterEnvironment *environment;
        }
        closure;
    };
}
EvaluationResult;

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
        
        struct IfThenElse
        {
            AbstractSyntaxTreeNode *condition;
            AbstractSyntaxTreeNode *pass_code;
            AbstractSyntaxTreeNode *fail_code;
        }
        if_then_else;
        
        struct FunctionDefinition
        {
            char *param_name;
            int param_name_length;
            AbstractSyntaxTreeNode *body;
        }
        function_definition;
        
        struct FunctionCall
        {
            char *name;
            int name_length;
            AbstractSyntaxTreeNode *parameter;
        }
        function_call;
        
    };
}
AbstractSyntaxTreeNode;

static AbstractSyntaxTreeNode *
MemoryArenaAllocateNode(MemoryArena *arena)
{
    return MemoryArenaAllocate(arena, sizeof(AbstractSyntaxTreeNode));
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
        case ABSTRACT_SYNTAX_TREE_NODE_boolean_constant:
        {
            printf("%s", root->boolean_constant.value ? "true" : "false");
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_binary_operator:
        {
            printf("(");
            PrintAbstractSyntaxTree(root->binary_operator.left);
            
#define BinaryOperator(name, str) if(root->binary_operator.type == BINARY_OPERATOR_##name) { printf(" " str " "); }
            BINARY_OPERATOR_LIST
#undef BinaryOperator
            
                PrintAbstractSyntaxTree(root->binary_operator.right);
            printf(")");
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_if_then_else:
        {
            printf("if(");
            PrintAbstractSyntaxTree(root->if_then_else.condition);
            printf(") then ");
            PrintAbstractSyntaxTree(root->if_then_else.pass_code);
            if(root->if_then_else.fail_code)
            {
                printf(" else ");
                PrintAbstractSyntaxTree(root->if_then_else.fail_code);
            }
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_function_definition:
        {
            printf("function(%.*s) ", root->function_definition.param_name_length,
                   root->function_definition.param_name);
            PrintAbstractSyntaxTree(root->function_definition.body);
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_function_call:
        {
            printf("%.*s(", root->function_call.name_length,
                   root->function_call.name);
            PrintAbstractSyntaxTree(root->function_call.parameter);
            printf(")");
            break;
        }
        default: break;
    }
}

#define INTERPRETER_ENVIRONMENT_DEFAULT_IDENTIFIER_TABLE_SIZE 512

typedef struct InterpreterEnvironment
{
    MemoryArena *arena;
    
    unsigned int identifier_table_count;
    unsigned int identifier_table_cap;
    
    struct
    {
        EvaluationResult value;
    }
    *identifier_table_values;
    
    struct
    {
        char *string;
        int string_length;
        int deleted;
    }
    *identifier_table_keys;
}
InterpreterEnvironment;

static unsigned int
HashString(char *str, int str_len)
{
    unsigned int hash = 5381;
    int c;
    int i = 0;
    while(c = *str++ && i < str_len)
    {
        hash = ((hash << 5) + hash) + c; // == hash * 33 + c
        ++i;
    }
    
    return hash;
}

static InterpreterEnvironment *
InterpreterEnvironmentDuplicate(InterpreterEnvironment *environment)
{
    InterpreterEnvironment *new_environment = MemoryArenaAllocate(environment->arena,
                                                                  sizeof(InterpreterEnvironment));
    new_environment->arena = environment->arena;
    new_environment->identifier_table_count = environment->identifier_table_count;
    new_environment->identifier_table_cap = environment->identifier_table_cap;
    new_environment->identifier_table_values = MemoryArenaAllocate(environment->arena,
                                                                   new_environment->identifier_table_cap * sizeof(new_environment->identifier_table_values[0]));
    new_environment->identifier_table_keys = MemoryArenaAllocate(environment->arena,
                                                                 new_environment->identifier_table_cap * sizeof(new_environment->identifier_table_keys[0]));
    
    MemoryCopy(new_environment->identifier_table_values, environment->identifier_table_values,
               sizeof(new_environment->identifier_table_values[0]) * new_environment->identifier_table_cap);
    MemoryCopy(new_environment->identifier_table_keys, environment->identifier_table_keys,
               sizeof(new_environment->identifier_table_keys[0]) * new_environment->identifier_table_cap);
    
    return new_environment;
}

static int
InterpreterEnvironmentBind(InterpreterEnvironment *environment, char *string, int string_length,
                           EvaluationResult evaluation)
{
    int added = 0;
    
    if(!environment->identifier_table_cap)
    {
        environment->identifier_table_count = 0;
        environment->identifier_table_cap = INTERPRETER_ENVIRONMENT_DEFAULT_IDENTIFIER_TABLE_SIZE;
        environment->identifier_table_values = MemoryArenaAllocate(environment->arena,
                                                                   sizeof(environment->identifier_table_values[0]) *
                                                                   environment->identifier_table_cap);
        environment->identifier_table_keys = MemoryArenaAllocate(environment->arena, sizeof(environment->identifier_table_keys[0]) *
                                                                 environment->identifier_table_cap);
    }
    
    if(environment->identifier_table_count >= environment->identifier_table_cap)
    {
        // TODO(rjf): We need to expand the table.
    }
    
    unsigned int hash_slot = HashString(string, string_length) % environment->identifier_table_cap;
    unsigned int original_hash_slot = hash_slot;
    
    for(;;)
    {
        if(!environment->identifier_table_keys[hash_slot].deleted &&
           environment->identifier_table_keys[hash_slot].string)
        {
            if(environment->identifier_table_keys[hash_slot].string &&
               StringMatch(string, string_length,
                           environment->identifier_table_keys[hash_slot].string,
                           environment->identifier_table_keys[hash_slot].string_length))
            {
                environment->identifier_table_values[hash_slot].value = evaluation;
                added = 1;
                break;
            }
            else
            {
                ++hash_slot;
                if(hash_slot >= environment->identifier_table_cap)
                {
                    hash_slot = 0;
                }
                else if(hash_slot == original_hash_slot)
                {
                    break;
                }
            }
        }
        else
        {
            added = 1;
            environment->identifier_table_values[hash_slot].value = evaluation;
            environment->identifier_table_keys[hash_slot].string = string;
            environment->identifier_table_keys[hash_slot].string_length = string_length;
            environment->identifier_table_keys[hash_slot].deleted = 0;
        }
    }
    
    return added;
}

static int
InterpreterEnvironmentLookUp(InterpreterEnvironment *environment, char *string, int string_length,
                             EvaluationResult *out_result)
{
    int found = 0;
    
    if(environment->identifier_table_cap)
    {
        unsigned int hash_slot = HashString(string, string_length) % environment->identifier_table_cap;
        unsigned int original_hash_slot = hash_slot;
        
        for(;;)
        {
            if(environment->identifier_table_keys[hash_slot].string ||
               environment->identifier_table_keys[hash_slot].deleted)
            {
                if(!environment->identifier_table_keys[hash_slot].deleted &&
                   environment->identifier_table_keys[hash_slot].string &&
                   StringMatch(string, string_length,
                               environment->identifier_table_keys[hash_slot].string,
                               environment->identifier_table_keys[hash_slot].string_length))
                {
                    found = 1;
                    *out_result = environment->identifier_table_values[hash_slot].value;
                    break;
                }
                else
                {
                    ++hash_slot;
                    if(hash_slot >= environment->identifier_table_cap)
                    {
                        hash_slot = 0;
                    }
                    else if(hash_slot == original_hash_slot)
                    {
                        break;
                    }
                }
            }
            else
            {
                break;
            }
        }
    }
    
    return found;
}

static int
InterpreterEnvironmentDelete(InterpreterEnvironment *environment, char *string, int string_length)
{
    int found = 0;
    
    if(environment->identifier_table_cap)
    {
        unsigned int hash_slot = HashString(string, string_length) % environment->identifier_table_cap;
        unsigned int original_hash_slot = hash_slot;
        
        for(;;)
        {
            if(environment->identifier_table_keys[hash_slot].string ||
               environment->identifier_table_keys[hash_slot].deleted)
            {
                if(environment->identifier_table_keys[hash_slot].string &&
                   StringMatch(string, string_length,
                               environment->identifier_table_keys[hash_slot].string,
                               environment->identifier_table_keys[hash_slot].string_length))
                {
                    found = 1;
                    environment->identifier_table_keys[hash_slot].deleted = 1;
                    environment->identifier_table_keys[hash_slot].string = 0;
                    environment->identifier_table_keys[hash_slot].string_length = 0;
                    break;
                }
                else
                {
                    ++hash_slot;
                    if(hash_slot >= environment->identifier_table_cap)
                    {
                        hash_slot = 0;
                    }
                    else if(hash_slot == original_hash_slot)
                    {
                        break;
                    }
                }
            }
            else
            {
                break;
            }
        }
    }
    
    return found;
}

static EvaluationResult
EvaluateAbstractSyntaxTree(InterpreterEnvironment *environment,
                           AbstractSyntaxTreeNode *root)
{
    EvaluationResult result = {0};
    
    switch(root->type)
    {
        case ABSTRACT_SYNTAX_TREE_NODE_let:
        {
            EvaluationResult binding = EvaluateAbstractSyntaxTree(environment, root->let.binding_expression);
            InterpreterEnvironmentBind(environment, root->let.string, root->let.string_length,
                                       binding);
            EvaluationResult body = EvaluateAbstractSyntaxTree(environment, root->let.body_expression);
            InterpreterEnvironmentDelete(environment, root->let.string, root->let.string_length);
            
            result = body;
            
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_identifier:
        {
            if(!InterpreterEnvironmentLookUp(environment, root->identifier.string, root->identifier.string_length,
                                             &result))
            {
                // NOTE(rjf): ERROR! Identifier not found.
            }
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_if_then_else:
        {
            EvaluationResult condition_evaluation =
                EvaluateAbstractSyntaxTree(environment, root->if_then_else.condition);
            
            if(condition_evaluation.boolean)
            {
                result = EvaluateAbstractSyntaxTree(environment, root->if_then_else.pass_code);
            }
            else
            {
                if(root->if_then_else.fail_code)
                {
                    result = EvaluateAbstractSyntaxTree(environment, root->if_then_else.fail_code);
                }
            }
            
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_function_definition:
        {
            EvaluationResult closure = {
                EVALUATION_RESULT_closure,
            };
            closure.closure.body = root->function_definition.body;
            closure.closure.environment = InterpreterEnvironmentDuplicate(environment);
            closure.closure.param_name = root->function_definition.param_name;
            closure.closure.param_name_length = root->function_definition.param_name_length;
            result = closure;
            
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_function_call:
        {
            EvaluationResult closure = {0};
            if(InterpreterEnvironmentLookUp(environment, root->function_call.name,
                                            root->function_call.name_length,
                                            &closure))
            {
                InterpreterEnvironment *call_environment = closure.closure.environment;
                EvaluationResult arg = EvaluateAbstractSyntaxTree(call_environment, root->function_call.parameter);
                InterpreterEnvironmentBind(call_environment, closure.closure.param_name,
                                           closure.closure.param_name_length,
                                           arg);
                result = EvaluateAbstractSyntaxTree(call_environment, closure.closure.body);
                InterpreterEnvironmentDelete(call_environment, closure.closure.param_name,
                                             closure.closure.param_name_length);
            }
            else
            {
                // NOTE(rjf): ERROR! Function not found.
            }
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_numeric_constant:
        {
            result.type = EVALUATION_RESULT_number;
            result.number = root->numeric_constant.value;
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_boolean_constant:
        {
            result.type = EVALUATION_RESULT_boolean;
            result.boolean = root->boolean_constant.value;
            break;
        }
        case ABSTRACT_SYNTAX_TREE_NODE_binary_operator:
        {
            EvaluationResult left_eval = EvaluateAbstractSyntaxTree(environment, root->binary_operator.left);
            EvaluationResult right_eval = EvaluateAbstractSyntaxTree(environment, root->binary_operator.right);
            
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
            else if(root->binary_operator.type == BINARY_OPERATOR_and)
            {
                result.type = EVALUATION_RESULT_boolean;
                result.boolean = left_eval.boolean && right_eval.boolean;
            }
            else if(root->binary_operator.type == BINARY_OPERATOR_or)
            {
                result.type = EVALUATION_RESULT_boolean;
                result.boolean = left_eval.boolean || right_eval.boolean;
            }
            else if(root->binary_operator.type == BINARY_OPERATOR_less_than)
            {
                result.type = EVALUATION_RESULT_boolean;
                result.boolean = left_eval.number < right_eval.number;
            }
            else if(root->binary_operator.type == BINARY_OPERATOR_less_than_equal_to)
            {
                result.type = EVALUATION_RESULT_boolean;
                result.boolean = left_eval.number <= right_eval.number;
            }
            else if(root->binary_operator.type == BINARY_OPERATOR_greater_than)
            {
                result.type = EVALUATION_RESULT_boolean;
                result.boolean = left_eval.number > right_eval.number;
            }
            else if(root->binary_operator.type == BINARY_OPERATOR_greater_than_equal_to)
            {
                result.type = EVALUATION_RESULT_boolean;
                result.boolean = left_eval.number >= right_eval.number;
            }
            else if(root->binary_operator.type == BINARY_OPERATOR_equal_to)
            {
                result.type = EVALUATION_RESULT_boolean;
                result.boolean = left_eval.number == right_eval.number;
            }
            else if(root->binary_operator.type == BINARY_OPERATOR_not_equal_to)
            {
                result.type = EVALUATION_RESULT_boolean;
                result.boolean = left_eval.number != right_eval.number;
            }
            
            break;
        }
        default: break;
    }
    
    return result;
}
