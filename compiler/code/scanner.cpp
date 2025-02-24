#include <string.h>
#include "scanner.h"

Scanner scanner;

void initScanner(const char* source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAtEnd()
{
    return *scanner.current == '\0';
}

//static se refiere a que solo se usa dentro de scanner.cpp

static Token makeToken(TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    
    return token;
};

static Token errorToken(const char* message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)(strlen(message));
    token.line = scanner.line;
    
    return token;
}

static char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

static bool match(char expected)
{
    if(isAtEnd()) return false;
    if(*scanner.current != expected) return false;
    ++scanner.current;
    return true;
}

static char peek()
{
    return *scanner.current;
}

static char peekNext()
{
    if(isAtEnd()) return '\0';
    return scanner.current[1];
}

static void skipWhitespace()
{
    for(;;)
    {
        char c = peek();
        switch(c)
        {
            case ' ':
            case '\r':
            case '\t':
            advance();
            break;
            case '\n':
            scanner.line++;
            advance();
            break;
            case '/':
            if(peekNext() == '/')
            {
                while(peek() != '\n' && isAtEnd()) advance();
            }
            break;
            default:
            return;
        }
    }
}

static Token string()
{
    while(peek() != '"' && !isAtEnd())
    {
        if(peek() == '\n') scanner.line++;
        advance();
    }
    
    if(isAtEnd()) return errorToken("Unterminated string.");
    
    
    advance();
    
    return makeToken(TOKEN_STRING);
}

static bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

static Token number()
{
    while(isDigit(peek())) advance();
    
    if(peek() == '.' && isDigit(peekNext()))
    {
        advance();
        
        while(isDigit(peek())) advance();
    }
    
    return makeToken(TOKEN_NUMBER);
}

Token scanToken()
{
    skipWhitespace();
    
    //aqui todo se tokeniza
    scanner.start = scanner.current;
    
    //aqui va el movimiento de current
    
    if(isAtEnd()) return makeToken(TOKEN_EOF);
    
    char c = advance();
    
    //if(isAlpha(c)) return identifier();
    if(isDigit(c)) return number();
    
    //se extrae despues de que se mueve 
    //primero se hacen unchingo de iffs para checar si si es el token
    //mueves el puntero
    switch(c)
    {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);
        
        case '!': return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=': return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '>': return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        
        case '"': return string();
        
    }
    
    return errorToken("Unexpectected Character");
}












