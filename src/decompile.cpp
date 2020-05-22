#include "decompile.hpp"
#include "lib_funcs.hpp"
#include "log.hpp"
#include "mem.hpp"
#include "types.hpp"
#include "util.hpp"
#include "v1vc_token.hpp"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int Nest = 0;
int OpNest = 0;

// NOTE(aen): Important to use this when adding tokens with dynamic contents
// because their Text will point directly to the string passed in. For dynamic
// bits like these markers, we need to allocate new memory so it remains unique
// per instance. Otherwise, the minified output will reuse whatever memory
// was pointed to for the last instance, creating output such as:
//
// event/*2*/{}event/*2*/{}event/*2*/{}
//
char *
NewString(char *OriginalString)
{
  u64 L = strlen(OriginalString);
  char *S = NewList(&TokenPool, L + 1, char);
  memcpy(S, OriginalString, L);
  S[L] = 0;
  return S;
}

void
decompiler::DisSaveAddress(char Marker)
{
  PrevDisMarker = DisMarker;
  DisMarker = Marker;
  DisRefCheck(); // NOTE(aen): May override marker
  u64 Address0 = C - ScriptBase;
  if (Address0 >= 0xffffffff)
    Fail("Address is larger than 32-bit: %d %x\n", Address0, Address0);
  DisAddress = (u32)Address0;
}

void
decompiler::DisLogComments(const char *Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  vsnprintf(DisComments, TEMP_BUFFER_SIZE, Format, Args);
  va_end(Args);
}

void
decompiler::DisFlush()
{
  u8 *A = (u8 *)&DisAddress;
  Log("%c%c%02x %02x %02x %02x %20s%s\n",
      PrevDisMarker,
      DisMarker,
      A[0],
      A[1],
      A[2],
      A[3],
      DisByteCodes,
      DisComments);

  PrevDisMarker = ' ';
  DisMarker = ' ';
  DisAddress = 0xffffffff;
  memset(DisByteCodes, 0, TEMP_BUFFER_SIZE);
  memset(DisComments, 0, TEMP_BUFFER_SIZE);
  DisByteCodesP = DisByteCodes;
}

char TempNumber[TEMP_BUFFER_SIZE];
token *
decompiler::AddTokenD(u32 Value)
{
  // Log("AddTokenD %d\n", Value);
  sprintf_s(TempNumber, TEMP_BUFFER_SIZE, "%d", Value);

  return AddToken(NewString(TempNumber), TT_NUMBER);
}

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

struct Var_t
{
  char *Name = 0;
  bool CanWrite = 0;
};

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
decompiler::ParseString()
{
  if (Mode == DISASSEMBLE)
    DisSaveAddress();

  // Log("ParseString\n");
  u8 *Start = C;
  while (*C++) {}
  // Log("ParseString: final '%c' %d\n", *C, *C);
  sprintf_s(TempBuffer, TEMP_BUFFER_SIZE, "\"%s\"", Start);
  AddToken(NewString(TempBuffer), TT_STRING);

  if (Mode == DISASSEMBLE)
  {
    DisLogComments("%s", TempBuffer);
    DisFlush();

    DumpHex("", Start, C - Start, C - Start, "              ");
    // for (u8 *P = Start; P != C; P++)
    //   Log("%02x ", *P);
    // Log(".%s\n", TempBuffer);
  }
}

void
decompiler::ParseVar0()
{
  if (Mode == DISASSEMBLE)
    DisSaveAddress();

  ExpectOpCode(VC_OP_VAR0);
  u8 Index = *C++;
  char *Name = VCVars0[Index].Name;

  AddIdent(Name);

  if (Mode == DISASSEMBLE)
  {
    DisC(VC_OP_VAR0);
    DisC(Index);
    DisLogComments(".var0 %s", Name);
    DisFlush();
  }
}

void
decompiler::ParseVar1()
{
  if (Mode == DISASSEMBLE)
    DisSaveAddress();

  ExpectOpCode(VC_OP_VAR1);
  u8 Index = *C++;
  char *Name = VCVars1[Index].Name;

  if (Mode == DISASSEMBLE)
  {
    DisC(VC_OP_VAR1);
    DisC(Index);
    DisLogComments(".var1 %s", Name);
    DisFlush();
  }

  AddIdent(Name);
  AddSymbol("(");
  ParseOperand();
  if (ParseFailure)
    return;
  AddSymbol(")");
}

void
decompiler::ParseVar2()
{
  if (Mode == DISASSEMBLE)
    DisSaveAddress();

  ExpectOpCode(VC_OP_VAR2);
  u8 Index = *C++;
  char *Name = VCVars2[Index].Name;

  if (Mode == DISASSEMBLE)
  {
    DisC(VC_OP_VAR2);
    DisC(Index);
    DisLogComments(".var2 %s", Name);
    DisFlush();
  }

  AddToken(Name, TT_IDENT);
  AddToken("(", TT_SYMBOL);
  ParseOperand();
  if (ParseFailure)
    return;
  AddToken(",", TT_SYMBOL);
  ParseOperand();
  if (ParseFailure)
    return;
  AddToken(")", TT_SYMBOL);
}

void
decompiler::ParseVarAssignment()
{
  if (Mode == DISASSEMBLE)
    DisSaveAddress();

  int AssignType = *C++;

  if (Mode == DISASSEMBLE)
  {
    switch (AssignType)
    {
      case VC_SET: DisC(VC_SET), DisLogComments(".set"); break;
      case VC_INCREMENT:
        DisC(VC_INCREMENT), DisLogComments(".increment");
        break;
      case VC_DECREMENT:
        DisC(VC_DECREMENT), DisLogComments(".decrement");
        break;
      case VC_INCSET: DisC(VC_INCSET), DisLogComments(".incset"); break;
      case VC_DECSET: DisC(VC_DECSET), DisLogComments(".decset"); break;
    }
    DisFlush();
  }

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
decompiler::ParseOperandPrimitive()
{
  // Log("ParseOperandPrimitive: %d\n", *C);
  switch (*C)
  {
    case VC_OP_IMMEDIATE:
    {
      if (Mode == DISASSEMBLE)
        DisSaveAddress();

      // Log("ParseOperandPrimitive: IMMEDIATE\n");
      *C++;
      if (Mode == DISASSEMBLE)
      {
        DisC(VC_OP_IMMEDIATE);
        DisLogComments(".literal");
        DisFlush();
        DisSaveAddress();
      }
      u32 Value = GetD();
      // Log("ParseOperandPrimitive: IMMEDIATE value %d\n", Value);

      if (Mode == DISASSEMBLE)
      {
        DisD(Value);
        DisLogComments("%d", Value);
        DisFlush();
      }

      AddTokenD(Value);
      break;
    }
    case VC_OP_VAR0: ParseVar0(); break;
    case VC_OP_VAR1: ParseVar1(); break;
    case VC_OP_VAR2: ParseVar2(); break;
    default:
    {
      // if (Mode == DISASSEMBLE)
      // {
      ParseFailure = true;
      AddComment("/*parse failure: ParseOperandArg: Unknown op %d*/", *C);

      // }
      // else
      // {
      //   DumpHex("Uknown op", C - 32, C - ScriptBase);
      //   Fail("ParseOperandArg: Unknown op %d\n", *C);
      // }
    }
  }
}

void
decompiler::ParseOperand()
{
  DebugLog(
      MEDIUM,
      "%.*sParseOperand '%c' %d, At End? %d\n",
      OpNest * 4,
      "",
      *C,
      *C,
      C >= CurrentScriptEnd);
  OpNest++;
  while (C < CurrentScriptEnd && *C != VC_END)
  {
    if (Mode == DISASSEMBLE)
      DisSaveAddress();

    if (*C == VC_OP_GROUP)
    {
      // Log("ParseOperand: GROUP\n");
      *C++;

      AddSymbol("(");
      if (Mode == DISASSEMBLE)
      {
        DisC(VC_OP_GROUP);
        DisLogComments(".group");
        DisFlush();
      }

      ParseOperand();
      AddSymbol(")");
    }
    else
    {
      ParseOperandPrimitive();
    }

    if (ParseFailure)
      return;
    if (Mode == DISASSEMBLE)
    {
      if (Mode == DISASSEMBLE)
        DisSaveAddress();
      switch (*C)
      {
        case VC_ADD: DisC(VC_ADD), DisLogComments(".add +"), DisFlush(); break;
        case VC_SUB: DisC(VC_SUB), DisLogComments(".sub -"), DisFlush(); break;
        case VC_DIV: DisC(VC_DIV), DisLogComments(".div /"), DisFlush(); break;
        case VC_MULT:
          DisC(VC_MULT), DisLogComments(".mul *"), DisFlush();
          break;
        case VC_MOD: DisC(VC_MOD), DisLogComments(".mod %"), DisFlush(); break;
      }
    }

    switch (*C)
    {
      case VC_ADD: C++, AddSymbol("+"); break;
      case VC_SUB: C++, AddSymbol("-"); break;
      case VC_DIV: C++, AddSymbol("/"); break;
      case VC_MULT: C++, AddSymbol("*"); break;
      case VC_MOD: C++, AddSymbol("%"); break;
      case VC_END:
      {
        if (Mode == DISASSEMBLE)
        {
          DisC(VC_END);
          DisLogComments(".end (operand)");
          DisFlush();
        }

        DebugLog(MEDIUM, "ParseOperand: END\n");
        *C++;
        OpNest--;

        return;
      }
      default:
      {
        // if (Mode == DISASSEMBLE)
        // {
        ParseFailure = true;
        AddComment("/*parse failure: ParseOperand: Unknown op %d*/", *C);

        // }
        // else
        // {
        //   Fail("ParseOperand: Unknown op %d\n", *C);
        // }
      }
    }
  }
  // Fail("ParseOperand: Fell off end\n");
}

char *
decompiler::ParseParam(u64 T1, char *P)
{
  // Log("ParseParam: %c, %s\n", T1, P);
  switch (T1)
  {
    case 's': ParseString(); break;
    case 'n': ParseOperand(); break;
    case 'v':
    {
      u64 NumArgs = *C++;
      u64 Base = *P++;
      u64 T2 = *P++;
      ParseParam(T2, 0);
      if (ParseFailure)
        return P;
      if (Base == '1')
        NumArgs--; // 🎩 The rent is too damn high!
      while (NumArgs--)
      {
        AddSymbol(",");
        ParseParam(T2, NULL);
        if (ParseFailure)
          return P;
      }
      break;
    }
    default:
    {
      // if (Mode == DISASSEMBLE)
      // {
      ParseFailure = true;
      AddComment(
          "/*parse failure: ParseLibFunc: Unknown param type '%c'*/", T1);
      Log("[ParseFailure] ParseLibFunc: Unknown param type '%c'\n", T1);
      // }
      // else
      //   Fail("ParseLibFunc: Unknown param type '%c'\n", T1);
    }
  }
  return P;
}

u64
decompiler::ExpectOpCode(u64 Code)
{
  u64 Result = *C++;
  if (Result != Code)
    Fail("Expected op-code %d but got %d\n", Code, Result);
  return Result;
}

void
decompiler::ParseLibFunc()
{
  if (Mode == DISASSEMBLE)
    DisSaveAddress();

  ExpectOpCode(VC_EXEC);

  u8 FunctionIndex = *C++;
  if (FunctionIndex < 1 || FunctionIndex > VCLibFuncsLength)
  {
    // if (Mode == DISASSEMBLE)
    // {
    ParseFailure = true;
    AddComment(
        "/*parse failure: Unknown function index: %d (max %d)*/",
        FunctionIndex,
        VCLibFuncsLength);
    Log("[ParseFailure]: Unknown function index: %d (max %d)\n",
        FunctionIndex,
        VCLibFuncsLength);
    return;
    // }
    // else
    // {
    //   Fail(
    //       "Unknown function index: %d (max %d)\n",
    //       FunctionIndex,
    //       VCLibFuncsLength);
    // }
  }

  char *Signature = (char *)VCLibFuncs[FunctionIndex - 1];
  char *Name = Signature;
  u64 NameLength = 0;
  char *P = Name;
  while (*P != ':')
    P++;
  NameLength = P - Name;
  P++; // :

  if (Mode == DISASSEMBLE)
  {
    DisC(VC_EXEC);
    DisC(FunctionIndex);
    DisLogComments(".exec %.*s", NameLength, Name);
    DisFlush();
  }

  AddIdent((char *)Name, NameLength);
  AddSymbol("(");

  while (*P)
  {
    char Type = *P++;
    P = ParseParam(Type, P);
    if (ParseFailure)
      return;
    if (*P)
      AddSymbol(",");
  }
  AddSymbol(")");
  AddSymbol(";");
}

void
decompiler::Debug()
{
  Log("@%d:0x%x %c[%d]\n", C - ScriptBase, C - ScriptBase, *C, *C);
}

void
decompiler::ParseIfTerm()
{
  DebugLog(MEDIUM, "ParseIfTerm\n");
  // NOTE(aen): Negation is a tail marker in V1 VC, so we need to
  // retroactively patch up the placeholder token we emitted, similar to
  // varargs.
  token *ZeroToken = AddToken(NewString(" "), TT_SYMBOL);
  ParseOperand();
  if (C >= CurrentScriptEnd)
  {
    Log("ParseIfTerm: C >= CurrentScriptEnd after parsing operand, "
        "bailing...\n");
    return;
  }
  if (ParseFailure)
    return;

  u8 T = *C++; // control byte (sign)
  if (C >= CurrentScriptEnd)
  {
    Log("ParseIfTerm: C >= CurrentScriptEnd after parsing control byte, "
        "bailing...\n");
    return;
  }

  if (Mode == DISASSEMBLE)
  {
    DisSaveAddress();
    DisC(T);
    if (T == VC_ZERO)
      DisLogComments(".if-sign-zero");
    else
      DisLogComments(".if-sign-nonzero");
    DisFlush();
  }

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
      default:
      {
        Fail("ParseIf: Unknown IF relational operator %d\n", T);
      }
    }
    ParseOperand(); // NOTE(aen): V1 VC doesn't allow negating term RHS
  }
}

void
decompiler::ParseIf()
{
  u64 IfAddress = C - ScriptBase;
  u64 IfNum = NumIfRefs++;
  IfRefs[IfNum] = IfAddress;
  AddressRefs[NumAddressRefs++] = IfAddress;

  if (Mode == DISASSEMBLE)
  {
    DisSaveAddress(':');
    Log("if%d\n", IfNum);
  }

  u64 IfStart = C - ScriptBase;
  DebugLog(MEDIUM, "ParseIf, Start %lld\n", IfStart);

  ExpectOpCode(VC_GENERAL_IF);
  token *Head = AddToken("if", TT_IDENT);
  AddToken("(", TT_SYMBOL);
  u8 NumTerms = *C++;

  if (Mode == DISASSEMBLE)
  {
    DisC(VC_GENERAL_IF);
    DisC(NumTerms);
    DisLogComments(".if [terms=%d]", NumTerms);
    DisFlush();

    DisSaveAddress();
  }

  u32 ReturnIndex = GetD();
  // AddComment("/*ReturnIndex %08x*/", ReturnIndex);
  AddressRefs[NumAddressRefs++] = ReturnIndex;
  DebugLog(
      MEDIUM,
      "ParseIf: NumTerms %lld, ReturnIndex %lld\n",
      NumTerms,
      ReturnIndex);

  if (Mode == DISASSEMBLE)
  {
    DisD(ReturnIndex);
    DisLogComments(".else");
    DisFlush();
  }

  u8 *IfEnd = ScriptBase + ReturnIndex;
  if (C >= IfEnd)
  {
    // if (Mode == DISASSEMBLE)
    // {
    //   ParseFailure = true;
    //   return;
    // }
    // else
    // {
    DumpHex("C past IfEnd", C - 32, CurrentScriptLength);
    Fail("ParseIf: C past IfEnd before beginning, %lld\n", C - ScriptBase);
    // }
  }
  // if (IfEnd >= CurrentScriptEnd)
  // {
  //   DumpHex("IfEnd past CurrentScriptEnd", C - 32, CurrentScriptLength);
  //   Fail(
  //       "ParseIf: IfEnd past CurrentScriptEnd before beginning, IfEnd: %lld,
  //       " "CurrentScriptEnd: %lld, C %lld\n", IfEnd - CurrentScriptBase,
  //       CurrentScriptEnd - CurrentScriptBase,
  //       C - ScriptBase);
  // }

  for (u64 N = 0; N < NumTerms; N++)
  {
    ParseIfTerm();
    if (N < NumTerms - 1)
      AddToken("&&", TT_SYMBOL);
  }

  AddToken(")", TT_SYMBOL);
  AddToken("{", TT_SYMBOL);

  u64 outer = 0;
  while (C < IfEnd)
  {
    DebugLog(HIGH, "ParseIf: Outer loop %lld\n", outer);
    outer++;
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
      // if (Mode == DISASSEMBLE)
      // {
      //   DisSaveAddress();
      //   DisC(VC_GOTO);
      //   DisLogComments(".goto if%d (while)", IfNum);
      //   DisFlush();
      // }

      Head->Text = "while";
      Head->Length = 5;
      // *C++;

      // if (Mode == DISASSEMBLE)
      //   DisSaveAddress();

      // u32 Value = GetD();

      // if (Mode == DISASSEMBLE)
      // {
      //   DisD(Value);
      //   DisFlush();
      // }
      ParseGoto(/*EmitDecompilation*/ false);

      break;
    }

    ParseExpression();
    if (ParseFailure)
    {
      Log("ParseIf: ParseFailure eject after ParseExpression\n");
      return;
    }
    // if (C > IfEnd)
    // {
    //   Fail("ParseIf: A Overshot IfEnd by %d bytes\n", C - IfEnd);
    // }

    if (*C == VC_END)
    {
      if (C < IfEnd)
      {
        AddIdent("return");
        AddSymbol(";");

        if (Mode == DISASSEMBLE)
        {
          DisSaveAddress();
          DisC(VC_END);
          DisLogComments(".end (return)");
          DisFlush();
        }

        C++;
      }
      // else
      // {
      //   Log("Real VC_END within ParseIf\n");
      // }
    }
  }

  // ExpectOpCode(VC_END);
  AddToken("}", TT_SYMBOL);

  if (C > IfEnd)
  {
    // DumpHex("Overshot If", C - 32, CurrentScriptLength);
    // Fail("ParseIf: Overshot IfEnd by %d bytes. '%c' %d\n", C - IfEnd, *C,
    // *C);
  }
  // C = IfEnd; // TODO(aen): Necessary?
}

// NOTE(aen): In expressions
void
decompiler::ParseReturn()
{
  // if (C + 1 < CurrentScriptEnd) {
  //   AddIdent("return");
  //   AddSymbol(";");
  // }
  // C++;
}

void
decompiler::ParseGoto(b64 EmitDecompilation)
{
  if (Mode == DISASSEMBLE)
  {
    DisSaveAddress();
    DisC(VC_GOTO);
    DisLogComments(".goto");
    DisFlush();
  }

  ExpectOpCode(VC_GOTO);

  if (Mode == DISASSEMBLE)
    DisSaveAddress();

  u32 Address = GetD();
  AddressRefs[NumAddressRefs++] = Address;
  u64 ThisGoto = NumGotoRefs++;
  GotoRefs[ThisGoto] = Address;

  if (Mode == DISASSEMBLE)
  {
    DisD(Address);

    b64 IsIf = false;
    u64 IfNum = 0;
    for (u64 N = 0; N < NumIfRefs; N++)
    {
      if (IfRefs[N] == Address)
      {
        IsIf = true;
        IfNum = N;
        break;
      }
    }
    if (IsIf)
      DisLogComments(".if%d", IfNum);
    DisFlush();
  }

  if (EmitDecompilation)
  {
    AddIdent("goto");
    AddIdent("???");
    AddSymbol(";");
  }
}

void
decompiler::ParseFor0()
{
  if (Mode == DISASSEMBLE)
    DisSaveAddress();

  ExpectOpCode(VC_FOR_LOOP0);

  u8 Index = *C++;
  char *Name = VCVars0[Index].Name;

  if (Mode == DISASSEMBLE)
  {
    DisC(VC_FOR_LOOP0);
    DisC(Index);
    DisLogComments(".for0 %s", Name);
    DisFlush();
  }

  AddIdent("for");
  AddSymbol("(");
  AddIdent(Name);
  AddSymbol(",");
  ParseOperand();
  if (ParseFailure)
    return;
  AddSymbol(",");
  ParseOperand();
  if (ParseFailure)
    return;
  AddSymbol(",");

  if (Mode == DISASSEMBLE)
  {
    DisSaveAddress();
    DisC(*C);
    if (*C == VC_ZERO)
      DisLogComments(".for0-sign-zero");
    else
      DisLogComments(".for0-sign-nonzero");
    DisFlush();
  }

  if (!*C++)
    AddSymbol("-");
  ParseOperand();
  if (ParseFailure)
    return;
  AddSymbol(")");
  AddSymbol("{");

  ParseBlock(CurrentScriptEnd, false); // NOTE(aen): Assume no top-level returns

  // TODO(aen): Why does this fixe PHAGE bugdung.map?
  // NOTE(aen): Seems to happen when closing out many blocks at the end of an
  // event, e.g. }}}}. Maybe special check for: Consecutive series of FF that
  // reach CurrentScriptEnd.
  if (*C == VC_END)
  {
    if (Mode == DISASSEMBLE)
    {
      DisSaveAddress();
      DisC(VC_END);
      DisLogComments(".end (block) for0");
      DisFlush();
    }
    C++;
  }
  else
  {
    ParseFailure = true;
    AddComment(
        "/*parse failure: ParseFor0: tail but no END found. Found %d*/", *C);
    Log("[ParseFailure] ParseFor0: tail but no END found. Found %d\n", *C);
    // Fail("\nParseFor0: tail but no END found. Found %d\n", *C);
  }

  // ExpectOpCode(VC_END);
  // Log("ParseFor0 END\n");

  // ParseBlock(CurrentScriptEnd);

  // Log("B\n");
  // ExpectOpCode(VC_END);
  AddSymbol("}");
  // Fail("No\n");
}

void
decompiler::ParseFor1()
{
  if (Mode == DISASSEMBLE)
    DisSaveAddress();

  ExpectOpCode(VC_FOR_LOOP1);

  AddIdent("for");
  AddSymbol("(");

  u8 Index = *C++;
  char *Name = VCVars1[Index].Name;

  if (Mode == DISASSEMBLE)
  {
    DisC(VC_FOR_LOOP1);
    DisC(Index);
    DisLogComments(".for1 %s", Name);
    DisFlush();
  }

  AddIdent(Name);
  AddSymbol("(");
  ParseOperand();
  if (ParseFailure)
    return;
  AddSymbol(")");
  AddSymbol(",");
  ParseOperand();
  if (ParseFailure)
    return;
  AddSymbol(",");
  ParseOperand();
  if (ParseFailure)
    return;
  AddSymbol(",");

  if (Mode == DISASSEMBLE)
  {
    DisSaveAddress();
    DisC(*C);
    if (*C == VC_ZERO)
      DisLogComments(".for1-sign-zero");
    else
      DisLogComments(".for1-sign-nonzero");
    DisFlush();
  }

  if (!*C++)
    AddSymbol("-");

  ParseOperand();
  if (ParseFailure)
    return;
  AddSymbol(")");
  AddSymbol("{");

  ParseBlock(CurrentScriptEnd, false); // NOTE(aen): Assume no top-level RETURNs

  // Log("CurrentScript: Base %lld, End %lld, Current %lld\n",
  //     CurrentScriptBase - ScriptBase,
  //     CurrentScriptEnd - CurrentScriptBase,
  //     C - CurrentScriptBase);

  // TOD(aen): HMMMMMMMMMMMM This fixes PHAGE forruin.map
  if (*C == VC_END)
  {
    if (Mode == DISASSEMBLE)
    {
      DisSaveAddress();
      DisC(VC_END);
      DisLogComments(".end (block) for1");
      DisFlush();
    }
    C++;
  }
  // ExpectOpCode(VC_END);
  // Log("B\n");

  AddSymbol("}");
  // Fail("No\n");
}

void
decompiler::ParseSwitch()
{
  if (Mode == DISASSEMBLE)
    DisSaveAddress();

  ExpectOpCode(VC_SWITCH);

  if (Mode == DISASSEMBLE)
  {
    DisC(VC_SWITCH);
    DisLogComments(".switch");
    DisFlush();
  }

  AddIdent("switch");
  AddSymbol("(");
  ParseOperand();
  if (ParseFailure)
    return;
  AddSymbol(")");
  AddSymbol("{");
  while (*C != VC_END)
  {
    if (Mode == DISASSEMBLE)
    {
      DisSaveAddress();
      DisC(VC_CASE);
      DisLogComments(".case");
      DisFlush();
    }
    // Log("Case %d\n", C - ScriptBase);
    ExpectOpCode(VC_CASE);
    AddIdent("case");
    ParseOperand();
    if (ParseFailure)
      return;
    AddSymbol(":");

    if (Mode == DISASSEMBLE)
      DisSaveAddress();

    u32 CaseEndAddress = GetD();
    AddressRefs[NumAddressRefs++] = CaseEndAddress;

    u8 *End = ScriptBase + CaseEndAddress;
    if (Mode == DISASSEMBLE)
    {
      DisD(CaseEndAddress);
      DisLogComments(".case-end-address");
      DisFlush();
    }

    // Log("Case End: %d\n", End - ScriptBase);
    ParseBlock(End);

    // NOTE(aen): Resetting C here is important. Without, you get:
    //   Expected op-code 11 but got 10
    // When parsing PHAGE *bat.vc. Anything like this:
    //   switch (a) { case 0: if (b) vgadump(); }
    //   switch (c) { case 1: vgadump(); }
    C = End;
  }

  if (Mode == DISASSEMBLE)
  {
    DisSaveAddress();
    DisC(VC_END);
    DisLogComments(".end (switch)");
    DisFlush();
  }

  ExpectOpCode(VC_END);
  AddSymbol("}");
}

void
decompiler::ParseExpression()
{
  DebugLog(MEDIUM, "ParseExpression %d\n", C - ScriptBase);
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
    default:
    {
      // if (Mode == DISASSEMBLE)
      // {
      // Assert(0);
      ParseFailure = true;
      AddComment("/*parse failure: ParseExpression: Illegal opcode %d*/", *C);
      Log("[ParseFailure] ParseExpression: Illegal opcode %d\n", *C);
      // }
      // else
      // {
      //   DebugLog(
      //       LOW,
      //       "Position %d, ScriptLength %d, C %d, CurrentScriptIndex %d\n",
      //       C - ScriptBase,
      //       CurrentScriptLength,
      //       C - CurrentScriptBase,
      //       CurrentScriptIndex);
      //   DumpHex("Illegal opcode", C - 32, C - ScriptBase);
      //   Fail("ParseExpression: Illegal opcode %d\n", *C);
      // }
      break;
    }
  }
}

void
decompiler::DisRefCheck()
{
  u64 Address = C - ScriptBase;
  for (u64 N = 0; N < NumAddressRefs; N++)
  {
    if (Address == AddressRefs[N])
    {
      DisMarker = ':';
      break;
    }
  }
}

token *
decompiler::AddToken(char *Text, u64 L, TokenType_e Type)
{
  // DebugLog("AddToken: L %d\n", L);
  DebugLog(LOW, "%.*s ", L, Text);
  return TokenList.AddToken(Text, L, Type, 0, 0);
}

token *
decompiler::AddToken(char *Text, TokenType_e Type)
{
  u64 L = strlen(Text);
  // Log("AddToken: strlen %s\n", Text);
  return AddToken(Text, L, Type);
}

token *
decompiler::AddIdent(char *S, u64 L)
{
  return AddToken(S, L, TT_IDENT);
}
token *
decompiler::AddIdent(char *Text)
{
  return AddIdent(Text, strlen(Text));
}
token *
decompiler::AddSymbol(char *Text)
{
  return AddToken(Text, TT_SYMBOL);
}

token *
decompiler::AddComment(char *Format, ...)
{
  char Temp[TEMP_BUFFER_SIZE];
  va_list Args;
  va_start(Args, Format);
  vsnprintf(Temp, TEMP_BUFFER_SIZE, Format, Args);
  va_end(Args);

  return AddSymbol(NewString(Temp));
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
decompiler::ParseBlock(u8 *End, b64 EmitReturns)
{
  Nest++;

  u8 *BlockStart = C;
  DebugLog(MEDIUM, "ParseBlock\n");
  while (C < End)
  {
    ParseExpression();
    if (ParseFailure)
      break;
    if (Mode == DISASSEMBLE)
    {
      DisSaveAddress();
    }

    if (*C == VC_END)
    {
      // NOTE(aen): Regardless of whether we're emitting returns or not, if the
      // next byte is the known end of the block, this isn't a return. Not sure
      // how smart this is. There is a lot of fiddly behavior with VC_END op
      // codes. The interpreter in the engine doesn't have to care about the
      // kind of nuances a decompiler does, which is why VC_END handling is so
      // special-cased all over the place.
      if (C + 1 == End)
      {
        // Log("\nTRUE END\n");
        // C++;
        break;
      }

      if (EmitReturns)
        C++;
      else
        break;

      // NOTE(aen): If we're not at the known end of this block, it's a
      // return.
      if (EmitReturns && C < End)
      {
        AddIdent("return");
        AddSymbol(";");

        if (Mode == DISASSEMBLE)
        {
          DisC(VC_END);
          DisLogComments(".end (return)");
          DisFlush();
        }
      }
      else
      {
        if (Mode == DISASSEMBLE)
        {
          DisC(VC_END);
          DisLogComments(".end (block)");
          DisFlush();
        }
      }

      // DebugLog(MEDIUM, "ParseBlock: Tail check...\n");
      // u8 *TailCheck = C;
      // while (*TailCheck++ == VC_END) {}
      // if (TailCheck >= End)
      // {
      //   DebugLog(
      //       MEDIUM,
      //       "ParseBlock: Detected end of block. %d FF\n",
      //       TailCheck - C);
      //   break;
      // }
    }
  }

  Nest--;
}

void
decompiler::DisD(u32 Value)
{
  u8 *P = (u8 *)&Value;
  sprintf_s(
      DisTemp,
      TEMP_BUFFER_SIZE,
      "%02x %02x %02x %02x ",
      P[0],
      P[1],
      P[2],
      P[3]);
  u64 L = strlen(DisTemp);
  memcpy(DisByteCodesP, DisTemp, L);
  DisByteCodesP += L;
}

void
decompiler::DisW(u16 Value)
{
  u8 *P = (u8 *)&Value;
  sprintf_s(DisTemp, TEMP_BUFFER_SIZE, "%02x %02x ", P[0], P[1]);
  u64 L = strlen(DisTemp);
  memcpy(DisByteCodesP, DisTemp, L);
  DisByteCodesP += L;
}

void
decompiler::DisC(u8 Value)
{
  sprintf_s(DisTemp, TEMP_BUFFER_SIZE, "%02x ", Value);
  // Log("DisC %d\n", Value);
  u64 L = strlen(DisTemp);
  // Log("DisC L %d\n", L);
  memcpy(DisByteCodesP, DisTemp, L);
  DisByteCodesP += L;
}

void
decompiler::ParseEvent(u64 Index, u8 *RetryAddress)
{
  Nest = 0;

  if (Mode == DISASSEMBLE)
    DisSaveAddress('*');

  DebugLog(MEDIUM, "\nParseEvent %d (of %d)\n", Index, NumScripts);
  CurrentScriptIndex = Index;
  CurrentScriptBase = ScriptBase + ScriptOffsetTable[Index];
  CurrentScriptEnd = ScriptBase + ScriptOffsetTable[Index + 1];
  CurrentScriptLength = ScriptOffsetTable[Index + 1] - ScriptOffsetTable[Index];
  DebugLog(MEDIUM, "ParseEvent: CurrentScriptLength %d\n", CurrentScriptLength);
  C = CurrentScriptBase;
  if (RetryAddress)
  {
    ParseFailure = false;
    C = RetryAddress;
  }
  DebugLog(MEDIUM, "ParseEvent: First byte '%c' %d\n", *C, *C);

  if (!RetryAddress)
  {
    AddIdent("event");
    char Num[TEMP_BUFFER_SIZE];
    sprintf_s(Num, TEMP_BUFFER_SIZE, "/*%lld*/", Index);
    AddSymbol(NewString(Num));
    AddSymbol("{");
  }
  // Log("CurrentScriptEnd %d\n", CurrentScriptEnd - CurrentScriptBase);

  if (Mode == DISASSEMBLE)
  {
    Log("\n.event %lld\n", Index);
  }

  switch (*C)
  {
    case VC_EXEC:
    case VC_VAR0_ASSIGN:
    case VC_VAR1_ASSIGN:
    case VC_VAR2_ASSIGN:
    case VC_GENERAL_IF:
    case VC_FOR_LOOP0:
    case VC_FOR_LOOP1:
    case VC_GOTO:
    case VC_SWITCH:
    case VC_END: ParseBlock(CurrentScriptEnd); break;
    default:
    {
      AddIdent("return");
      AddSymbol(";");
      // AddIdent("/* Skipping. Bad offset? */");
      break;
    }
  }

  // ExpectOpCode(VC_END);
  DebugLog(MEDIUM, "ParseEvent END\n");

  // if (Mode == DISASSEMBLE)
  // {
  if (ParseFailure)
  {
    Log("Parse failure. Attempting to recover...\n");
    s64 BytesLeft = CurrentScriptEnd - C;
    if (BytesLeft < 0)
      Log("Negative bytes left: %d. Skipping to next event...\n", BytesLeft);
    else if (BytesLeft > 0)
    {
      Log("ParseFailure: Skipping past next 0xff byte\n");
      u8 *P = C;
      while (*C++ != VC_END) {}
      DumpHex("Skipped Bytes", P, C - P, C - P);
      Log("Retry parsing the rest of event %d\n", Index);

      ParseEvent(Index, C);
    }
  }

  // Log("D\n");
  AddSymbol("}");
  // }
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
decompiler::PeekD()
{
  return *(u32 *)C;
}
u16
decompiler::PeekW()
{
  return *(u16 *)C;
}
u32
decompiler::GetD()
{
  return C += 4, *(u32 *)(C - 4);
}
u16
decompiler::GetW()
{
  return C += 2, *(u16 *)(C - 4);
}

void
decompiler::Init(buffer *Buffer, u64 MaxTokens, decomp_mode M)
{
  DebugLog(MEDIUM, "Decompile.Init: Size %lld\n", Buffer->Length);

  Mode = M;

  NumAddressRefs = 0;
  AddressRefs = NewList(&TempPool, 1000, u64);
  DisByteCodesP = DisByteCodes;

  NumGotoRefs = 0;
  GotoRefs = NewList(&TempPool, 1000, u64);

  NumIfRefs = 0;
  IfRefs = NewList(&TempPool, 1000, u64);

  memset(DisTemp, 0, TEMP_BUFFER_SIZE);
  memset(DisByteCodes, 0, TEMP_BUFFER_SIZE);
  memset(DisComments, 0, TEMP_BUFFER_SIZE);

  Data = Buffer->Data;
  DataSize = Buffer->Length;
  DataEnd = Data + DataSize;
  C = Data;

  DebugLog(LOW, "MaxTokens %d\n", MaxTokens);
  TokenList.SetMaxTokens(MaxTokens);

  NumScripts = GetD();
  DebugLog(MEDIUM, "NumScripts %d\n", NumScripts);
  if (NumScripts < 0 || NumScripts > 100)
    Fail("Scripts negative or >100: %d\n", NumScripts);
  ScriptOffsetTable = (u32 *)NewTempBuffer(sizeof(u32 *) * (NumScripts + 1));
  memcpy(ScriptOffsetTable, C, NumScripts * 4);
  C += NumScripts * 4;

  ScriptBase = C;

  // NOTE(aen): Attempt to fixup corrupt offset tables. In PHAGE, every offset
  // which follows a corruption will be shifted ahead by a fixed amount.
  for (u64 Index = 1; Index < NumScripts; Index++)
  {
    u8 *Head = ScriptBase + ScriptOffsetTable[Index];
    if (Index && Head[-1] != 0xff)
    {
      u8 *P = Head - 1;
      while (P != ScriptBase && *P != 0xff)
        P--;
      if (P == ScriptBase)
      {
        Log("/*Script %d has a corrupt offset. No 0xff byte found earlier.*/",
            Index);
      }
      else
      {
        u32 NearestEndByte = (u32)(Head - (P + 1));

        Log("/*Script %d has a corrupt offset. Rewinding all subsequent "
            "offsets by %d bytes.*/",
            Index,
            NearestEndByte);
        for (u64 FixupIndex = Index; FixupIndex < NumScripts; FixupIndex++)
          ScriptOffsetTable[FixupIndex] -= NearestEndByte;
      }
    }
  }

  u64 HeaderSize = ScriptBase - Data;
  // NOTE(aen): So we can always check +1 to get length of current script
  DebugLog(MEDIUM, "DataSize %lld\n", DataSize);
  u32 LastOffset = ScriptOffsetTable[NumScripts - 1];
  DebugLog(MEDIUM, "Last offset %d\n", LastOffset);
  u64 Size = DataSize;
  DebugLog(MEDIUM, "Last size %lld\n", Size);
  ScriptOffsetTable[NumScripts] = (u32)(DataSize - HeaderSize);
  DebugLog(MEDIUM, "Load: Final offset %d\n", ScriptOffsetTable[NumScripts]);
  DebugLog(
      MEDIUM, "Load: Scripts end %d, Total end %d\n", C - ScriptBase, DataSize);

  // NOTE(aen): Validate script offsets
  for (int N = 0; N < NumScripts; N++)
  {
    u64 Current = ScriptOffsetTable[N];
    u64 Next = ScriptOffsetTable[N + 1];
    // Log("Current %lld, Next %lld, diff %lld\n", Current, Next, Next -
    // Current);
    DebugLog(LOW, "Script %d: @%lld, L%lld\n", N, Current, Next - Current);
  }
}

void
Decompile(buffer *Input, buffer *Output, u64 MaxTokens, decomp_mode Mode)
{
  Assert(Input);
  Assert(Output);

  if (!Input->Length)
  {
    Log("Empty input buffer, nothing to decompile.\n");
    return;
  }

  // Log("Decompile: Input %d\n", Input->Length);

  decompiler D;
  D.Init(Input, MaxTokens, Mode);

  for (u64 N = 0; N < D.NumScripts; N++)
    D.ParseEvent(N);

  if (Mode == DISASSEMBLE) {}
  // NOTE(aen): Full decompilation
  else
  {
    // Log("Decompile: Finished parsing events, debugging token list...\n");
    // D.TokenList.Debug();
    u64 MinifiedLength = D.TokenList.Minify(
        Output->Data, /*ForceSpaces*/ false, /*DryRun*/ true);
    // Log("Needed minified length: %d\n", MinifiedLength);
    Output->Data = (u8 *)NewTempBuffer(MinifiedLength);
    Output->Length = MinifiedLength;
    // Log("Minifying...\n");
    D.TokenList.Index = 0;
    D.TokenList.Minify(Output->Data);
  }

  // Log("MinifiedLength %d\n", MinifiedLength);
  // Log("Decompiled Output:%s\n", Output->Data);
}

void
Decompile(const char *Filename, buffer *Output, u64 MaxTokens, decomp_mode Mode)
{
  Decompile(Load(Filename), Output, MaxTokens, Mode);
}
