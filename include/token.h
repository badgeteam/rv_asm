#pragma once

#include"common_types.h"


// Reads files recursively and creates a Tokenchain
Token*tokenizeFile(char*filename);


bool tokenComp(Token*token1,Token*token2);
// Checks if the token is of type Ident and compares the strings
bool tokenIdentComp(char*str, Token*token);

// Checks if the token is of type Ident and compares the strings while being case insensitive
bool tokenIdentCompCI(char*str, Token*token);

/* Case Insensitive
 * Begins token string at offset
 * token string does not have to be terminated
 */
bool tokenIdentCompPartialCI(char*str, Token*token, uint32_t offset);
bool tokenIdentCompPartial(char*str,Token*token,uint32_t offset);

// Parses an unsigned Integer or throws an error
uint32_t parseUInt(Token*token);

// Parses a signed Integer or throws an error
int32_t parseInt(Token*token);

// Parses an unsigned Immediate Number of a specific length or throws an error
uint32_t parseUImm(Token*token, uint32_t length);

// Parses a signed Immediate Number of a specific length or throws an error
uint32_t parseImm(Token*teken, uint32_t length);


uint32_t parseNumber(Token*token);


void nextTokenEnforceExistence(CompContext*ctx);
void nextTokenEnforceComma(CompContext*ctx);
bool nextTokenCheckConcat(CompContext*ctx);
void nextTokenEnforceNewlineEOF(CompContext*ctx);
void enforceNewlineEOF(CompContext*ctx);

char*copyTokenContent(Token*token);
// Returns the name of the token. For Debug purposes
char*tokenTypeName(Token*token);

void printToken(Token*token);
void printTokenList(Token*token);


void compWarning(char*msg, Token*token);
void compError(char*msg, Token*token);

