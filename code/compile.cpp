// **********************************************************************
// *                                                                    *
// *                     The Verge-C Compiler v.0.10                    *
// *                     Copyright (C)1997 BJ Eirich                    *
// *                                                                    *
// * Module: COMPILE.C                                                  *
// *                                                                    *
// * Description: The main lexical parser and code generator.           *
// *                                                                    *
// * Portability: ANSI C - should compile on any 32 bit compiler.       *
// *                                                                    *
// **********************************************************************

#include "compile.hpp"
#include "funclib.hpp"
#include "lib_funcs.hpp"
#include "log.hpp"
#include "op_codes.hpp"
#include "util.hpp"
#include "vcc.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================== Variables ==============================

struct label GlobalLabels[200]; // goto labels
struct label GlobalGotos[200];  // goto occurence records
char GlobalLastToken[2048];     // restorebuf for NextIs
u64 GlobalTokenNumericValue;    // int value of token if it's type DIGIT
u64 GlobalTokenType;            // type of current token.
u64 GlobalTokenSubType;         // This is just crap.
u8 *GlobalNumArgsPointer;       // number of arguements to IF ptr

u64 GlobalLines = 1;

// Compilation-state flags
b64 GlobalInEvent = 0;
b64 GlobalInExternal = 0;
char *GlobalScriptToken;
u64 GlobalFunctionIndex;
u64 GlobalNumLabels = 0;
u64 GlobalNumGotos = 0;

// ================================ Code =================================

void
err(char *String)
{
  FILE *File;

  if (!CompileGuy.IsQuiet)
    printf("%s (%lld) \n", String, GlobalLines);
  if (CompileGuy.IsQuiet)
  {
    fopen_s(&File, "error.txt", "w");
    fprintf(File, "%s (%lld)\n", String, GlobalLines);
    fclose(File);
  }
  if (Exist("$$tmep$$.ma_"))
    remove("$$tmep$$.ma_");
  exit(-1);
}

b64
TokenIs(char *String)
{
  return StringsMatch(String, GlobalToken);
}

void
ParseWhitespace()
{
  // printf("ParseWhitespace\n");
  // ParseWhitespace() does what you'd expect - sifts through any white
  // space and advances the file pointer accordingly. Additionally, it
  // handles // style comments as well as /* and */ comments.

  while (1)
  {
    while (*CompileGuy.C <= ' ')
    {
      if (!*CompileGuy.C)
      {
        // printf("ParseWhitespace BUST OUT #1\n");
        return; // EOF reached
      }
      if (*CompileGuy.C == '\n')
        GlobalLines++;
      CompileGuy.C++;
    }

    if (CompileGuy.C[0] == '/' && CompileGuy.C[1] == '/') // Skip // comments
    {
      while (*CompileGuy.C && (*CompileGuy.C != '\n'))
      {
        CompileGuy.C++; // Skip to next line
      }
      continue;
    }

    if (CompileGuy.C[0] == '/' && CompileGuy.C[1] == '*') // Skip /* until */
    {
      while (!(CompileGuy.C[0] == '*' && CompileGuy.C[1] == '/'))
      {
        if (*CompileGuy.C == '\n')
        {
          GlobalLines++;
        }
        CompileGuy.C++;
        if (!*CompileGuy.C)
        {
          printf("ParseWhitespace BUST OUT #2\n");
          return;
        }
      }
      CompileGuy.C += 2;
      continue;
    }

    break; // end of whitespace
  }
  // printf("ParseWhitespace END\n");
}

void
CheckLibFunc()
{
  // printf("CheckLibFunc...\n");
  int Index;

  // If the current token is a recognized library function, sets
  // GlobalTokenType to FUNCTION and GlobalFunctionIndex to the appropriate
  // function code.

  // Todo: Maybe replace this with a binary tree instead of a linear
  // search. It hasn't gotten slow enough yet tho. :)

  GlobalTokenNumericValue = 0;

  if (StringLength(GlobalToken) == 1 && GlobalToken[0] > 64 &&
      GlobalToken[0] < 91)
  {
    GlobalTokenType = IDENTIFIER;
    GlobalTokenNumericValue = GlobalToken[0] - 64;
    return;
  }

  if (TokenIs("FLAGS") || TokenIs("IF") || TokenIs("FOR") || TokenIs("WHILE") ||
      TokenIs("SWITCH") || TokenIs("CASE") || TokenIs("GOTO"))
  {
    GlobalTokenType = RESERVED;
    return;
  }
  if (TokenIs("AND"))
  {
    GlobalTokenType = CONTROL;
    GlobalToken[0] = '&';
    GlobalToken[1] = '&';
    GlobalToken[2] = 0;
    return;
  }
  Index = 0;
  while (Index < NUM_FUNCS)
  {
    if (StringsMatch(GlobalStaticFunctionList[Index], GlobalToken))
      break;
    Index++;
  }
  if (Index != NUM_FUNCS)
  {
    GlobalTokenType = FUNCTION;
    // printf("CheckLibFunc: Found %lld (%s)\n", i,
    // GlobalStaticFunctionList[i]);
  }
  GlobalFunctionIndex = Index;
}

u64
SearchVarList()
{
  u64 Index;

  Index = 0;
  while (Index < NUM_VARS0)
  {
    if (StringsMatch(GlobalStaticVar0List[Index], GlobalToken))
      break;
    Index++;
  }
  if (Index != NUM_VARS0)
  {
    GlobalTokenType = RESERVED;
    GlobalTokenSubType = VAR0;
    return Index;
  }

  Index = 0;
  while (Index < NUM_VARS1)
  {
    if (StringsMatch(GlobalStaticVar1List[Index], GlobalToken))
      break;
    Index++;
  }
  if (Index != NUM_VARS1)
  {
    GlobalTokenType = RESERVED;
    GlobalTokenSubType = VAR1;
    return Index;
  }

  Index = 0;
  while (Index < NUM_VARS2)
  {
    if (StringsMatch(GlobalStaticVar2List[Index], GlobalToken))
      break;
    Index++;
  }
  if (Index != NUM_VARS2)
  {
    GlobalTokenType = RESERVED;
    GlobalTokenSubType = VAR2;
    return Index;
  }
  return Index;
}

void
GetIdentifier()
{
  // printf("GetIdentifier\n");
  int Index;

  // Retrieves an identifier from the source buffer. Before calling this,
  // it should be guaranteed that the first character is a letter. A check
  // will need to be made afterward to see if it's a reserved word.

  Index = 0;
  while ((CharTypeLookup[(int)*CompileGuy.C] == LETTER) ||
         (CharTypeLookup[(int)*CompileGuy.C] == DIGIT))
  {
    GlobalToken[Index] = *CompileGuy.C;
    CompileGuy.C++;
    Index++;
  }
  GlobalToken[Index] = 0;
  _strupr_s(GlobalToken, 2048);
  CheckLibFunc();
  SearchVarList();
}

void
GetNumber()
{
  // printf("GetNumber\n");
  int Index;

  // Grabs the next number. String version remains in token[], numerical
  // version is placed in GlobalTokenNumericValue.

  Index = 0;
  while (CharTypeLookup[(int)*CompileGuy.C] == DIGIT)
  {
    GlobalToken[Index] = *CompileGuy.C;
    CompileGuy.C++;
    Index++;
  }
  GlobalToken[Index] = 0;
  GlobalTokenNumericValue = atoi(GlobalToken);
}

void
GetPunctuation()
{
  // printf("GetPunctuation\n");
  // Grabs the next recognized punctuation type. If a double-char punctuation
  // type is recognized, it will be returned. Ie, differentiate b/w = and ==.

  switch (*CompileGuy.C)
  {
    case '(':
      GlobalToken[0] = '(';
      GlobalToken[1] = 0;
      CompileGuy.C++;
      break;
    case ')':
      GlobalToken[0] = ')';
      GlobalToken[1] = 0;
      CompileGuy.C++;
      break;
    case '{':
      GlobalToken[0] = '{';
      GlobalToken[1] = 0;
      CompileGuy.C++;
      break;
    case '}':
      GlobalToken[0] = '}';
      GlobalToken[1] = 0;
      CompileGuy.C++;
      break;
    case '[':
      GlobalToken[0] = '(';
      GlobalToken[1] = 0;
      CompileGuy.C++;
      break;
    case ']':
      GlobalToken[0] = ')';
      GlobalToken[1] = 0;
      CompileGuy.C++;
      break;
    case ',':
      GlobalToken[0] = ',';
      GlobalToken[1] = 0;
      CompileGuy.C++;
      break;
    case ':':
      GlobalToken[0] = ':';
      GlobalToken[1] = 0;
      CompileGuy.C++;
      break;
    case ';':
      GlobalToken[0] = ';';
      GlobalToken[1] = 0;
      CompileGuy.C++;
      break;
    case '/':
      GlobalToken[0] = '/';
      GlobalToken[1] = 0;
      CompileGuy.C++;
      break;
    case '*':
      GlobalToken[0] = '*';
      GlobalToken[1] = 0;
      CompileGuy.C++;
      break;
    case '%':
      GlobalToken[0] = '%';
      GlobalToken[1] = 0;
      CompileGuy.C++;
      break;
    case '\"':
      GlobalToken[0] = '\"';
      GlobalToken[1] = 0;
      CompileGuy.C++;
      break;
    case '\'':
      GlobalToken[0] = '\'';
      GlobalToken[1] = 0;
      CompileGuy.C++;
      break;
    case '+':
      GlobalToken[0] = '+'; // Is it ++ or += or +?
      CompileGuy.C++;
      if (*CompileGuy.C == '+')
      {
        GlobalToken[1] = '+';
        CompileGuy.C++;
      }
      else if (*CompileGuy.C == '=')
      {
        GlobalToken[1] = '=';
        CompileGuy.C++;
      }
      else
      {
        GlobalToken[1] = 0;
      }
      GlobalToken[2] = 0;
      break;
    case '-':
      GlobalToken[0] = '-'; // Is it -- or -= or -?
      CompileGuy.C++;
      if (*CompileGuy.C == '-')
      {
        GlobalToken[1] = '-';
        CompileGuy.C++;
      }
      else if (*CompileGuy.C == '=')
      {
        GlobalToken[1] = '=';
        CompileGuy.C++;
      }
      else
      {
        GlobalToken[1] = 0;
      }
      GlobalToken[2] = 0;
      break;
    case '>':
      GlobalToken[0] = '>'; // Is it > or >=?
      CompileGuy.C++;
      if (*CompileGuy.C == '=') // It's >=
      {
        GlobalToken[1] = '=';
        GlobalToken[2] = 0;
        CompileGuy.C++;
        break;
      }
      GlobalToken[1] = 0; // It's >
      break;
    case '<':
      GlobalToken[0] = '<'; // Is it < or <=?
      CompileGuy.C++;
      if (*CompileGuy.C == '=') // It's <=
      {
        GlobalToken[1] = '=';
        GlobalToken[2] = 0;
        CompileGuy.C++;
        break;
      }
      GlobalToken[1] = 0; // It's <
      break;
    case '!':
      GlobalToken[0] = '!';
      CompileGuy.C++;           // Is it just ! or is it != ?
      if (*CompileGuy.C == '=') // It's !=
      {
        GlobalToken[1] = '=';
        GlobalToken[2] = 0;
        CompileGuy.C++;
        break;
      }
      GlobalToken[1] = 0; // It's just !
      break;
    case '=':
      GlobalToken[0] = '=';
      CompileGuy.C++;           // is = or == ?
      if (*CompileGuy.C == '=') // Doesn't really matter,
      {
        GlobalToken[1] = 0; // just skip the last byte
        CompileGuy.C++;
      }
      else
      {
        GlobalToken[1] = 0;
      }
      break;
    case '&':
      GlobalToken[0] = '&';
      GlobalToken[1] = '&';
      GlobalToken[2] = 0;
      CompileGuy.C += 2;
      break;
    default: CompileGuy.C++; // This should be an error.
  }
}

void
GetString()
{
  // printf("GetString\n");
  int Index;

  // Expects a "quoted" string. Places the contents of the string in
  // token[] but does not include the quotes.

  Expect("\"");
  Index = 0;
  while (*CompileGuy.C != '\"')
  {
    GlobalToken[Index] = *CompileGuy.C;
    CompileGuy.C++;
    Index++;
  }
  CompileGuy.C++;
  GlobalToken[Index] = 0;
}

void
GetToken()
{
  // printf("GetToken\n");
  // simply reads in the next statement and places it in the
  // token buffer.

  ParseWhitespace();
  switch (CharTypeLookup[(int)*CompileGuy.C])
  {
    case LETTER:
    {
      // printf("GetToken: LETTER\n");
      GlobalTokenType = IDENTIFIER;
      GetIdentifier();
      break;
    }
    case DIGIT:
    {
      // printf("GetToken: DIGIT\n");
      GlobalTokenType = DIGIT;
      GetNumber();
      break;
    }
    case SPECIAL:
    {
      // printf("GetToken: SPECIAL\n");
      GlobalTokenType = CONTROL;
      GetPunctuation();
      break;
    }
  }

  // NOTE(aen): event{} without newline at EOF shouldn't explode.
  bool IsClosingEvent = GlobalInEvent && GlobalToken[0] == '}';
  if (!*CompileGuy.C && !IsClosingEvent)
  {
    err("Unexpected end of file!");
  }
  // printf("GetToken END\n");
}

b64
NextIs(char *String)
{
  u8 *Pointer;
  // char tt, tst;
  u64 Index;
  u64 oGlobalLines;
  u64 NumericValue;

  Pointer = CompileGuy.C;
  oGlobalLines = GlobalLines;
  // tt = GlobalTokenType;
  // tst = GlobalTokenSubType;
  NumericValue = GlobalTokenNumericValue;
  memcpy(GlobalLastToken, GlobalToken, 2048);
  GetToken();
  CompileGuy.C = Pointer;
  GlobalLines = oGlobalLines;
  GlobalTokenNumericValue = NumericValue;
  // tst = GlobalTokenSubType; // TODO(aen): Is this a bug? Intended to restore?
  // tt = GlobalTokenType;
  if (StringsMatch(String, GlobalToken))
  {
    Index = 1;
  }
  else
  {
    Index = 0;
  }
  memcpy(GlobalToken, GlobalLastToken, 2048);
  return Index;
}

void
Expect(char *String)
{
  // printf("Expect %s\n", str);
  FILE *File;

  GetToken();
  if (StringsMatch(String, GlobalToken))
  {
    // printf("Expect END %s\n", str);
    return;
  }
  if (!CompileGuy.IsQuiet)
  {
    printf(
        "error: %s expected, %s got (%lld)", String, GlobalToken, GlobalLines);
  }
  if (CompileGuy.IsQuiet)
  {
    fopen_s(&File, "error.txt", "w");
    fprintf(
        File,
        "error: %s expected, %s got (%lld) \n",
        String,
        GlobalToken,
        GlobalLines);
    fclose(File);
  }
  exit(-1);
}

u64
ExpectNumber()
{
  GetToken();
  if (GlobalTokenType != DIGIT)
  {
    err("error: Numerical value expected");
  }
  return GlobalTokenNumericValue;
}

void
EmitC(u64 Value)
{
  // printf("EmitC %lld\n", c);
  *CompileGuy.GeneratedCodeLocation++ = (u8)Value;
}

void
EmitW(u64 Value)
{
  // printf("EmitW %lld\n", w);
  u8 *Pointer = (u8 *)&Value;
  *CompileGuy.GeneratedCodeLocation++ = *Pointer++;
  *CompileGuy.GeneratedCodeLocation++ = *Pointer;
}

void
EmitD(u64 Value)
{
  // printf("EmitD %lld\n", w);
  u8 *Pointer = (u8 *)&Value;
  *CompileGuy.GeneratedCodeLocation++ = *Pointer++;
  *CompileGuy.GeneratedCodeLocation++ = *Pointer++;
  *CompileGuy.GeneratedCodeLocation++ = *Pointer++;
  *CompileGuy.GeneratedCodeLocation++ = *Pointer;
}

void
EmitString(char *str)
{
  u64 Index;

  Index = 0;
  while (str[Index])
  {
    *CompileGuy.GeneratedCodeLocation = str[Index];
    CompileGuy.GeneratedCodeLocation++;
    Index++;
  }
  *CompileGuy.GeneratedCodeLocation = 0;
  CompileGuy.GeneratedCodeLocation++;
}

void
HandleOperand()
{
  u64 varidx;

  GetToken();
  if (GlobalTokenType == DIGIT)
  {
    EmitC(OP_IMMEDIATE);
    EmitD(GlobalTokenNumericValue);
  }
  else if (GlobalTokenSubType == VAR0)
  {
    EmitC(OP_VAR0);
    varidx = SearchVarList();
    if (varidx == NUM_VARS0)
    {
      err("error: Unknown identifier.");
    }
    EmitC(varidx);
  }
  else if (GlobalTokenSubType == VAR1)
  {
    EmitC(OP_VAR1);
    varidx = SearchVarList();
    if (varidx == NUM_VARS1)
    {
      err("error: Unknown identifier.");
    }
    EmitC(varidx);
    Expect("(");
    EmitOperand();
    Expect(")");
  }
  else if (GlobalTokenSubType == VAR2)
  {
    EmitC(OP_VAR2);
    varidx = SearchVarList();
    if (varidx == NUM_VARS2)
    {
      err("error: Unknown identifier.");
    }
    EmitC(varidx);
    Expect("(");
    EmitOperand();
    Expect(",");
    EmitOperand();
    Expect(")");
  }
}

void
EmitOperand()
{
  while (1) // Modifier-process loop.
  {
    if (NextIs("("))
    {
      EmitC(OP_GROUP);
      GetToken();
      EmitOperand();
      Expect(")");
    }
    else
    {
      HandleOperand();
    }

    if (NextIs("+"))
    {
      EmitC(ADD);
      GetToken();
      continue;
    }
    else if (NextIs("-"))
    {
      EmitC(SUB);
      GetToken();
      continue;
    }
    else if (NextIs("/"))
    {
      EmitC(DIV);
      GetToken();
      continue;
    }
    else if (NextIs("*"))
    {
      EmitC(MULT);
      GetToken();
      continue;
    }
    else if (NextIs("%"))
    {
      EmitC(MOD);
      GetToken();
      continue;
    }
    else
    {
      EmitC(OP_END);
      break;
    }
  }
}

b64
HandleExpression()
{
  // Parses one single "expression". Can be anything short of a new script.

  GetToken();
  if (GlobalTokenType == FUNCTION)
  {
    OutputCode(GlobalFunctionIndex);
    return 1;
  }
  if (GlobalTokenType == RESERVED && TokenIs("IF"))
  {
    ProcessIf();
    return 1;
  }
  if (GlobalTokenType == RESERVED && TokenIs("FOR"))
  {
    ProcessFor();
    return 1;
  }
  if (GlobalTokenType == RESERVED && TokenIs("WHILE"))
  {
    ProcessWhile();
    return 1;
  }
  if (GlobalTokenType == RESERVED && TokenIs("SWITCH"))
  {
    ProcessSwitch();
    return 1;
  }
  if (GlobalTokenType == RESERVED && TokenIs("GOTO"))
  {
    ProcessGoto();
    return 1;
  }
  if (GlobalTokenType == RESERVED && GlobalTokenSubType == VAR0)
  {
    ProcessVar0Assign();
    return 1;
  }
  if (GlobalTokenType == RESERVED && GlobalTokenSubType == VAR1)
  {
    ProcessVar1Assign();
    return 1;
  }
  if (GlobalTokenType == RESERVED && GlobalTokenSubType == VAR2)
  {
    ProcessVar2Assign();
    return 1;
  }
  if (NextIs(":"))
  {
    memcpy(GlobalLabels[GlobalNumLabels].Identifier, GlobalLastToken, 40);
    GlobalLabels[GlobalNumLabels].Position =
        (u8 *)(CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode);
    if (CompileGuy.IsVerbose)
    {
      printf(
          "label %s found on line %lld, "
          "CompileGuy.GeneratedCodeLocation: %lld. \n",
          GlobalLastToken,
          GlobalLines,
          CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode);
    }
    GlobalNumLabels++;
    Expect(":");
    return 1;
  }
  return 0; // Not a valid command;
}

void
ProcessVar0Assign()
{
  u64 Index;

  EmitC(VAR0_ASSIGN);
  Index = SearchVarList();
  EmitC(Index);
  if (!GlobalStaticVar0WriteList[Index])
    err("error: Variable is read-only.");

  GetToken(); // Find relational operator
  if (TokenIs("="))
  {
    EmitC(SET);
  }
  else if (TokenIs("+="))
  {
    EmitC(INCSET);
  }
  else if (TokenIs("-="))
  {
    EmitC(DECSET);
  }
  else if (TokenIs("++"))
  {
    EmitC(INCREMENT);
    Expect(";");
    return;
  }
  else if (TokenIs("--"))
  {
    EmitC(DECREMENT);
    Expect(";");
    return;
  }

  EmitOperand();
  Expect(";");
}

void
ProcessVar1Assign()
{
  u64 Index;

  EmitC(VAR1_ASSIGN);
  Index = SearchVarList();
  EmitC(Index);
  if (!GlobalStaticVar1WriteList[Index])
  {
    err("error: Variable is read-only.");
  }

  Expect("(");
  EmitOperand();
  Expect(")");

  GetToken(); // Find relational operator
  if (TokenIs("="))
  {
    EmitC(SET);
  }
  else if (TokenIs("+="))
  {
    EmitC(INCSET);
  }
  else if (TokenIs("-="))
  {
    EmitC(DECSET);
  }
  else if (TokenIs("++"))
  {
    EmitC(INCREMENT);
    Expect(";");
    return;
  }
  else if (TokenIs("--"))
  {
    EmitC(DECREMENT);
    Expect(";");
    return;
  }

  EmitOperand();
  Expect(";");
}

void
ProcessVar2Assign()
{
  u64 Index;

  EmitC(VAR2_ASSIGN);
  Index = SearchVarList();
  EmitC(Index);
  if (!GlobalStaticVar2WriteList[Index])
  {
    err("error: Variable is read-only.");
  }

  Expect("(");
  EmitOperand();
  Expect(",");
  EmitOperand();
  Expect(")");

  GetToken(); // Find relational operator
  if (TokenIs("="))
  {
    EmitC(SET);
  }
  else if (TokenIs("+="))
  {
    EmitC(INCSET);
  }
  else if (TokenIs("-="))
  {
    EmitC(DECSET);
  }
  else if (TokenIs("++"))
  {
    EmitC(INCREMENT);
    Expect(";");
    return;
  }
  else if (TokenIs("--"))
  {
    EmitC(DECREMENT);
    Expect(";");
    return;
  }

  EmitOperand();
  Expect(";");
}

void
ProcessIf()
{
  u64 NumArgs = 0;
  b64 IsExclamation = 0;
  u8 *ReturnPointer;
  u8 *Buffer;

  // The general opcode form of an IF is:
  // <BYTE: GENERAL_IF>
  // <BYTE: number of arguements to IF>
  // <DWORD: ptr -> execution branch of IF evaluates false>
  // <1 .. number of arguements: Argument descriptions>

  EmitC(GENERAL_IF);
  Expect("(");
  GlobalNumArgsPointer = CompileGuy.GeneratedCodeLocation;
  EmitC(0); // We'll come back and fix this. <NumArgs>
  ReturnPointer = CompileGuy.GeneratedCodeLocation;
  EmitD(0); // This too.                     <elseofs>

  // Here we begin the loop to write out IF arguements.

  while (1)
  {
    NumArgs++;
    IsExclamation = 0;

    if (NextIs("!"))
    {
      IsExclamation = 1;
      GetToken();
    }

    EmitOperand(); // First Operand

    // Now we need to output the conditional operator, which is a bit more
    // complicated by the possibility of zero/nonzero styles.

    if (IsExclamation)
    {
      EmitC(ZERO);
    }

    GetToken();
    if (TokenIs("&&") || TokenIs(")"))
    {
      if (!IsExclamation)
      {
        EmitC(NONZERO);
      }
      if (TokenIs("&&"))
        continue;
      break;
    }
    if (TokenIs("="))
    {
      EmitC(EQUALTO);
    }
    else if (TokenIs("!="))
    {
      EmitC(NOTEQUAL);
    }
    else if (TokenIs(">"))
    {
      EmitC(GREATERTHAN);
    }
    else if (TokenIs(">="))
    {
      EmitC(GREATERTHANOREQUAL);
    }
    else if (TokenIs("<"))
    {
      EmitC(LESSTHAN);
    }
    else if (TokenIs("<="))
    {
      EmitC(LESSTHANOREQUAL);
    }
    else
    {
      printf("IF %s\n", GlobalToken);
      err("error: Unknown IF relational operator");
    }

    EmitOperand(); // Emit the second operand if applicable

    GetToken(); // See if there are more arguements
    if (TokenIs("&&"))
      continue;
    if (TokenIs(")"))
      break;

    err("error: &&, AND, or ) expected");
  }

  // Now that we've parsed the conditional arguements of the IF, go back to
  // the IF header and set the correct number of arguements.

  Buffer = CompileGuy.GeneratedCodeLocation;
  CompileGuy.GeneratedCodeLocation = GlobalNumArgsPointer;
  EmitC(NumArgs);
  CompileGuy.GeneratedCodeLocation = Buffer;

  if (NextIs("{")) // It's a compound statement.
  {
    Expect("{");
    while (HandleExpression())
    {
      ;
    }
    if (!TokenIs("}"))
    {
      err("error: } expected, or unknown identifier");
    }
    Buffer = CompileGuy.GeneratedCodeLocation;
    CompileGuy.GeneratedCodeLocation = ReturnPointer;
    EmitD(Buffer - CompileGuy.GeneratedCode);
    CompileGuy.GeneratedCodeLocation = Buffer;
    return;
  }
  else // Just a single statement.
  {
    HandleExpression();
    Buffer = CompileGuy.GeneratedCodeLocation;
    CompileGuy.GeneratedCodeLocation = ReturnPointer;
    EmitD(Buffer - CompileGuy.GeneratedCode);
    CompileGuy.GeneratedCodeLocation = Buffer;
  }
}

void
ProcessFor0()
{
  EmitC(FOR_LOOP0);
  EmitC(SearchVarList());

  Expect(",");
  EmitOperand();
  Expect(",");
  EmitOperand();
  Expect(",");
  if (NextIs("-"))
  {
    EmitC(0);
    GetToken();
  }
  else
  {
    EmitC(1);
  }
  EmitOperand();
  Expect(")");

  Expect("{");
  while (HandleExpression())
  {
    ;
  }
  if (!TokenIs("}"))
  {
    err("error: } expected, or unknown identifier");
  }
  EmitC(ENDSCRIPT);
}

void
ProcessFor1()
{
  EmitC(FOR_LOOP1);
  EmitC(SearchVarList());
  Expect("(");
  EmitOperand();
  Expect(")");

  Expect(",");
  EmitOperand();
  Expect(",");
  EmitOperand();
  Expect(",");
  if (NextIs("-"))
  {
    EmitC(0);
    GetToken();
  }
  else
  {
    EmitC(1);
  }
  EmitOperand();
  Expect(")");

  Expect("{");
  while (HandleExpression())
  {
    ;
  }
  if (!TokenIs("}"))
  {
    err("error: } expected, or unknown identifier");
  }
  EmitC(ENDSCRIPT);
}

void
ProcessFor()
{
  Expect("(");
  GetToken();
  if (GlobalTokenSubType == VAR0)
  {
    ProcessFor0();
  }
  else if (GlobalTokenSubType == VAR1)
  {
    ProcessFor1();
  }
  else
  {
    err("Parse error in FOR loop.");
  }
}

void
ProcessWhile()
{
  u64 NumArgs = 0;
  b64 IsExclamation = 0;
  u8 *Buffer;
  u8 *StartPointer;
  u8 *ReturnPointer;

  // The WHILE statement is actually pretty easy to do. It basically has an IF
  // header, and then at the bottom of the IF processing, is a GOTO back to the
  // top. So essentially it will continuously loop until the IF is false.
  // It in fact uses an IF opcode.

  StartPointer = CompileGuy.GeneratedCodeLocation;
  EmitC(GENERAL_IF);
  Expect("(");
  GlobalNumArgsPointer = CompileGuy.GeneratedCodeLocation;
  EmitC(0); // We'll come back and fix this. <NumArgs>
  ReturnPointer = CompileGuy.GeneratedCodeLocation;
  EmitD(0); // This too.                     <elseofs>

  // Here we begin the loop to write out WHILE arguements.

  while (1)
  {
    NumArgs++;
    IsExclamation = 0;

    if (NextIs("!"))
    {
      IsExclamation = 1;
      GetToken();
    }

    EmitOperand(); // First Operand

    // Now we need to output the conditional operator, which is a bit more
    // complicated by the possibility of zero/nonzero styles.

    if (IsExclamation)
    {
      EmitC(ZERO);
    }

    GetToken();
    if (TokenIs("&&") || TokenIs(")"))
    {
      if (!IsExclamation)
      {
        EmitC(NONZERO);
      }
      if (TokenIs("&&"))
        continue;
      break;
    }
    if (TokenIs("="))
    {
      EmitC(EQUALTO);
    }
    else if (TokenIs("!="))
    {
      EmitC(NOTEQUAL);
    }
    else if (TokenIs(">"))
    {
      EmitC(GREATERTHAN);
    }
    else if (TokenIs(">="))
    {
      EmitC(GREATERTHANOREQUAL);
    }
    else if (TokenIs("<"))
    {
      EmitC(LESSTHAN);
    }
    else if (TokenIs("<="))
    {
      EmitC(LESSTHANOREQUAL);
    }
    else
    {
      printf("WHILE %s\n", GlobalToken);
      err("error: Unknown IF relational operator");
    }

    EmitOperand(); // Emit the second operand if applicable

    GetToken(); // See if there are more arguements
    if (TokenIs("&&"))
      continue;
    if (TokenIs(")"))
      break;

    err("error: &&, AND, or ) expected");
  }

  // Now that we've parsed the conditional arguements of the WHILE, go back to
  // the WHILE header and set the correct number of arguements.

  Buffer = CompileGuy.GeneratedCodeLocation;
  CompileGuy.GeneratedCodeLocation = GlobalNumArgsPointer;
  EmitC(NumArgs);
  CompileGuy.GeneratedCodeLocation = Buffer;

  if (NextIs("{")) // It's a compound statement.
  {
    Expect("{");
    while (HandleExpression())
    {
      ;
    }
    if (!TokenIs("}"))
    {
      err("error: } expected, or unknown identifier");
    }
    EmitC(GOTO);
    EmitD(StartPointer - CompileGuy.GeneratedCode);
    Buffer = CompileGuy.GeneratedCodeLocation;
    CompileGuy.GeneratedCodeLocation = ReturnPointer;
    EmitD(Buffer - CompileGuy.GeneratedCode);
    CompileGuy.GeneratedCodeLocation = Buffer;
    return;
  }
  else // Just a single statement.
  {
    HandleExpression();
    EmitC(GOTO);
    EmitD(StartPointer - CompileGuy.GeneratedCode);
    Buffer = CompileGuy.GeneratedCodeLocation;
    CompileGuy.GeneratedCodeLocation = ReturnPointer;
    EmitD(Buffer - CompileGuy.GeneratedCode);
    CompileGuy.GeneratedCodeLocation = Buffer;
  }
}

void
ProcessSwitch()
{
  u8 *Buffer;
  u8 *ReturnPointer;

  // Special thanks for Zeromus for giving me a good idea on how to implement
  // this... Even tho I changed his idea around a bit :)

  EmitC(SWITCH);
  Expect("(");
  EmitOperand();
  Expect(")");
  Expect("{");

  // case .. option loop

  while (!NextIs("}"))
  {
    Expect("CASE");
    EmitC(CASE);
    EmitOperand();
    Expect(":");
    ReturnPointer = CompileGuy.GeneratedCodeLocation;
    EmitD(0);
    while (!NextIs("CASE") && !NextIs("}"))
    {
      HandleExpression();
    }
    EmitC(ENDSCRIPT);
    Buffer = CompileGuy.GeneratedCodeLocation;
    CompileGuy.GeneratedCodeLocation = ReturnPointer;
    EmitD(Buffer - CompileGuy.GeneratedCode);
    CompileGuy.GeneratedCodeLocation = Buffer;
  }
  Expect("}");
  EmitC(ENDSCRIPT);
}

void
ProcessGoto()
{
  EmitC(GOTO);
  GetToken();
  memcpy(&GlobalGotos[GlobalNumGotos].Identifier, GlobalToken, 40);
  if (CompileGuy.IsVerbose)
  {
    printf(
        "GOTO tagged on line %lld at "
        "CompileGuy.GeneratedCodeLocation %lld, label %s. \n",
        GlobalLines,
        CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode,
        GlobalToken);
  }
  GlobalGotos[GlobalNumGotos].Position = CompileGuy.GeneratedCodeLocation;
  EmitD(0);
  GlobalNumGotos++;
  Expect(";");
}

void
ProcessEvent()
{
  // printf("*** ProcessEvent\n");
  Expect("{");
  GlobalInEvent = 1;
  GlobalScriptOffsetTable[GlobalNumScripts] = SafeTruncateU64(
      CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode);
  GlobalNumScripts++;

  while (1)
  {
    GetToken();
    if (GlobalTokenType == CONTROL)
    {
      if (!TokenIs("}"))
      {
        err("error: Identifier expected");
      }
      else
      {
        EmitC(ENDSCRIPT); // End of script
        GlobalInEvent = 0;
        return;
      }
    }

    if (GlobalTokenType == RESERVED)
    {
      if (TokenIs("IF"))
      {
        ProcessIf();
        continue;
      }
      if (TokenIs("FOR"))
      {
        ProcessFor();
        continue;
      }
      if (TokenIs("WHILE"))
      {
        ProcessWhile();
        continue;
      }
      if (TokenIs("SWITCH"))
      {
        ProcessSwitch();
        continue;
      }
      if (TokenIs("GOTO"))
      {
        ProcessGoto();
        continue;
      }
    }
    if (GlobalTokenType == RESERVED && GlobalTokenSubType == VAR0)
    {
      ProcessVar0Assign();
      continue;
    }
    if (GlobalTokenType == RESERVED && GlobalTokenSubType == VAR1)
    {
      ProcessVar1Assign();
      continue;
    }
    if (GlobalTokenType == RESERVED && GlobalTokenSubType == VAR2)
    {
      ProcessVar2Assign();
      continue;
    }
    if (GlobalTokenType != FUNCTION && NextIs(":"))
    {
      memcpy(GlobalLabels[GlobalNumLabels].Identifier, GlobalLastToken, 40);
      GlobalLabels[GlobalNumLabels].Position =
          (u8 *)(CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode);
      if (CompileGuy.IsVerbose)
      {
        printf(
            "label %s found on line %lld, "
            "CompileGuy.GeneratedCodeLocation: %lld. \n",
            GlobalLastToken,
            GlobalLines,
            CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode);
      }
      GlobalNumLabels++;
      Expect(":");
      continue;
    }
    if (GlobalTokenType != FUNCTION)
    {
      err("error: Function-identifier expected.");
    }
    OutputCode(GlobalFunctionIndex);
  }
}

u8 *
GetLabelAddr(char *String)
{
  u64 Index;

  for (Index = 0; Index < GlobalNumLabels; Index++)
  {
    if (StringsMatch(String, GlobalLabels[Index].Identifier))
    {
      return GlobalLabels[Index].Position;
    }
  }

  sprintf_s(GlobalToken, 2048, "Undefined label %s.", String);
  err(GlobalToken);
  return 0;
}

void
ResolveGotos()
{
  // printf("ResolveGotos\n");
  u8 *LabelAddress;
  u8 *SavePointer;
  u64 Index;

  SavePointer = CompileGuy.GeneratedCodeLocation;
  for (Index = 0; Index < GlobalNumGotos; Index++)
  {
    LabelAddress = GetLabelAddr(GlobalGotos[Index].Identifier);
    if (CompileGuy.IsVerbose)
    {
      printf(
          "resolving goto %lld (%s) as %s at "
          "CompileGuy.GeneratedCodeLocation %lld. \n",
          Index,
          GlobalGotos[Index].Identifier,
          LabelAddress,
          GlobalGotos[Index].Position - CompileGuy.GeneratedCode);
    }
    CompileGuy.GeneratedCodeLocation = GlobalGotos[Index].Position;
    EmitD((u64)LabelAddress);
  }
  CompileGuy.GeneratedCodeLocation = SavePointer;
}

b64
Parse()
{
  // printf("Parse\n");

  GlobalToken[0] = 0; // clear token-buffer
  ParseWhitespace();  // Sift through any whitespace.
  if (!*CompileGuy.C)
  {
    // printf("Parse: EOF\n");
    return 1; // EOF
  }
  GetToken(); // Grab next token

  if (TokenIs(GlobalScriptToken))
  {
    ProcessEvent();
  }
  ParseWhitespace();
  if (!*CompileGuy.C)
  {
    DebugLog(MEDIUM, "Parse: zero byte\n");
    return 1;
  }
  if (!NextIs(GlobalScriptToken) && !GlobalInExternal)
  {
    if (!CompileGuy.IsQuiet)
    {
      printf(
          "warning: Unknown token '%s' outside scripts (%lld) \n",
          GlobalToken,
          GlobalLines);
    }
    GlobalInExternal = 1;
  }
  return 0;
}

// NOTE(aen): Does not include # scripts and script offset table prefix.
void
CompileToBuffer(
    u64 Type,
    char *Input,
    u8 *Output,
    u64 OutputLimit,
    u64 *GeneratedByteCount)
{
  switch (Type)
  {
    case COMPILE_TYPE_MAP:
    case COMPILE_TYPE_STARTUP:
    case COMPILE_TYPE_EFFECT:
    case COMPILE_TYPE_MAGIC: break;
    default: Fail("Unknown Type: %d\n", Type);
  }
  Assert(Input);
  Assert(Output);

  // Log("CompileToBuffer: Type %d, Input: %s\n", Type, Input);

  Compile(Type, (u8 *)Input, Output);

  u64 GeneratedLength =
      CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode;
  // Log("GeneratedLength %d\n", GeneratedLength);
  *GeneratedByteCount = GeneratedLength;

  if (GeneratedLength > OutputLimit)
    Fail(
        "Generated %d bytes byte output buffer only has room for %d bytes\n",
        GeneratedLength,
        OutputLimit);

  memcpy(Output, CompileGuy.GeneratedCode, GeneratedLength);
  Output[GeneratedLength] = 0;
}

void
Compile(u64 Type, u8 *Input, u8 *Output)
{
  // Log("Compile\n");

  // NOTE(aen): Very important when compiling a lot, like in tests.
  GlobalNumScripts = 0;
  GlobalLines = 1;
  GlobalInEvent = 0;
  GlobalInExternal = 0;
  GlobalFunctionIndex = 0;
  GlobalNumLabels = 0;
  GlobalNumGotos = 0;

  if (!Input)
    Input = CompileGuy.Data;
  if (!Output)
    Output = CompileGuy.GeneratedCode;

  CompileGuy.Data = Input;
  CompileGuy.C = Input;
  CompileGuy.GeneratedCode = Output;
  CompileGuy.GeneratedCodeLocation = Output;

  switch (Type)
  {
    case COMPILE_TYPE_MAP: GlobalScriptToken = "EVENT"; break;
    case COMPILE_TYPE_STARTUP: GlobalScriptToken = "SCRIPT"; break;
    case COMPILE_TYPE_EFFECT: GlobalScriptToken = "EFFECT"; break;
    case COMPILE_TYPE_MAGIC: GlobalScriptToken = "EFFECT"; break;
  }

  while (!Parse()) {}

  ResolveGotos();

  // if (!CompileGuy.IsQuiet) {
  //   printf("%lld scripts successfully compiled. (%lld lines) \n",
  //   GlobalNumScripts,
  //       GlobalLines);
  // }
}

void
InitCharTypeLookup()
{
  u64 Index;

  if (CompileGuy.IsVerbose)
    printf("Building CharTypeLookup[]. \n");
  for (Index = 0; Index < 256; Index++)
    CharTypeLookup[Index] = SPECIAL;
  for (Index = '0'; Index <= '9'; Index++)
    CharTypeLookup[Index] = DIGIT;
  for (Index = 'A'; Index <= 'Z'; Index++)
    CharTypeLookup[Index] = LETTER;
  for (Index = 'a'; Index <= 'z'; Index++)
    CharTypeLookup[Index] = LETTER;

  CharTypeLookup[10] = 0;
  CharTypeLookup[13] = 0;
  CharTypeLookup[' '] = 0;
  CharTypeLookup['_'] = LETTER;
  CharTypeLookup['.'] = LETTER;
}

b64
IsCharType(u64 Char, u64 Type)
{
  return CharTypeLookup[Char] == Type;
}
