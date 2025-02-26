
#include "stdio.h"
#include "stdlib.h"
#include "compiler.h"

//Where can they be used?
ParseRule rules[TOKEN_EOF + 1] = { 
    {grouping, NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE}, 
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {unary,    binary, PREC_TERM},
    {NULL,     binary, PREC_TERM},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     binary, PREC_FACTOR},
    {NULL,     binary, PREC_FACTOR},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {number,   NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
    {NULL,     NULL,   PREC_NONE},
};

Parser parser;
Chunk * compilingChunk;

Chunk * currentChunk()
{
    return compilingChunk;
}

static void errorAt(Token* token, const char * message)
{
    if(parser.panicMode) return;
    parser.panicMode = true;
    
    fprintf(stderr, "[line %d] Error", token->line);
    
    if(token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if(token->type == TOKEN_ERROR)
    {
        
    }
    else
    {
        fprintf( stderr, " at '%.*s'", token->length, token->start);
    }
    
    fprintf(stderr, ": %s\n", message);
    
    parser.hadError = true;
}


static void errorAtCurrent(const char* message)
{
    errorAt(&parser.current, message);
}

static void error(const char* message)
{
    errorAt(&parser.previous, message);
}

static void advance()
{
    parser.previous = parser.current;
    
    for(;;)
    {
        parser.current = scanToken();
        if(parser.current.type != TOKEN_ERROR)
            break;
        
        errorAtCurrent(parser.current.start);
    }
}

static void consume(TokenType type, const char* message)
{
    if(parser.current.type == type)
    {
        advance();
        return;
    }
    
    errorAtCurrent(message);
}

static void emitByte(uint8_t byte)
{
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2)
{
    emitByte(byte1);
    emitByte(byte2);
}

static void emitReturn()
{
    emitByte(OP_RETURN);
}

static void endCompiler()
{
    emitReturn();
}

uint8_t makeConstant(Value value)
{
    int index = addconstant(currentChunk(), value);
    //SAFE CAST
    if(index > UINT8_MAX)
    {
        //I cannot index outide 256 in the instruction!
        error("Too many constants in one chunk.");
        return 0;
    }
    
    return (uint8_t)index;
}

void emitConstant(Value value)
{
    emitBytes(OP_CONSTANT, makeConstant(value));
}

void number()
{
    double value = strtod(parser.previous.start, NULL);
    emitConstant(value);
}


static ParseRule * getRule(TokenType type)
{
    return &rules[type];
}

static void parsePrecedence(Precedence precedence)
{
    //prefix parsing
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if(prefixRule == NULL)
    {
        error("Expect expression.");
    }
    prefixRule();
    
    //infix parsing
    /*
agarras la misma
*/
    while(getRule(parser.current.type)->precedence >= precedence)
    {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
    
}

static void expression()
{
    //Literal le estoy diciende que parsee todas!
    parsePrecedence(PREC_ASSIGNMENT);
}

void binary()
{
    TokenType type = parser.previous.type;
    
    ParseRule * rule = getRule(type);
    
    //It compiles the right operand!
    //we need to stop if operand is the same as this one
    parsePrecedence((Precedence)(rule->precedence + 1));
    
    switch(type)
    {
        case TOKEN_PLUS: emitByte(OP_ADD); break;
        
        case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
        
        case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
        
        case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
        default: return;
    }
    
}

void unary()
{
    TokenType type = parser.previous.type;
    
    //este es del que sigue
    parsePrecedence(PREC_UNARY);
    
    //Por eso se pone despues por que el stack tiene aqui el valor que se va a negar
    switch(type)
    {
        case TOKEN_MINUS:
        emitByte(OP_NEGATE);
        break;
        default: return;
    }
    
}

void grouping()
{
    //es para el inner code
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

bool compile(const char* source, Chunk* chunk)
{
    initScanner(source);
    
    compilingChunk = chunk;
    
    parser.hadError = false;
    parser.panicMode = false;
    
    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    endCompiler();
    return !parser.hadError;
}
