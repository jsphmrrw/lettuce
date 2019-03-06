#include <stdlib.h>
#include <stdio.h>

#include "lettuce_utilities.c"
#include "lettuce_tokenizer.c"
#include "lettuce_abstract_syntax_tree.c"
#include "lettuce_parse.c"

static void
InterpretCode(char *code)
{
    Tokenizer tokenizer_ = {0};
    ParseContext context_ = {0};
    InterpreterEnvironment environment_ = {0};
    
    Tokenizer *tokenizer = &tokenizer_;
    ParseContext *context = &context_;
    InterpreterEnvironment *environment = &environment_;
    
    tokenizer->at = code;
    
    AbstractSyntaxTreeNode *root = ParseExpression(tokenizer, context);
    PrintAbstractSyntaxTree(root);
    printf("\n");
    
    EvaluationResult result = EvaluateAbstractSyntaxTree(environment, root);
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
        if(lettuce_file)
        {
            InterpretCode(lettuce_file);
        }
        else
        {
            fprintf(stderr, "FATAL ERROR: \"%s\" could not be loaded.\n", arguments[1]);
        }
    }
    else
    {
        fprintf(stderr, "Usage: %s <lettuce file>\n", arguments[0]);
    }
    return 0;
}