
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include "object.h"
#include "compiler.h"

//Where can they be used?

ParseRule rules[] = {
    /*[TOKEN_LEFT_PAREN]    = */{grouping, call,   PREC_CALL},
    /*[TOKEN_RIGHT_PAREN]   = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_LEFT_BRACE]    = */{NULL,     NULL,   PREC_NONE}, 
    /*[TOKEN_RIGHT_BRACE]   = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_COMMA]         = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_DOT]           = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_MINUS]         = */{unary,    binary, PREC_TERM},
    /*[TOKEN_PLUS]          = */{NULL,     binary, PREC_TERM},
    /*[TOKEN_SEMICOLON]     = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_SLASH]         = */{NULL,     binary, PREC_FACTOR},
    /*[TOKEN_STAR]          = */{NULL,     binary, PREC_FACTOR},
    /*[TOKEN_BANG]          = */{unary,     NULL,   PREC_NONE},
    /*[TOKEN_BANG_EQUAL]    = */{NULL,     binary,   PREC_EQUALITY},
    /*[TOKEN_EQUAL]         = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_EQUAL_EQUAL]   = */{NULL,     binary,   PREC_EQUALITY},
    /*[TOKEN_GREATER]       = */{NULL,     binary,   PREC_COMPARISON},
    /*[TOKEN_GREATER_EQUAL] = */{NULL,     binary,   PREC_COMPARISON},
    /*[TOKEN_LESS]          = */{NULL,     binary,   PREC_COMPARISON},
    /*[TOKEN_LESS_EQUAL]    = */{NULL,     binary,   PREC_COMPARISON},
    /*[TOKEN_IDENTIFIER]    = */{variable,     NULL,   PREC_NONE},
    /*[TOKEN_STRING]        = */{string,     NULL,   PREC_NONE},
    /*[TOKEN_NUMBER]        = */{number,   NULL,   PREC_NONE},
    /*[TOKEN_AND]           = */{NULL,     and_,   PREC_AND},
    /*[TOKEN_CLASS]         = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_ELSE]          = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_FALSE]         = */{literal,     NULL,   PREC_NONE},
    /*[TOKEN_FOR]           = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_FUN]           = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_IF]            = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_NIL]           = */{literal,     NULL,   PREC_NONE},
    /*[TOKEN_OR]            = */{NULL,     or_,   PREC_OR},
    /*[TOKEN_PRINT]         = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_RETURN]        = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_SUPER]         = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_THIS]          = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_TRUE]          = */{literal,     NULL,   PREC_NONE},
    /*[TOKEN_VAR]           = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_WHILE]         = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_ERROR]         = */{NULL,     NULL,   PREC_NONE},
    /*[TOKEN_EOF]           = */{NULL,     NULL,   PREC_NONE},
};

Parser parser;

Compiler* current = NULL;

static Chunk* currentChunk()
{
    return &current->function->chunk;
}

static void initCompiler(Compiler * compiler, FunctionType type)
{
    //esta cosa apunta arriba a su papa
    compiler->enclosing = current;
    compiler->function = newFunction();
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    
    current = compiler;
    
    current->function->name = copyString(parser.previous.start, parser.previous.length);
    
    //aqui estan todas
    Local* local = &current->locals[current->localCount++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
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

void literal(bool canAssign)
{
    switch(parser.previous.type)
    {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
    }
    
}

static void emitBytes(uint8_t byte1, uint8_t byte2)
{
    emitByte(byte1);
    emitByte(byte2);
}

static void emitReturn()
{
    emitByte(OP_NIL);
    emitByte(OP_RETURN);
}

static ObjFunction* endCompiler()
{
    emitReturn();
    
    ObjFunction* function = current->function;
    
#ifdef DEBUG_PRINT_CODE
    
    if(!parser.hadError)
    {
        disassembleChunk(currentChunk(), function->name != NULL ? function->name->chars : "script");
    }
    
#endif
    
    current = current->enclosing;
    return function;
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

void string(bool canAssign)
{
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

void number(bool canAssign)
{
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}


static ParseRule * getRule(TokenType type)
{
    return &rules[type];
}


static bool check(TokenType type)
{
    return parser.current.type == type;
}

static bool match(TokenType type)
{
    if(!check(type)) return false;
    advance();
    return true;
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
    
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    
    prefixRule(canAssign);
    
    //infix parsing
    /*
agarras la misma
se supone que aqui ya deberia de haber consumido el assignment!
*/
    while(getRule(parser.current.type)->precedence >= precedence)
    {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }
    
    if(canAssign && match(TOKEN_EQUAL))
    {
        error("Invalid assignment target.");
    }
    
}



static void expression()
{
    //Literal le estoy diciende que parsee todas!
    parsePrecedence(PREC_ASSIGNMENT);
}

static void printStatement()
{
    //The statements keep the stack equal
    //and expressions keep it 1.
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void expressionStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_POP);
}


static uint8_t identifierConstant(Token* name)
{
    //tengo un value
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

//shadowing?
void addLocal(Token name)
{
    //aqui ya se salió
    if(current->localCount == UINT8_COUNT)
    {
        error("Too many local variables in function");
        return;
    }
    
    Local * local = &current->locals[current->localCount++];
    
    local->name = name;
    local->depth = -1;
    
}

bool identifiersEqual(Token * a, Token * b)
{
    if(a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static void declareVariable()
{
    //esto es para locales!
    if(current->scopeDepth == 0) return;
    
    Token* name = &parser.previous;
    
    for(int i = current->localCount - 1; i >= 0; i--)
    {
        Local* local = &current->locals[i]; 
        
        if(local->depth != -1 && local->depth < current->scopeDepth)
        {
            break;
        }
        
        //esto nomas checa el current scope!
        //esto si permite el shadowing!
        if(identifiersEqual(name, &local->name))
        {
            error("Already a variable with this name in this scope.");
        }
    }
    
    addLocal(*name);
}

static uint8_t parseVariable(const char* errorMessage)
{
    consume(TOKEN_IDENTIFIER, errorMessage);
    
    //no se mete si es local
    declareVariable();
    if(current->scopeDepth > 0) return 0;
    
    return identifierConstant(&parser.previous);
}

static void markInitialized()
{
    if(current->scopeDepth == 0) return;
    
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t index)
{
    //aqui puede llegar con 0
    if(current->scopeDepth > 0)
    {
        markInitialized();
        return;
    }
    
    emitBytes(OP_DEFINE_GLOBAL, index);
}

//tal vez porque no se puede hacer mucho en un var declaration
static void varDeclaration()
{
    uint8_t index = parseVariable("Expect variable name.");
    
    if(match(TOKEN_EQUAL))
    {
        expression();
    }
    else
    {
        emitByte(OP_NIL);
    }
    
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    
    defineVariable(index);
}

static uint8_t argumentList()
{
    uint8_t argCount = 0;
    
    if(!check(TOKEN_RIGHT_PAREN))
    {
        do
        {
            
            //esto es codigo de argumentos
            expression();
            
            if(argCount == 255)
            {
                error("Can't have more than 255 parameters.");
            }
            
            ++argCount;
        }while(match(TOKEN_COMMA));
    }
    
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    
    return argCount;
}

//este es para call
void call(bool canAssign)
{
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

static void block()
{
    while(!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        declaration();
    }
    
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block"); 
}

static void beginScope()
{
    ++current->scopeDepth;
}

static void endScope()
{
    --current->scopeDepth;
    
    while(current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth)
    {
        //estas eliminando variables
        emitByte(OP_POP);
        --current->localCount;
    }
}

static int emitJump(uint8_t instruction)
{
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    
    return currentChunk()->count - 2;
}

static void patchJump(int offset)
{
    int jump = (currentChunk()->count - offset) - 2;
    
    if(jump > UINT16_MAX) 
    {
        error("Too much code to jump over.");
    }
    
    currentChunk()->code[offset] = (jump >> 8) & 0xFF;
    currentChunk()->code[offset + 1] = jump & 0xFF;
    
}

/*so the point is to not pop the second value because this one must left something on the stack"*/
void and_(bool canAssign)
{
    int endJump = emitJump(OP_JUMP_IF_FALSE);
    
    emitByte(OP_POP);
    
    parsePrecedence(PREC_AND);
    
    patchJump(endJump);
}

//se tienen que evaluar los dos!
void or_(bool canAssign)
{
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);
    
    patchJump(elseJump);
    emitByte(OP_POP);
    
    parsePrecedence(PREC_OR);
    
    patchJump(endJump);
    
}


static void ifStatement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    
    emitByte(OP_POP);
    
    statement();
    
    //esta cosa se va a saltar esa madre!
    int elseJump = emitJump(OP_JUMP);
    
    //backpatching
    patchJump(thenJump);
    
    emitByte(OP_POP);
    
    if(match(TOKEN_ELSE)) statement();
    
    patchJump(elseJump);
    
}

static void emitLoop(int loopStart)
{
    //el ip va a estar despues de offset
    emitByte(OP_LOOP);
    
    int offset = (currentChunk()->count - loopStart) + 2;
    if(offset > UINT16_MAX) error("Loop body too large.");
    
    emitByte((offset >> 8) & 0xFF);
    emitByte(offset & 0xFF);
    
    
}

static void whileStatement()
{
    int loopStart = currentChunk()->count;
    
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    
    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    
    emitByte(OP_POP);
    
    statement();
    
    emitLoop(loopStart);
    
    patchJump(exitJump);
    emitByte(OP_POP);
}

void forStatement()
{
    // esta variable tiene que estar inicializada y creada en el scope
    beginScope();
    
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    
    if(match(TOKEN_SEMICOLON))
    {
        // no initializer!
    }
    else if(match(TOKEN_VAR))
    {
        // var declaration
        varDeclaration();
    }
    else
    {
        expressionStatement();
    }
    
    //este sirve para subir hasta aqui antes de la condicion!
    //hasta qui se regresa
    int loopStart = currentChunk()->count;
    
    //puede que no haya jump!
    int exitJump = -1;
    
    //este es el segundo
    if(!match(TOKEN_SEMICOLON))
    {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';'.");
        
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
    }
    
    if(!match(TOKEN_RIGHT_PAREN))
    {
        int bodyJump = emitJump(OP_JUMP);
        
        int incrementStart = currentChunk()->count;
        
        expression();
        
        emitByte(OP_POP);
        
        //aqui no consumes ;
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
        
        emitLoop(loopStart);
        
        loopStart = incrementStart;
        
        patchJump(bodyJump);
    }
    
    
    //aqui es donde tiene que estar el break!
    statement();
    
    emitLoop(loopStart);
    
    if(exitJump != -1)
    {
        patchJump(exitJump);
        emitByte(OP_POP);
    }
    
    endScope();
    
    //aqui es donde tienes que saltar con el break!
}

static void returnStatement()
{
    if(current->type == TYPE_SCRIPT)
    {
        error("Can't return from top-level code.");
    }
    
    if(match(TOKEN_SEMICOLON))
    {
        //regresa NIL
        emitReturn();
    }
    else
    {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
}

void statement()
{
    if(match(TOKEN_PRINT))
    {
        printStatement();
    }
    else if(match(TOKEN_IF))
    {
        ifStatement();
    }
    else if(match(TOKEN_RETURN))
    {
        returnStatement();
    }
    else if(match(TOKEN_WHILE))
    {
        whileStatement();
    }
    else if(match(TOKEN_FOR))
    {
        forStatement();
    }
    else if(match(TOKEN_LEFT_BRACE))
    {
        beginScope();
        block();
        endScope();
    }
    else
    {
        //aqui se llama al setter
        expressionStatement();
    }
}


static void synchronize()
{
    parser.panicMode = false;
    
    while(parser.current.type != TOKEN_EOF)
    {
        if(parser.previous.type == TOKEN_SEMICOLON) return;
        
        //empiezan un statement
        switch(parser.current.type)
        {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
            return;
            
        }
        
        advance();
    }
}

//decalaration
static void function(FunctionType type)
{
    Compiler compiler;
    initCompiler(&compiler, type);
    
    beginScope();
    
    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    
    if(!check(TOKEN_RIGHT_PAREN))
    {
        do
        {
            //it is because functions name is stored in the data segment as well
            ++current->function->arity;
            
            if(current->function->arity > 255)
            {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            
            //esto es codigo de parametros
            uint8_t index = parseVariable("Expect parameter name.");
            defineVariable(index);
        }while(match(TOKEN_COMMA));
    }
    
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    
    block();
    
    //esto emite return y ya!
    ObjFunction* function = endCompiler();
    
    emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));
}

static void funDeclaration()
{
    uint8_t index = parseVariable("Expect function name.");
    
    //con funciones si se puede!
    //aqui estas en global!
    markInitialized();
    function(TYPE_FUNCTION);
    //store that function back into the variable we declared
    
    defineVariable(index);
}

void declaration()
{
    if(match(TOKEN_FUN))
    {
        funDeclaration();
    }
    else if(match(TOKEN_VAR))
    {
        varDeclaration();
    }
    else
    {
        statement();
    }
    
    if(parser.panicMode) synchronize();
}

//shadowing
int resolveLocal(Compiler* compiler, Token* name)
{
    for(int i = compiler->localCount - 1; i >= 0; i--)
    {
        Local* local = &compiler->locals[i]; 
        
        if(identifiersEqual(name, &local->name))
        {
            if(local->depth == -1)
            {
                error("Can't read local variable in its own initializer.");
            }
            
            return i;
        }
    }
    
    return -1;
}

static void namedVariable(Token name, bool canAssign)
{
    uint8_t getOp, setOp;
    //como no se encuentra aqui
    int arg = resolveLocal(current, &name);
    
    if(arg != -1)
    {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else
    {
        //this is for globals only!
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }
    
    if(canAssign && match(TOKEN_EQUAL))
    {
        expression();
        //este es un safe cast!
        emitBytes(setOp, (uint8_t)arg);
    }
    else
    {
        emitBytes(getOp, (uint8_t)arg);
    }
}

void variable(bool canAssign)
{
    namedVariable(parser.previous, canAssign);
}

void binary(bool canAssign)
{
    TokenType type = parser.previous.type;
    
    ParseRule * rule = getRule(type);
    
    //It compiles the right operand!
    //we need to stop if operand is the same as this one
    parsePrecedence((Precedence)(rule->precedence + 1));
    
    switch(type)
    {
        case TOKEN_BANG_EQUAL: emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
        case TOKEN_GREATER: emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS: emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS: emitByte(OP_ADD); break;
        case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
        default: return;
    }
    
}

void unary(bool canAssign)
{
    TokenType type = parser.previous.type;
    
    //este es del que sigue
    parsePrecedence(PREC_UNARY);
    
    //Por eso se pone despues por que el stack tiene aqui el valor que se va a negar
    switch(type)
    {
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        case TOKEN_BANG: emitByte(OP_NOT); break;
        default: return;
    }
    
}

void grouping(bool canAssign)
{
    //es para el inner code
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

ObjFunction* compile(const char* source)
{
    initScanner(source);
    
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);
    
    //compilingChunk = chunk;
    
    parser.hadError = false;
    parser.panicMode = false;
    
    advance();
    
    while(!match(TOKEN_EOF))
    {
        declaration();
    }
    
    ObjFunction* function = endCompiler();
    return parser.hadError ? NULL : function;
}
