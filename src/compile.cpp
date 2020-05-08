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
#include "code.hpp"
#include "funclib.hpp"
#include "libfuncs.hpp"
#include "log.hpp"
#include "util.hpp"
#include "vcc.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================== Variables ==============================

struct label labels[200]; // goto labels
struct label gotos[200];  // goto occurence records
char token[2048];         // current token buffer
char lasttoken[2048];     // restorebuf for NextIs
u64 token_nvalue;         // int value of token if it's type DIGIT
u64 token_type;           // type of current token.
u64 token_subtype;        // This is just crap.
u8 *numargsptr;           // number of arguements to IF ptr
u32 scriptofstbl[1024];   // script offset table

u64 numscripts = 0; // number of scripts in the VC file
u64 lines = 1;

// Compilation-state flags
bool64 inevent = 0;
bool64 iex = 0;
const char *scripttoken;
u64 funcidx;
u64 numlabels = 0;
u64 numgotos = 0;

// ================================ Code =================================

void
err(const char *str)
{
  FILE *f;

  if (!CompileGuy.IsQuiet)
    printf("%s (%lld) \n", str, lines);
  if (CompileGuy.IsQuiet)
  {
    fopen_s(&f, "error.txt", "w");
    fprintf(f, "%s (%lld)\n", str, lines);
    fclose(f);
  }
  if (Exist("$$tmep$$.ma_"))
    remove("$$tmep$$.ma_");
  exit(-1);
}

bool64
TokenIs(const char *str)
{
  if (!strcmp(str, token))
    return 1;
  else
    return 0;
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
        lines++;
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
          lines++;
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
  int i;

  // If the current token is a recognized library function, sets
  // token_type to FUNCTION and funcidx to the appropriate function code.

  // Todo: Maybe replace this with a binary tree instead of a linear
  // search. It hasn't gotten slow enough yet tho. :)

  token_nvalue = 0;

  if (strlen(token) == 1 && token[0] > 64 && token[0] < 91)
  {
    token_type = IDENTIFIER;
    token_nvalue = token[0] - 64;
    return;
  }

  if (TokenIs("FLAGS") || TokenIs("IF") || TokenIs("FOR") || TokenIs("WHILE") ||
      TokenIs("SWITCH") || TokenIs("CASE") || TokenIs("GOTO"))
  {
    token_type = RESERVED;
    return;
  }
  if (TokenIs("AND"))
  {
    token_type = CONTROL;
    token[0] = '&';
    token[1] = '&';
    token[2] = 0;
    return;
  }
  i = 0;
  while (i < numfuncs)
  {
    if (!strcmp(funcs[i], token))
      break;
    i++;
  }
  if (i != numfuncs)
  {
    token_type = FUNCTION;
    // printf("CheckLibFunc: Found %lld (%s)\n", i, funcs[i]);
  }
  funcidx = i;
}

u64
SearchVarList()
{
  u64 i;

  i = 0;
  while (i < numvars0)
  {
    if (!strcmp(vars0[i], token))
      break;
    i++;
  }
  if (i != numvars0)
  {
    token_type = RESERVED;
    token_subtype = VAR0;
    return i;
  }

  i = 0;
  while (i < numvars1)
  {
    if (!strcmp(vars1[i], token))
      break;
    i++;
  }
  if (i != numvars1)
  {
    token_type = RESERVED;
    token_subtype = VAR1;
    return i;
  }

  i = 0;
  while (i < numvars2)
  {
    if (!strcmp(vars2[i], token))
      break;
    i++;
  }
  if (i != numvars2)
  {
    token_type = RESERVED;
    token_subtype = VAR2;
    return i;
  }
  return i;
}

void
GetIdentifier()
{
  // printf("GetIdentifier\n");
  int i;

  // Retrieves an identifier from the source buffer. Before calling this,
  // it should be guaranteed that the first character is a letter. A check
  // will need to be made afterward to see if it's a reserved word.

  i = 0;
  while ((CharTypeLookup[(int)*CompileGuy.C] == LETTER) ||
         (CharTypeLookup[(int)*CompileGuy.C] == DIGIT))
  {
    token[i] = *CompileGuy.C;
    CompileGuy.C++;
    i++;
  }
  token[i] = 0;
  _strupr_s(token, 2048);
  CheckLibFunc();
  SearchVarList();
}

void
GetNumber()
{
  // printf("GetNumber\n");
  int i;

  // Grabs the next number. String version remains in token[], numerical
  // version is placed in token_nvalue.

  i = 0;
  while (CharTypeLookup[(int)*CompileGuy.C] == DIGIT)
  {
    token[i] = *CompileGuy.C;
    CompileGuy.C++;
    i++;
  }
  token[i] = 0;
  token_nvalue = atoi(token);
}

void
GetPunctuation()
{
  // printf("GetPunctuation\n");
  u64 c;

  // Grabs the next recognized punctuation type. If a double-char punctuation
  // type is recognized, it will be returned. Ie, differentiate b/w = and ==.

  c = *CompileGuy.C;
  switch (c)
  {
    case '(':
      token[0] = '(';
      token[1] = 0;
      CompileGuy.C++;
      break;
    case ')':
      token[0] = ')';
      token[1] = 0;
      CompileGuy.C++;
      break;
    case '{':
      token[0] = '{';
      token[1] = 0;
      CompileGuy.C++;
      break;
    case '}':
      token[0] = '}';
      token[1] = 0;
      CompileGuy.C++;
      break;
    case '[':
      token[0] = '(';
      token[1] = 0;
      CompileGuy.C++;
      break;
    case ']':
      token[0] = ')';
      token[1] = 0;
      CompileGuy.C++;
      break;
    case ',':
      token[0] = ',';
      token[1] = 0;
      CompileGuy.C++;
      break;
    case ':':
      token[0] = ':';
      token[1] = 0;
      CompileGuy.C++;
      break;
    case ';':
      token[0] = ';';
      token[1] = 0;
      CompileGuy.C++;
      break;
    case '/':
      token[0] = '/';
      token[1] = 0;
      CompileGuy.C++;
      break;
    case '*':
      token[0] = '*';
      token[1] = 0;
      CompileGuy.C++;
      break;
    case '%':
      token[0] = '%';
      token[1] = 0;
      CompileGuy.C++;
      break;
    case '\"':
      token[0] = '\"';
      token[1] = 0;
      CompileGuy.C++;
      break;
    case '\'':
      token[0] = '\'';
      token[1] = 0;
      CompileGuy.C++;
      break;
    case '+':
      token[0] = '+'; // Is it ++ or += or +?
      CompileGuy.C++;
      if (*CompileGuy.C == '+')
      {
        token[1] = '+';
        CompileGuy.C++;
      }
      else if (*CompileGuy.C == '=')
      {
        token[1] = '=';
        CompileGuy.C++;
      }
      else
      {
        token[1] = 0;
      }
      token[2] = 0;
      break;
    case '-':
      token[0] = '-'; // Is it -- or -= or -?
      CompileGuy.C++;
      if (*CompileGuy.C == '-')
      {
        token[1] = '-';
        CompileGuy.C++;
      }
      else if (*CompileGuy.C == '=')
      {
        token[1] = '=';
        CompileGuy.C++;
      }
      else
      {
        token[1] = 0;
      }
      token[2] = 0;
      break;
    case '>':
      token[0] = '>'; // Is it > or >=?
      CompileGuy.C++;
      if (*CompileGuy.C == '=') // It's >=
      {
        token[1] = '=';
        token[2] = 0;
        CompileGuy.C++;
        break;
      }
      token[1] = 0; // It's >
      break;
    case '<':
      token[0] = '<'; // Is it < or <=?
      CompileGuy.C++;
      if (*CompileGuy.C == '=') // It's <=
      {
        token[1] = '=';
        token[2] = 0;
        CompileGuy.C++;
        break;
      }
      token[1] = 0; // It's <
      break;
    case '!':
      token[0] = '!';
      CompileGuy.C++;           // Is it just ! or is it != ?
      if (*CompileGuy.C == '=') // It's !=
      {
        token[1] = '=';
        token[2] = 0;
        CompileGuy.C++;
        break;
      }
      token[1] = 0; // It's just !
      break;
    case '=':
      token[0] = '=';
      CompileGuy.C++;           // is = or == ?
      if (*CompileGuy.C == '=') // Doesn't really matter,
      {
        token[1] = 0; // just skip the last byte
        CompileGuy.C++;
      }
      else
      {
        token[1] = 0;
      }
      break;
    case '&':
      token[0] = '&';
      token[1] = '&';
      token[2] = 0;
      CompileGuy.C += 2;
      break;
    default: CompileGuy.C++; // This should be an error.
  }
}

void
GetString()
{
  // printf("GetString\n");
  int i;

  // Expects a "quoted" string. Places the contents of the string in
  // token[] but does not include the quotes.

  Expect("\"");
  i = 0;
  while (*CompileGuy.C != '\"')
  {
    token[i] = *CompileGuy.C;
    CompileGuy.C++;
    i++;
  }
  CompileGuy.C++;
  token[i] = 0;
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
      token_type = IDENTIFIER;
      GetIdentifier();
      break;
    }
    case DIGIT:
    {
      // printf("GetToken: DIGIT\n");
      token_type = DIGIT;
      GetNumber();
      break;
    }
    case SPECIAL:
    {
      // printf("GetToken: SPECIAL\n");
      token_type = CONTROL;
      GetPunctuation();
      break;
    }
  }

  // NOTE(aen): event{} without newline at EOF shouldn't explode.
  bool IsClosingEvent = inevent && token[0] == '}';
  if (!*CompileGuy.C && !IsClosingEvent)
  {
    err("Unexpected end of file!");
  }
  // printf("GetToken END\n");
}

bool64
NextIs(const char *str)
{
  u8 *ptr;
  // char tt, tst;
  u64 i;
  u64 olines, nv;

  ptr = CompileGuy.C;
  olines = lines;
  // tt = token_type;
  // tst = token_subtype;
  nv = token_nvalue;
  memcpy(lasttoken, token, 2048);
  GetToken();
  CompileGuy.C = ptr;
  lines = olines;
  token_nvalue = nv;
  // tst = token_subtype; // TODO(aen): Is this a bug? Intended to restore?
  // tt = token_type;
  if (!strcmp(str, token))
  {
    i = 1;
  }
  else
  {
    i = 0;
  }
  memcpy(token, lasttoken, 2048);
  return i;
}

void
Expect(const char *str)
{
  // printf("Expect %s\n", str);
  FILE *f;

  GetToken();
  if (!strcmp(str, token))
  {
    // printf("Expect END %s\n", str);
    return;
  }
  if (!CompileGuy.IsQuiet)
  {
    printf("error: %s expected, %s got (%lld)", str, token, lines);
  }
  if (CompileGuy.IsQuiet)
  {
    fopen_s(&f, "error.txt", "w");
    fprintf(f, "error: %s expected, %s got (%lld) \n", str, token, lines);
    fclose(f);
  }
  exit(-1);
}

u64
ExpectNumber()
{
  GetToken();
  if (token_type != DIGIT)
  {
    err("error: Numerical value expected");
  }
  return token_nvalue;
}

void
EmitC(u64 c)
{
  // printf("EmitC %lld\n", c);
  *CompileGuy.GeneratedCodeLocation = (u8)c;
  CompileGuy.GeneratedCodeLocation++;
}

void
EmitW(u64 w)
{
  // printf("EmitW %lld\n", w);
  u8 *ptr;

  ptr = (u8 *)&w;
  *CompileGuy.GeneratedCodeLocation = *ptr;
  CompileGuy.GeneratedCodeLocation++;
  ptr++;
  *CompileGuy.GeneratedCodeLocation = *ptr;
  CompileGuy.GeneratedCodeLocation++;
}

void
EmitD(u64 w)
{
  // printf("EmitD %lld\n", w);
  u8 *ptr;

  ptr = (u8 *)&w;
  *CompileGuy.GeneratedCodeLocation = *ptr;
  CompileGuy.GeneratedCodeLocation++;
  ptr++;
  *CompileGuy.GeneratedCodeLocation = *ptr;
  CompileGuy.GeneratedCodeLocation++;
  ptr++;
  *CompileGuy.GeneratedCodeLocation = *ptr;
  CompileGuy.GeneratedCodeLocation++;
  ptr++;
  *CompileGuy.GeneratedCodeLocation = *ptr;
  CompileGuy.GeneratedCodeLocation++;
}

void
EmitString(const char *str)
{
  u64 i;

  i = 0;
  while (str[i])
  {
    *CompileGuy.GeneratedCodeLocation = str[i];
    CompileGuy.GeneratedCodeLocation++;
    i++;
  }
  *CompileGuy.GeneratedCodeLocation = 0;
  CompileGuy.GeneratedCodeLocation++;
}

void
HandleOperand()
{
  u64 varidx;

  GetToken();
  if (token_type == DIGIT)
  {
    EmitC(OP_IMMEDIATE);
    EmitD(token_nvalue);
  }
  else if (token_subtype == VAR0)
  {
    EmitC(OP_VAR0);
    varidx = SearchVarList();
    if (varidx == numvars0)
    {
      err("error: Unknown identifier.");
    }
    EmitC(varidx);
  }
  else if (token_subtype == VAR1)
  {
    EmitC(OP_VAR1);
    varidx = SearchVarList();
    if (varidx == numvars1)
    {
      err("error: Unknown identifier.");
    }
    EmitC(varidx);
    Expect("(");
    EmitOperand();
    Expect(")");
  }
  else if (token_subtype == VAR2)
  {
    EmitC(OP_VAR2);
    varidx = SearchVarList();
    if (varidx == numvars2)
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

bool64
HandleExpression()
{
  // Parses one single "expression". Can be anything short of a new script.

  GetToken();
  if (token_type == FUNCTION)
  {
    OutputCode(funcidx);
    return 1;
  }
  if (token_type == RESERVED && TokenIs("IF"))
  {
    ProcessIf();
    return 1;
  }
  if (token_type == RESERVED && TokenIs("FOR"))
  {
    ProcessFor();
    return 1;
  }
  if (token_type == RESERVED && TokenIs("WHILE"))
  {
    ProcessWhile();
    return 1;
  }
  if (token_type == RESERVED && TokenIs("SWITCH"))
  {
    ProcessSwitch();
    return 1;
  }
  if (token_type == RESERVED && TokenIs("GOTO"))
  {
    ProcessGoto();
    return 1;
  }
  if (token_type == RESERVED && token_subtype == VAR0)
  {
    ProcessVar0Assign();
    return 1;
  }
  if (token_type == RESERVED && token_subtype == VAR1)
  {
    ProcessVar1Assign();
    return 1;
  }
  if (token_type == RESERVED && token_subtype == VAR2)
  {
    ProcessVar2Assign();
    return 1;
  }
  if (NextIs(":"))
  {
    memcpy(labels[numlabels].ident, lasttoken, 40);
    labels[numlabels].pos =
        (u8 *)(CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode);
    if (CompileGuy.IsVerbose)
    {
      printf(
          "label %s found on line %lld, "
          "CompileGuy.GeneratedCodeLocation: %lld. \n",
          lasttoken,
          lines,
          CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode);
    }
    numlabels++;
    Expect(":");
    return 1;
  }
  return 0; // Not a valid command;
}

void
ProcessVar0Assign()
{
  u64 a;

  EmitC(VAR0_ASSIGN);
  a = SearchVarList();
  EmitC(a);
  if (!write0[a])
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
  u64 a;

  EmitC(VAR1_ASSIGN);
  a = SearchVarList();
  EmitC(a);
  if (!write1[a])
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
  u64 a;

  EmitC(VAR2_ASSIGN);
  a = SearchVarList();
  EmitC(a);
  if (!write2[a])
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
  u64 numargs = 0;
  bool64 excl = 0;
  u8 *returnptr;
  u8 *buf;

  // The general opcode form of an IF is:
  // <BYTE: GENERAL_IF>
  // <BYTE: number of arguements to IF>
  // <DWORD: ptr -> execution branch of IF evaluates false>
  // <1 .. number of arguements: Argument descriptions>

  EmitC(GENERAL_IF);
  Expect("(");
  numargsptr = CompileGuy.GeneratedCodeLocation;
  EmitC(0); // We'll come back and fix this. <numargs>
  returnptr = CompileGuy.GeneratedCodeLocation;
  EmitD(0); // This too.                     <elseofs>

  // Here we begin the loop to write out IF arguements.

  while (1)
  {
    numargs++;
    excl = 0;

    if (NextIs("!"))
    {
      excl = 1;
      GetToken();
    }

    EmitOperand(); // First Operand

    // Now we need to output the conditional operator, which is a bit more
    // complicated by the possibility of zero/nonzero styles.

    if (excl)
    {
      EmitC(ZERO);
    }

    GetToken();
    if (TokenIs("&&") || TokenIs(")"))
    {
      if (!excl)
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
      printf("IF %s\n", token);
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

  buf = CompileGuy.GeneratedCodeLocation;
  CompileGuy.GeneratedCodeLocation = numargsptr;
  EmitC(numargs);
  CompileGuy.GeneratedCodeLocation = buf;

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
    buf = CompileGuy.GeneratedCodeLocation;
    CompileGuy.GeneratedCodeLocation = returnptr;
    EmitD(buf - CompileGuy.GeneratedCode);
    CompileGuy.GeneratedCodeLocation = buf;
    return;
  }
  else // Just a single statement.
  {
    HandleExpression();
    buf = CompileGuy.GeneratedCodeLocation;
    CompileGuy.GeneratedCodeLocation = returnptr;
    EmitD(buf - CompileGuy.GeneratedCode);
    CompileGuy.GeneratedCodeLocation = buf;
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
  if (token_subtype == VAR0)
  {
    ProcessFor0();
  }
  else if (token_subtype == VAR1)
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
  u64 numargs = 0;
  bool64 excl = 0;
  u8 *buf, *start, *returnptr;

  // The WHILE statement is actually pretty easy to do. It basically has an IF
  // header, and then at the bottom of the IF processing, is a GOTO back to the
  // top. So essentially it will continuously loop until the IF is false.
  // It in fact uses an IF opcode.

  start = CompileGuy.GeneratedCodeLocation;
  EmitC(GENERAL_IF);
  Expect("(");
  numargsptr = CompileGuy.GeneratedCodeLocation;
  EmitC(0); // We'll come back and fix this. <numargs>
  returnptr = CompileGuy.GeneratedCodeLocation;
  EmitD(0); // This too.                     <elseofs>

  // Here we begin the loop to write out WHILE arguements.

  while (1)
  {
    numargs++;
    excl = 0;

    if (NextIs("!"))
    {
      excl = 1;
      GetToken();
    }

    EmitOperand(); // First Operand

    // Now we need to output the conditional operator, which is a bit more
    // complicated by the possibility of zero/nonzero styles.

    if (excl)
    {
      EmitC(ZERO);
    }

    GetToken();
    if (TokenIs("&&") || TokenIs(")"))
    {
      if (!excl)
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
      printf("WHILE %s\n", token);
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

  buf = CompileGuy.GeneratedCodeLocation;
  CompileGuy.GeneratedCodeLocation = numargsptr;
  EmitC(numargs);
  CompileGuy.GeneratedCodeLocation = buf;

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
    EmitD(start - CompileGuy.GeneratedCode);
    buf = CompileGuy.GeneratedCodeLocation;
    CompileGuy.GeneratedCodeLocation = returnptr;
    EmitD(buf - CompileGuy.GeneratedCode);
    CompileGuy.GeneratedCodeLocation = buf;
    return;
  }
  else // Just a single statement.
  {
    HandleExpression();
    EmitC(GOTO);
    EmitD(start - CompileGuy.GeneratedCode);
    buf = CompileGuy.GeneratedCodeLocation;
    CompileGuy.GeneratedCodeLocation = returnptr;
    EmitD(buf - CompileGuy.GeneratedCode);
    CompileGuy.GeneratedCodeLocation = buf;
  }
}

void
ProcessSwitch()
{
  u8 *buf, *retrptr;

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
    retrptr = CompileGuy.GeneratedCodeLocation;
    EmitD(0);
    while (!NextIs("CASE") && !NextIs("}"))
    {
      HandleExpression();
    }
    EmitC(ENDSCRIPT);
    buf = CompileGuy.GeneratedCodeLocation;
    CompileGuy.GeneratedCodeLocation = retrptr;
    EmitD(buf - CompileGuy.GeneratedCode);
    CompileGuy.GeneratedCodeLocation = buf;
  }
  Expect("}");
  EmitC(ENDSCRIPT);
}

void
ProcessGoto()
{
  EmitC(GOTO);
  GetToken();
  memcpy(&gotos[numgotos].ident, token, 40);
  if (CompileGuy.IsVerbose)
  {
    printf(
        "GOTO tagged on line %lld at "
        "CompileGuy.GeneratedCodeLocation %lld, label %s. \n",
        lines,
        CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode,
        token);
  }
  gotos[numgotos].pos = CompileGuy.GeneratedCodeLocation;
  EmitD(0);
  numgotos++;
  Expect(";");
}

void
ProcessEvent()
{
  // printf("*** ProcessEvent\n");
  Expect("{");
  inevent = 1;
  scriptofstbl[numscripts] =
      (u32)(CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode);
  numscripts++;

  while (1)
  {
    GetToken();
    if (token_type == CONTROL)
    {
      if (!TokenIs("}"))
      {
        err("error: Identifier expected");
      }
      else
      {
        EmitC(ENDSCRIPT); // End of script
        inevent = 0;
        return;
      }
    }

    if (token_type == RESERVED)
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
    if (token_type == RESERVED && token_subtype == VAR0)
    {
      ProcessVar0Assign();
      continue;
    }
    if (token_type == RESERVED && token_subtype == VAR1)
    {
      ProcessVar1Assign();
      continue;
    }
    if (token_type == RESERVED && token_subtype == VAR2)
    {
      ProcessVar2Assign();
      continue;
    }
    if (token_type != FUNCTION && NextIs(":"))
    {
      memcpy(labels[numlabels].ident, lasttoken, 40);
      labels[numlabels].pos =
          (u8 *)(CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode);
      if (CompileGuy.IsVerbose)
      {
        printf(
            "label %s found on line %lld, "
            "CompileGuy.GeneratedCodeLocation: %lld. \n",
            lasttoken,
            lines,
            CompileGuy.GeneratedCodeLocation - CompileGuy.GeneratedCode);
      }
      numlabels++;
      Expect(":");
      continue;
    }
    if (token_type != FUNCTION)
    {
      err("error: Function-identifier expected.");
    }
    OutputCode(funcidx);
  }
}

u8 *
GetLabelAddr(char *str)
{
  u64 i;

  for (i = 0; i < numlabels; i++)
  {
    if (!strcmp(str, labels[i].ident))
    {
      return labels[i].pos;
    }
  }

  sprintf_s(token, 2048, "Undefined label %s.", str);
  err(token);
  return 0;
}

void
ResolveGotos()
{
  // printf("ResolveGotos\n");
  u8 *a, *ocp;
  u64 i;

  ocp = CompileGuy.GeneratedCodeLocation;
  for (i = 0; i < numgotos; i++)
  {
    a = GetLabelAddr(gotos[i].ident);
    if (CompileGuy.IsVerbose)
    {
      printf(
          "resolving goto %lld (%s) as %s at "
          "CompileGuy.GeneratedCodeLocation %lld. \n",
          i,
          gotos[i].ident,
          a,
          gotos[i].pos - CompileGuy.GeneratedCode);
    }
    CompileGuy.GeneratedCodeLocation = gotos[i].pos;
    EmitD((u64)a);
  }
  CompileGuy.GeneratedCodeLocation = ocp;
}

bool64
Parse()
{
  // printf("Parse\n");

  token[0] = 0;      // clear token-buffer
  ParseWhitespace(); // Sift through any whitespace.
  if (!*CompileGuy.C)
  {
    // printf("Parse: EOF\n");
    return 1; // EOF
  }
  GetToken(); // Grab next token

  if (TokenIs(scripttoken))
  {
    ProcessEvent();
  }
  ParseWhitespace();
  if (!*CompileGuy.C)
  {
    return 1;
  }
  if (!NextIs(scripttoken) && !iex)
  {
    if (!CompileGuy.IsQuiet)
    {
      printf(
          "warning: Unknown token '%s' outside scripts (%lld) \n",
          token,
          lines);
    }
    iex = 1;
  }
  return 0;
}

// NOTE(aen): Does not include # scripts and script offset table prefix.
void
CompileToBuffer(
    u64 Type,
    const char *Input,
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
  ASSERT(Input);
  ASSERT(Output);

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
  numscripts = 0;
  lines = 1;
  inevent = 0;
  iex = 0;
  funcidx = 0;
  numlabels = 0;
  numgotos = 0;

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
    case COMPILE_TYPE_MAP: scripttoken = "EVENT"; break;
    case COMPILE_TYPE_STARTUP: scripttoken = "SCRIPT"; break;
    case COMPILE_TYPE_EFFECT: scripttoken = "EFFECT"; break;
    case COMPILE_TYPE_MAGIC: scripttoken = "EFFECT"; break;
  }

  while (!Parse()) {}

  ResolveGotos();

  // if (!CompileGuy.IsQuiet) {
  //   printf("%lld scripts successfully compiled. (%lld lines) \n", numscripts,
  //       lines);
  // }
}

void
InitCharTypeLookup()
{
  u64 i;

  if (CompileGuy.IsVerbose)
    printf("Building CharTypeLookup[]. \n");
  for (i = 0; i < 256; i++)
    CharTypeLookup[i] = SPECIAL;
  for (i = '0'; i <= '9'; i++)
    CharTypeLookup[i] = DIGIT;
  for (i = 'A'; i <= 'Z'; i++)
    CharTypeLookup[i] = LETTER;
  for (i = 'a'; i <= 'z'; i++)
    CharTypeLookup[i] = LETTER;

  CharTypeLookup[10] = 0;
  CharTypeLookup[13] = 0;
  CharTypeLookup[' '] = 0;
  CharTypeLookup['_'] = LETTER;
  CharTypeLookup['.'] = LETTER;
}

bool64
IsCharType(u64 C, u64 Type)
{
  return CharTypeLookup[C] == Type;
}
