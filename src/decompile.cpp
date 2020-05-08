#include "decompile.hpp"
#include "libfuncs.hpp"
#include "log.hpp"
#include "types.hpp"
#include "util.hpp"
#include "v1vc_token.hpp"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STRING_HEAP_SIZE (1024 * 1024 * 10)
u8 *StringHeap = 0;
u8 *StringHeapTail = 0;  // NOTE(aen): byte index of tail
s32 StringHeapCount = 0; // NOTE(aen): # strings in the heap
u64 StringHeapSize = 0;
u8 *LastString = 0;
u64 LastStringLength = 0;

void
InitStringHeap()
{
  if (IsStringHeapReady)
    return;
  StringHeapSize = STRING_HEAP_SIZE;
  StringHeap = new u8[StringHeapSize];
  StringHeapTail = StringHeap;
  IsStringHeapReady = true;
}

void
FreeStringHeap()
{
  delete[] StringHeap;
}

u8 *
NewString(char *S, u64 L)
{
  if (!IsStringHeapReady)
    Fail("Error: Must call InitStringHeap before using NewString\n");
  if (!L)
    L = strlen(S);
  u64 NewSize = (StringHeapTail + L) - StringHeap;
  if (NewSize > STRING_HEAP_SIZE)
    Fail(
        "Error: String heap exhausted. %s strings, %d bytes, no room for "
        "%.*s[%d]\n",
        StringHeapCount,
        StringHeapTail - StringHeap,
        L,
        S,
        L);

  u8 *Home = StringHeapTail;
  memcpy(Home, S, L);
  Home[L] = 0; // NOTE(aen): Important. Confusing print output otherwise! 😅
  StringHeapTail += L + 1;
  StringHeapCount++;

  LastString = Home;
  LastStringLength = L;
  // Log("LastString %.*s\n", LastStringLength, LastString);

  // Log("<<<NewString %.*s[%d] #%d>>>\n", L, S, L, StringHeapCount);
  return Home;
}

Token_t *
Decomp_t::AddTokenD(u32 Value)
{
  // Log("AddTokenD %d\n", Value);
  sprintf_s(TempBuffer, TEMP_BUFFER_SIZE, "%d", Value);
  return AddToken(TempBuffer, TT_NUMBER);
}

#define VC_END 255

// Single-byte opcode descriptors

#define VC_EXEC 1
#define VC_VAR0_ASSIGN 2
#define VC_VAR1_ASSIGN 3
#define VC_VAR2_ASSIGN 4
#define VC_GENERAL_IF 5
#define VC_ELSE 6
#define VC_GOTO 7
#define VC_FOR_LOOP0 8
#define VC_FOR_LOOP1 9
#define VC_SWITCH 10
#define VC_CASE 11

// Single-byte operand descriptors

#define VC_OP_IMMEDIATE 1
#define VC_OP_VAR0 2
#define VC_OP_VAR1 3
#define VC_OP_VAR2 4
#define VC_OP_GROUP 5

// Single-byte IF handler parameters

#define VC_ZERO 0
#define VC_NONZERO 1
#define VC_EQUALTO 2
#define VC_NOTEQUAL 3
#define VC_GREATERTHAN 4
#define VC_GREATERTHANOREQUAL 5
#define VC_LESSTHAN 6
#define VC_LESSTHANOREQUAL 7

// Single byte assignment descriptors

#define VC_SET 1
#define VC_INCREMENT 2
#define VC_DECREMENT 3
#define VC_INCSET 4
#define VC_DECSET 5

// Operand combination descriptors
#define VC_ADD 1
#define VC_SUB 2
#define VC_MULT 3
#define VC_DIV 4
#define VC_MOD 5

// Token types

#define VC_TT_IDENT 1
#define VC_TT_DIGIT 2
#define VC_TT_CONTROL 3
#define VC_TT_RESERVED 4
#define VC_TT_FUNCTION 5
#define VC_TT_VAR0 5
#define VC_TT_VAR1 6
#define VC_TT_VAR2 7

struct Var_t
{
  char *Name = 0;
  bool CanWrite = 0;
};

// NOTE(aen): Return is never actually reached as a lib function because it
// emits the 255 end marker. It's kept in here so that the correct numbering is
// maintained.
const char *VCLibFuncs[] = {"MapSwitch:snnn",
                            "Warp:nnn",
                            "AddCharacter:n",
                            "SoundEffect:n",
                            "GiveItem:n",
                            "Text:nsss",
                            "AlterFTile:nnnn",
                            "AlterBTile:nnnn",
                            "FakeBattle:",
                            "Return:!",
                            "PlayMusic:s",
                            "StopMusic:",
                            "HealAll:",
                            "AlterParallax:nnn",
                            "FadeIn:n",
                            "FadeOut:n",
                            "RemoveCharacter:n",
                            "Banner:sn",
                            "EnforceAnimation:",
                            "WaitKeyUp:",
                            "DestroyItem:nn",
                            "Prompt:nsssnss",
                            "ChainEvent:v0n",
                            "CallEvent:v0n",
                            "Heal:nn",
                            "EarthQuake:nnn",
                            "SaveMenu:",
                            "EnableSave:",
                            "DisableSave:",
                            "ReviveChar:n",
                            "RestoreMp:nn",
                            "Redraw:",
                            "SText:nsss",
                            "DisableMenu:",
                            "EnableMenu:",
                            "Wait:n",
                            "SetFace:nn",
                            "MapPaletteGradient:nnnn",
                            "BoxFadeout:n",
                            "BoxFadeIn:n",
                            "GiveGp:n",
                            "TakeGp:n",
                            "ChangeZone:nnn",
                            "GetItem:nn",
                            "ForceEquip:nn",
                            "GiveXp:nn",
                            "Shop:v1n",
                            "PaletteMorph:nnnnn",
                            "ChangeChr:ns",
                            "ReadControls:",
                            "VcPutPcx:snn",
                            "HookTimer:n",
                            "HookRetrace:n",
                            "VcLoadPcx:sn",
                            "VcBlitImage:nnnnn",
                            "PlayFli:s",
                            "VcClear:",
                            "VcClearRegion:nnnn",
                            "VcText:nns",
                            "VcTBlitImage:nnnnn",
                            "Exit:",
                            "Quit:s",
                            "VcCenterText:ns",
                            "ResetTimer:",
                            "VcBlitTile:nnn",
                            "Sys_ClearScreen:",
                            "Sys_DisplayPcx:s",
                            "OldStartupMenu:",
                            "VgaDump:",
                            "NewGame:s",
                            "LoadMenu:",
                            "Delay:n",
                            "PartyMove:s",
                            "EntityMove:ns",
                            "AutoOn:",
                            "AutoOff:",
                            "EntityMoveScript:nn",
                            "VcTextNum:nnn",
                            "VcLoadRaw:snnn",
                            "VcBox:nnnn",
                            "VcCharName:nnnn",
                            "VcItemName:nnnn",
                            "VcItemDesc:nnnn",
                            "VcItemImage:nnnn",
                            "VcATextNum:nnnn",
                            "VcSpc:nnnn",
                            "CallEffect:v0n",
                            "CallScript:v0n",
                            "VcLine:nnnnn",
                            "GetMagic:nn",
                            "BindKey:nn",
                            "TextMenu:nnnnv1s",
                            "ItemMenu:n",
                            "EquipMenu:n",
                            "MagicMenu:n",
                            "StatusScreen:n",
                            "VcCr2:nnnn",
                            "VcSpellName:nnnn",
                            "VCSpellDesc:nnnn",
                            "VcSpellImage:nnnn",
                            "MagicShop:v1n",
                            "VcTextBox:nnnv1s",
                            "PlayVas:snnnnn"};

const Var_t VCVars0[] = {{"A", 1},
                         {"B", 1},
                         {"C", 1},
                         {"D", 1},
                         {"E", 1},
                         {"F", 1},
                         {"G", 1},
                         {"H", 1},
                         {"I", 1},
                         {"J", 1},
                         {"K", 1},
                         {"L", 1},
                         {"M", 1},
                         {"N", 1},
                         {"O", 1},
                         {"P", 1},
                         {"Q", 1},
                         {"R", 1},
                         {"S", 1},
                         {"T", 1},
                         {"U", 1},
                         {"V", 1},
                         {"W", 1},
                         {"X", 1},
                         {"Y", 1},
                         {"Z", 1},
                         {"NUMCHARS", 0},
                         {"GP", 1},
                         {"LOCX", 0},
                         {"LOCY", 0},
                         {"TIMER", 1},
                         {"DRAWPARTY", 1},
                         {"CAMERATRACKING", 1},
                         {"XWIN", 1},
                         {"YWIN", 1},
                         {"B1", 1},
                         {"B2", 1},
                         {"B3", 1},
                         {"B4", 1},
                         {"UP", 1},
                         {"DOWN", 1},
                         {"LEFT", 1},
                         {"RIGHT", 1},
                         {"TIMERANIMATE", 1},
                         {"FADE", 1},
                         {"LAYER0", 1},
                         {"LAYER1", 1},
                         {"LAYERVC", 1},
                         {"QUAKEX", 1},
                         {"QUAKEY", 1},
                         {"QUAKE", 1},
                         {"SCREENGRADIENT", 1},
                         {"PMULTX", 1},
                         {"PMULTY", 1},
                         {"PDIVX", 1},
                         {"PDIVY", 1},
                         {"VOLUME", 1},
                         {"PARALLAXC", 1},
                         {"CANCELFADE", 1},
                         {"DRAWENTITIES", 1},
                         {"CURZONE", 0},
                         {"TILEB", 0},
                         {"TILEF", 0},
                         {"FOREGROUNDLOCK", 1},
                         {"XWIN1", 1},
                         {"YWIN1", 1},
                         {"LAYER1TRANS", 1},
                         {"LAYERVCTRANS", 1},
                         {"FONTCOLOR", 1},
                         {"KEEPAZ", 1},
                         {"LAYERVC2", 1},
                         {"LAYERVC2TRANS", 1},
                         {"VCLAYERWRITE", 1},
                         {"MODPOSITION", 1}};

const Var_t VCVars1[] = {
    {"FLAGS", 1},
    {"FACING", 0},
    {"CHAR", 0},
    {"ITEM", 0},
    {"VAR", 1},
    {"PARTYINDEX", 0},
    {"XP", 0},
    {"CURHP", 1},
    {"MAXHP", 1},
    {"CURMP", 1},
    {"MAXMP", 1},
    {"KEY", 1},
    {"VCDATABUF", 1},
    {"SPECIALFRAME", 1},
    {"FACE", 1},
    {"SPEED", 1},
    {"ENTITY.MOVING", 1},
    {"ENTITY.CHRINDEX", 1},
    {"MOVECODE", 1},
    {"ACTIVEMODE", 1},
    {"OBSMODE", 1},
    {"ENTITY.STEP", 1},
    {"ENTITY.DELAY", 1},
    {"ENTITY.LOCX", 1},
    {"ENTITY.LOCY", 1},
    {"ENTITY.X1", 1},
    {"ENTITY.X2", 1},
    {"ENTITY.Y1", 1},
    {"ENTITY.Y2", 1},
    {"ENTITY.FACE", 1},
    {"ENTITY.CHASING", 1},
    {"ENTITY.CHASEDIST", 1},
    {"ENTITY.CHASESPEED", 1},
    {"ENTITY.SCRIPTOFS", 0},
    {"ATK", 0},
    {"DEF", 0},
    {"HIT", 0},
    {"DOD", 0},
    {"MAG", 0},
    {"MGR", 0},
    {"REA", 0},
    {"MBL", 0},
    {"FER", 0},
    {"ITEM.USE", 0},
    {"ITEM.EFFECT", 0},
    {"ITEM.TYPE", 0},
    {"ITEM.EQUIPTYPE", 0},
    {"ITEM.EQUIPINDEX", 0},
    {"ITEM.PRICE", 0},
    {"SPELL.USE", 0},
    {"SPELL.EFFECT", 0},
    {"SPELL.TYPE", 0},
    {"SPELL.PRICE", 0},
    {"SPELL.COST", 0},
    {"LVL", 0},
    // NOTE(aen): made NXT, CHARSTATUS, SPELL writable for V1+ Chernobyl compile
    {"NXT", 1},
    {"CHARSTATUS", 1},
    {"SPELL", 1}};

const Var_t VCVars2[] = {{"RANDOM", 0},
                         {"SCREEN", 1},
                         {"ITEMS", 1},
                         {"CANEQUIP", 0},
                         {"CHOOSECHAR", 0},
                         {"OBS", 1},
                         {"SPELLS", 0}};

int VCLibFuncsLength = sizeof(VCLibFuncs) / sizeof(*VCLibFuncs);
int VCVars0Length = sizeof(VCVars0) / sizeof(*VCVars0);
int VCVars1Length = sizeof(VCVars1) / sizeof(*VCVars1);
int VCVars2Length = sizeof(VCVars2) / sizeof(*VCVars2);

void
Decomp_t::ParseString()
{
  u8 *Start = C;
  while (*C++) {}
  sprintf_s(TempBuffer, TEMP_BUFFER_SIZE, "\"%s\"", Start);
  AddToken(TempBuffer, TT_STRING);
}

void
Decomp_t::ParseVar0()
{
  ExpectOpCode(VC_OP_VAR0);
  AddIdent(VCVars0[*C++].Name);
}

void
Decomp_t::ParseVar1()
{
  ExpectOpCode(VC_OP_VAR1);
  AddIdent(VCVars1[*C++].Name);
  AddSymbol("(");
  ParseOperand();
  AddSymbol(")");
}

void
Decomp_t::ParseVar2()
{
  ExpectOpCode(VC_OP_VAR2);
  AddToken(VCVars2[*C++].Name, TT_IDENT);
  AddToken("(", TT_SYMBOL), ParseOperand();
  AddToken(",", TT_SYMBOL), ParseOperand();
  AddToken(")", TT_SYMBOL);
}

void
Decomp_t::ParseVarAssignment()
{
  int AssignType = *C++;
  switch (AssignType)
  {
    case VC_SET: AddToken("=", TT_SYMBOL), ParseOperand(); break;
    case VC_INCREMENT: AddToken("++", TT_SYMBOL); break;
    case VC_DECREMENT: AddToken("--", TT_SYMBOL); break;
    case VC_INCSET: AddToken("+=", TT_SYMBOL), ParseOperand(); break;
    case VC_DECSET: AddToken("-=", TT_SYMBOL), ParseOperand(); break;
  }
  AddToken(";", TT_SYMBOL);
}

void
Decomp_t::ParseOperandPrimitive()
{
  switch (*C)
  {
    case VC_OP_IMMEDIATE: *C++, AddTokenD(GetD()); break;
    case VC_OP_VAR0: ParseVar0(); break;
    case VC_OP_VAR1: ParseVar1(); break;
    case VC_OP_VAR2: ParseVar2(); break;
    default: Fail("ParseOperandArg: Unknown op %d\n", *C);
  }
}

void
Decomp_t::ParseOperand()
{
  while (C < CurrentScriptEnd && *C != VC_END)
  {
    if (*C == VC_OP_GROUP)
    {
      *C++, AddSymbol("("), ParseOperand(), AddSymbol(")");
    }
    else
    {
      ParseOperandPrimitive();
    }

    switch (*C)
    {
      case VC_ADD: C++, AddSymbol("+"); break;
      case VC_SUB: C++, AddSymbol("-"); break;
      case VC_DIV: C++, AddSymbol("/"); break;
      case VC_MULT: C++, AddSymbol("*"); break;
      case VC_MOD: C++, AddSymbol("%"); break;
      case VC_END: *C++; return;
      default: Fail("ParseOperand: Unknown op %d\n", *C);
    }
  }
}

char *
Decomp_t::ParseParam(u64 T1, char *P)
{
  switch (T1)
  {
    case 's': ParseString(); break;
    case 'n': ParseOperand(); break;
    case 'v':
    {
      u64 NumArgs = *C++, Base = *P++, T2 = *P++;
      ParseParam(T2, 0);
      if (Base == '1')
        NumArgs--; // 🎩 The rent is too damn high!
      while (NumArgs--)
        AddSymbol(","), ParseParam(T2, NULL);
      break;
    }
    default: Fail("ParseLibFunc: Unknown param type '%c'\n", T1);
  }
  return P;
}

u64
Decomp_t::ExpectOpCode(u64 Code)
{
  u64 Result = *C++;
  if (Result != Code)
    Fail("Expected op-code %d but got %d\n", Code, Result);
  return Result;
}

void
Decomp_t::ParseLibFunc()
{
  ExpectOpCode(VC_EXEC);

  u8 FunctionIndex = *C++;
  if (FunctionIndex < 1 || FunctionIndex > VCLibFuncsLength)
    Fail(
        "Unknown function index: %d (max %d)\n",
        FunctionIndex,
        VCLibFuncsLength);

  char *Signature = (char *)VCLibFuncs[FunctionIndex - 1];
  char *Name = Signature;
  u64 NameLength = 0;
  char *P = Name;
  while (*P != ':')
    P++;
  NameLength = P - Name;
  P++; // :

  AddIdent((char *)Name, NameLength);
  AddSymbol("(");
  while (*P)
  {
    char Type = *P++;
    P = ParseParam(Type, P);
    *P &&AddSymbol(",");
  }
  AddSymbol(")");
  AddSymbol(";");
}

void
Decomp_t::Debug()
{
  Log("@%d:0x%x %c[%d]\n", C - ScriptBase, C - ScriptBase, *C, *C);
}

void
Decomp_t::ParseIfTerm()
{
  // Log("ParseIfTerm\n");
  // NOTE(aen): Negation is a tail marker in V1 VC, so we need to
  // retroactively patch up the placeholder token we emitted, similar to
  // varargs.
  Token_t *ZeroToken = AddToken(" ", TT_SYMBOL);
  ParseOperand();
  u64 T = *C++;
  if (T == VC_ZERO)
    *ZeroToken->Text = '!';
  else if (T == VC_NONZERO)
    ZeroToken->Length = 0;
  else
  {
    ZeroToken->Length = 0;
    switch (T)
    {
      case VC_EQUALTO: AddToken("==", TT_SYMBOL); break;
      case VC_NOTEQUAL: AddToken("!=", TT_SYMBOL); break;
      case VC_GREATERTHAN: AddToken(">", TT_SYMBOL); break;
      case VC_GREATERTHANOREQUAL: AddToken(">=", TT_SYMBOL); break;
      case VC_LESSTHAN: AddToken("<", TT_SYMBOL); break;
      case VC_LESSTHANOREQUAL: AddToken("<=", TT_SYMBOL); break;
      default: Fail("ParseIf: Unknown IF relational operator %d\n", T);
    }
    ParseOperand(); // NOTE(aen): V1 VC doesn't allow negating term RHS
  }
}

void
Decomp_t::ParseIf()
{
  u64 IfStart = C - ScriptBase;
  // Log("ParseIf, Start %d\n", IfStart);
  ExpectOpCode(VC_GENERAL_IF);
  Token_t *Head = AddToken("if", TT_IDENT);
  AddToken("(", TT_SYMBOL);
  u64 NumArgs = *C++;
  u64 ReturnIndex = GetD();

  for (u64 N = 0; N < NumArgs; N++)
  {
    ParseIfTerm();
    (N < NumArgs - 1) && AddToken("&&", TT_SYMBOL);
  }

  AddToken(")", TT_SYMBOL);
  AddToken("{", TT_SYMBOL);

  u8 *IfEnd = ScriptBase + ReturnIndex;
  while (C < IfEnd) // && ParseExpression())
  {
    // NOTE(aen): WHILE detection. If GOTO is last expression in block and it
    // points to the start, it's a WHILE. It could be a manually written IF/GOTO
    // loop, but converting to WHILE is behavior-preserving.
    u64 GotoAddress = *(u32 *)(C + 1);
    // if (*C == VC_GOTO)
    //   Log("GOTO C+5>=IfEnd? %d, GotoAddress==IfStart?\n",
    //       C + 5 >= IfEnd,
    //       GotoAddress == IfStart);
    if (*C == VC_GOTO && C + 5 >= IfEnd && GotoAddress == IfStart)
    {
      Head->Text = "while", Head->Length = 5, *C++, GetD();
      break;
    }

    NewParseExpression();
    if (*C == VC_END)
    {
      if (C < IfEnd)
      {
        AddIdent("return");
        AddSymbol(";");
      }
      C++;
      // else
      // {
      //   Log("Real VC_END within ParseIf\n");
      // }
    }
  }

  // ExpectOpCode(VC_END);
  AddToken("}", TT_SYMBOL);
}

// NOTE(aen): In expressions
void
Decomp_t::ParseReturn()
{
  // if (C + 1 < CurrentScriptEnd) {
  //   AddIdent("return");
  //   AddSymbol(";");
  // }
  // C++;
}

void
Decomp_t::ParseGoto()
{
  ExpectOpCode(VC_GOTO);
  GetD();
  AddIdent("goto"), AddIdent("???"), AddSymbol(";");
}

void
Decomp_t::ParseFor0()
{
  ExpectOpCode(VC_FOR_LOOP0);
  AddIdent("for");
  AddSymbol("(");
  AddIdent(VCVars0[*C++].Name);
  AddSymbol(",");
  ParseOperand();
  AddSymbol(",");
  ParseOperand();
  AddSymbol(",");
  if (!*C++)
    AddSymbol("-");
  ParseOperand();
  AddSymbol(")");
  AddSymbol("{");
  // Log("A\n");
  // while (C < CurrentScriptEnd && ParseExpression()) {}

  ParseBlock(CurrentScriptEnd, false); // NOTE(aen): Assume no top-level returns
  ExpectOpCode(VC_END);
  // Log("ParseFor0 END\n");

  // ParseBlock(CurrentScriptEnd);

  // Log("B\n");
  // ExpectOpCode(VC_END);
  AddSymbol("}");
  // Fail("No\n");
}

void
Decomp_t::ParseFor1()
{
  ExpectOpCode(VC_FOR_LOOP1);
  AddIdent("for");
  AddSymbol("(");
  AddIdent(VCVars1[*C++].Name);
  AddSymbol("(");
  ParseOperand();
  AddSymbol(")");
  AddSymbol(",");
  ParseOperand();
  AddSymbol(",");
  ParseOperand();
  AddSymbol(",");
  if (!*C++)
    AddSymbol("-");
  ParseOperand();
  AddSymbol(")");
  AddSymbol("{");

  ParseBlock(CurrentScriptEnd, false); // NOTE(aen): Assume no top-level RETURNs
  // while (C < CurrentScriptEnd && ParseExpression()) {}
  ExpectOpCode(VC_END);

  AddSymbol("}");
  // Fail("No\n");
}

void
Decomp_t::ParseSwitch()
{
  ExpectOpCode(VC_SWITCH);
  AddIdent("switch");
  AddSymbol("(");
  ParseOperand();
  AddSymbol(")");
  AddSymbol("{");
  // TODO(aen): Check for VC_END instead?
  while (*C != VC_END)
  {
    // Log("Case %d\n", C - ScriptBase);
    ExpectOpCode(VC_CASE);
    AddIdent("case");
    ParseOperand();
    AddSymbol(":");
    u8 *End = ScriptBase + GetD();
    // Log("Case End: %d\n", End - ScriptBase);
    // NOTE(aen): ParseExpression similar to ExecuteBlock
    // while (C < End && ParseExpression()) {}
    ParseBlock(End);
    // ParseExpression();
    // ExpectOpCode(VC_END);
  }
  ExpectOpCode(VC_END);
  AddSymbol("}");
}

void
Decomp_t::NewParseExpression()
{
  // Log("ParseExpression %d\n", C - ScriptBase);
  switch (*C)
  {
    case VC_EXEC: ParseLibFunc(); break;
    case VC_VAR0_ASSIGN: ParseVar0(), ParseVarAssignment(); break;
    case VC_VAR1_ASSIGN: ParseVar1(), ParseVarAssignment(); break;
    case VC_VAR2_ASSIGN: ParseVar2(), ParseVarAssignment(); break;
    case VC_GENERAL_IF: ParseIf(); break;
    case VC_FOR_LOOP0: ParseFor0(); break;
    case VC_FOR_LOOP1: ParseFor1(); break;
    case VC_GOTO: ParseGoto(); break;
    case VC_SWITCH: ParseSwitch(); break;
    case VC_END:
      // NOTE(aen): Never handle VC_END ourselves. It may or not be a RETURN
      // based on local context.
      break;
    default: Fail("Illegal opcode %d\n", *C);
  }
}

// NOTE(aen): End is an escape hatch for WHILE detection.
bool64
Decomp_t::ParseExpression()
{
  // Log("ParseExpression %d\n", C - ScriptBase);
  switch (*C)
  {
    case VC_EXEC: ParseLibFunc(); return 1;
    case VC_VAR0_ASSIGN: ParseVar0(), ParseVarAssignment(); return 1;
    case VC_VAR1_ASSIGN: ParseVar1(), ParseVarAssignment(); return 1;
    case VC_VAR2_ASSIGN: ParseVar2(), ParseVarAssignment(); return 1;
    case VC_GENERAL_IF: ParseIf(); return 1;
    case VC_FOR_LOOP0: ParseFor0(); return 1;
    case VC_FOR_LOOP1: ParseFor1(); return 1;
    case VC_GOTO: ParseGoto(); return 1;
    case VC_SWITCH: ParseSwitch(); return 1;
    case VC_END:
      // NOTE(aen): Never handle VC_END ourselves. It may or not be a RETURN
      // based on local context.

      // ParseReturn();
      C++;
      if (C < CurrentScriptEnd)
      {
        AddIdent("return");
        AddSymbol(";");
      }
      return 0;
  }
  Fail("Illegal opcode %d\n", *C);
  return 0;
}

// void
// Decomp_t::Parse()
// {
//   Log("Parse %d\n", C - ScriptBase);
//   switch (*C)
//   {
//     case VC_EXEC: ParseLibFunc(); break;
//     case VC_VAR0_ASSIGN: ParseVar0(), ParseVarAssignment(); break;
//     case VC_VAR1_ASSIGN: ParseVar1(), ParseVarAssignment(); break;
//     case VC_VAR2_ASSIGN: ParseVar2(), ParseVarAssignment(); break;
//     case VC_GENERAL_IF: ParseIf(); break;
//     case VC_GOTO: ParseGoto(); break;
//     case VC_FOR_LOOP0: ParseFor0(); break;
//     case VC_FOR_LOOP1: ParseFor1(); break;
//     case VC_SWITCH: ParseSwitch(); break;
//     case VC_END:
//       if (C + 2 < CurrentScriptEnd)
//       {
//         AddIdent("return");
//         AddSymbol(";");
//       }
//       // C++;
//       // ParseReturn(); // TODO(aen): Why must this be after C++?
//       break;
//     default: Fail("Unexpected op-code: %d\n", *C);
//   }
// }

Token_t *
Decomp_t::AddToken(char *Text, u64 L, TokenType_e Type)
{
  return TokenList.AddToken((char *)NewString(Text, L), L, Type, 0, 0);
}

Token_t *
Decomp_t::AddToken(char *Text, TokenType_e Type)
{
  return AddToken(Text, strlen(Text), Type);
}

Token_t *
Decomp_t::AddIdent(char *S, u64 L)
{
  return AddToken(S, L, TT_IDENT);
}
Token_t *
Decomp_t::AddIdent(char *Text)
{
  return AddIdent(Text, strlen(Text));
}
Token_t *
Decomp_t::AddSymbol(char *Text)
{
  return AddToken(Text, TT_SYMBOL);
}

// TODO(aen): Maybe split this into multiple funcs. The EmitReturns stuff is
// primarily for FOR loops, where we assume that a top-level RETURN will never
// be present. FOR loops do not have explicitly known ends and terminate with
// the same END op-code as everything else, which means top-level RETURNs create
// ambigous decompilations. It seems safe to assume most folks wouldn't do this.
// I can only imagine a top-level RETURN being used when doing label+GOTO
// shenanigans inside a FOR. I've seen label+GOTO in WHILE (e.g. Chernobyl) but
// no top-level RETURNs yet.
void
Decomp_t::ParseBlock(u8 *End, bool64 EmitReturns)
{
  // Log("ParseBlock\n");
  while (C < End)
  {
    NewParseExpression();
    if (*C == VC_END)
    {
      if (EmitReturns)
        C++;
      else
        break;
      // NOTE(aen): If we're not at the known end of this block, it's a return.
      if (EmitReturns && C < End)
      {
        AddIdent("return");
        AddSymbol(";");
      }
    }
  }
}

void
Decomp_t::ParseEvent(u64 Index)
{
  // Log("ParseEvent %d\n", Index);
  CurrentScriptBase = ScriptBase + ScriptOffsetTable[Index];
  CurrentScriptEnd = ScriptBase + ScriptOffsetTable[Index + 1];
  CurrentScriptLength = ScriptOffsetTable[Index + 1] - ScriptOffsetTable[Index];
  C = CurrentScriptBase;
  AddIdent("event");
  AddSymbol("{");
  // Log("CurrentScriptEnd %d\n", CurrentScriptEnd - CurrentScriptBase);
  ParseBlock(CurrentScriptEnd);
  AddSymbol("}");
  // ExpectOpCode(VC_END);
}

void
DebugLengths()
{
  Log("%d funcs, %d vars0, %d vars1, %d vars2\n",
      VCLibFuncsLength,
      VCVars0Length,
      VCVars1Length,
      VCVars2Length);
}

u32
Decomp_t::PeekD()
{
  return *(u32 *)C;
}
u16
Decomp_t::PeekW()
{
  return *(u16 *)C;
}
u32
Decomp_t::GetD()
{
  return C += 4, *(u32 *)(C - 4);
}
u16
Decomp_t::GetW()
{
  return C += 2, *(u16 *)(C - 4);
}

void
Decomp_t::Init(Buffer_t *Buffer)
{
  // Log("Decompile.Init\n");

  Data = Buffer->Data;
  DataSize = Buffer->Length;
  Data[DataSize] = 0;
  DataEnd = Data + DataSize;
  C = Data;

  NumScripts = GetD();
  // Log("NumScripts %d\n", NumScripts);
  if (NumScripts < 0 || NumScripts > 100)
    Fail("Scripts negative or >100: %d\n", NumScripts);
  ScriptOffsetTable = new u32[NumScripts + 1];
  memcpy(ScriptOffsetTable, C, NumScripts * 4);
  C += NumScripts * 4;

  ScriptBase = C;
  // NOTE(aen): So we can always check +1 to get length of current script
  ScriptOffsetTable[NumScripts] = (u32)(DataSize - (ScriptBase - Data));
  // Log("Load: Final offset %d\n", ScriptOffsetTable[NumScripts]);
  // Log("Load: Scripts end %d, Total end %d\n", C - ScriptBase, DataSize);
}

void
Decompile(Buffer_t *Input, Buffer_t *Output)
{
  ASSERT(Input);
  ASSERT(Output);

  if (!Input->Length)
  {
    Log("Empty input buffer, nothing to decompile.\n");
    return;
  }

  // Log("Decompile: Input %d\n", Input->Length);

  InitBufferSlab();
  InitStringHeap();
  InitTokenSlab();

  Decomp_t D;
  D.Init(Input);

  for (u64 N = 0; N < D.NumScripts; N++)
    D.ParseEvent(N);
  // Log("Decompile: Finished parsing events, debugging token list...\n");
  // D.TokenList.Debug();
  u64 MinifiedLength = D.TokenList.Minify(Output->Data);
  // Log("MinifiedLength %d\n", MinifiedLength);
  // Log("Decompiled Output:%s\n", Output->Data);
  Output->Length = MinifiedLength;
}

void
Decompile(const char *Filename, Buffer_t *Output)
{
  Decompile(Load(Filename), Output);
}

int
decompile_cmdline(int argc, char *argv[])
{
  // char *Filename = 0;
  Log("argc %d\n", argc);
  switch (argc)
  {
    case 1: Fail("usage: vccnow d <filename.map|compiled>\n"); break;
    case 2: break;
    default: Fail("vccnow d: Too many parameters.\n");
  }
  // Log("Decompiling %s...\n", argv[1]);
  Buffer_t Output;
  Output.Data = (u8 *)TempBuffer;
  Decompile(argv[1], &Output);
  Log("%s", TempBuffer);
  return 0;
}