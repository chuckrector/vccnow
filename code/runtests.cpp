#include "compile.hpp"
#include "decompile.hpp"
#include "log.hpp"
#include "string.hpp"
#include "util.hpp"
#include "v1vc_macro.hpp"
#include "v1vc_parser.hpp"
#include "v1vc_token.hpp"
#include "vcc.hpp"
#include <stdlib.h>
#include <string.h>

// NOTE(aen): Token list index should be pointing at the head of a macro
void
TestMacroParsing(
    token_list *TokenList,
    char *ExpectedName,
    token_list *ExpectedParamList,
    char *ExpectedExpansion)
{
  macro *Macro = NewMacro();
  Macro->ParseFrom(TokenList);
  Assert(Macro->Token->Length == StringLength(ExpectedName));
  Assert(!memcmp(Macro->Token->Text, ExpectedName, Macro->Token->Length));
  if (ExpectedParamList)
  {
    Assert(Macro->ParamList.NumTokens == ExpectedParamList->NumTokens);
    for (int i = 0; i < Macro->ParamList.NumTokens; i++)
    {
      token *A = Macro->ParamList.Get(i);
      token *B = ExpectedParamList->Get(i);
      Assert(A->Length == B->Length);
      Assert(!strncmp(A->Text, B->Text, A->Length));
    }
    u64 MinifiedLength = Macro->Expansion.Minify((u8 *)TempBuffer);
    Assert(!memcmp(TempBuffer, ExpectedExpansion, MinifiedLength));
  }
}

// NOTE(aen): Expect token list to have same # tokens and text values
void
ExpectTokenList(token_list *TokenList, u64 NumTokens, char *TokenTextList[])
{
  Assert(NumTokens == TokenList->NumTokens);
  for (u64 i = 0; i < NumTokens; i++)
  {
    token *Token = TokenList->Get(i);
    Assert(!memcmp(TokenTextList[i], Token->Text, Token->Length));
  }
}

u8 *
SkipWhite(u8 *P)
{
  while (*P && *P <= ' ')
    P++;
  return P;
}

u8 *
SkipNewLine(u8 *C)
{
  while (*C && (*C == '\r' || *C == '\n'))
    C++;
  return C;
}
u8 *
SkipToNewLine(u8 *C)
{
  while (*C && *C != '\r' && *C != '\n')
    C++;
  C = SkipNewLine(C);
  return C;
}

void
ParseHexCodes(buffer *Input, buffer *Output)
{
  u8 *In = Input->Data;
  u64 N = 0;
  u8 *End = In + Input->Length;
  // Log("ParseHexCodes: InputLength %d, End %d, %s\n", Input->Length, End - In,
  //     In);
  In = SkipWhite(In);
  while (*In && In < End)
  {
    while (*In == '.')
    {
      In = SkipToNewLine(In);
      In = SkipWhite(In);
    }
    if (*In == '/')
      break; // TODO(aen): Don't pass this in to begin with
    u8 *Head = In;
    while (*In && *In > ' ')
      In++;
    // Log("Parsing: %c %d\n", *Head, *Head);
    u8 Code = (u8)strtol((char *)Head, NULL, 16);
    // Log("Saving hex code %02x\n", Code);
    Output->Data[N++] = Code;
    In = SkipWhite(In);
  }
  Output->Length = N;
  Output->Data[N] = 0;
  // Log("ParseHexCodes: Found %d hex codes in %.*s\n", N, Input->Length,
  //     Input->Data);
}

void
AssertBytes(u8 *Buffer, u64 GeneratedByteCount, buffer *HexCodeString)
{
  buffer HexCodes;
  HexCodes.Data = (u8 *)NewTempBuffer(1024);
  ParseHexCodes(HexCodeString, &HexCodes);

  if (GeneratedByteCount != HexCodes.Length)
  {
    Log("\nAssertBytes: Expected %d bytes but got %d\n",
        HexCodes.Length,
        GeneratedByteCount);
    DumpHex("Generated", Buffer, GeneratedByteCount);
    DumpHex("Expected", HexCodes.Data, HexCodes.Length);
    exit(-1);
  }
  // Log("AssertBytes: HexCodes.Length %d\n", HexCodes.Length);
  for (int N = 0; N < HexCodes.Length; N++)
  {
    if (Buffer[N] != HexCodes.Data[N])
    {
      DumpHex("Generated", Buffer, GeneratedByteCount);
      DumpHex("Expected", HexCodes.Data, HexCodes.Length);
      Fail("FAIL");
    }
  }
}

void
AssertBytes(char *Buffer, u64 GeneratedByteCount, buffer *HexCodeString)
{
  AssertBytes((u8 *)Buffer, GeneratedByteCount, HexCodeString);
}

#define ASSERT_BYTES_AUTO(Buffer, Gen, Expected)                               \
  AssertBytes(Buffer, Gen, Expected, sizeof(Expected) / sizeof(Expected[0]))

// void
// AssertCompile(
//     char *Text,
//     buffer *CompiledOutput,
//     u64 CompiledOutputLimit,
//     buffer *HexCodeString)
// {
//   // Log("AssertCompile\n");
//   u64 GeneratedByteCount = 0;
//   CompileToBuffer(
//       COMPILE_TYPE_MAP,
//       Text,
//       CompiledOutput->Data,
//       CompiledOutputLimit,
//       &GeneratedByteCount);
//   // Log("AssertCompile: GeneratedByteCount %d\n", GeneratedByteCount);
//   CompiledOutput->Length = GeneratedByteCount;

//   //..const u8 ExpectedBytes[] = TheBytes;
//   AssertBytes(CompiledOutput->Data, GeneratedByteCount, HexCodeString);
// }

u8 *
ParseTestSection(u8 *Head, u8 *End)
{
  // Log("ParseTestSection\n");
  u8 *C = Head;
  // Log("Len %d\n", End - Head);
  for (; *C && C < End; C++)
  {
    // Log("c '%c' (%d)\n", *C, *C);
    if (*C == '\n' || *C == '\r')
    {
      C = SkipNewLine(C);
      // Log("Skip: c '%c' (%d), %s\n", *C, *C, C);
      // TODO(aen): Guard against access violation at end of file
      // C + 3 < End // This doesn't seem to be accurate
      if (*C && !memcmp(C, "//", 2))
      {
        // Log("ScriptEnd found\n");
        return C;
      }
    }
  }
  return C;
}

void
AssertCompileDecompile(char *TestName)
{
  Log("[Test] %s...", TestName);

  char TestFilename[TEMP_BUFFER_SIZE];
  sprintf_s(TestFilename, TEMP_BUFFER_SIZE, "test_data/%s.test", TestName);

  buffer *TestFile = LoadEntireFile(TestFilename);
  // Log("TestFile: %s\n", TestFile->Data);

  u8 *FileHead = TestFile->Data;
  u8 *End = FileHead + TestFile->Length;
  u8 *SectionEnd = ParseTestSection(FileHead, End);
  // Log("A\n");
  if (SectionEnd >= End)
    Fail("Unable to find script end\n");
  // Log("B\n");
  buffer VC;
  u64 VCLength = SectionEnd - FileHead;
  if (VCLength < 0 || VCLength > 10000)
    Fail("VCLength out of bounds: %d\n", VCLength);
  VC.Data = (u8 *)NewTempBuffer(VCLength + 1);
  memcpy(VC.Data, FileHead, VCLength);
  VC.Length = VCLength;
  VC.Data[VCLength] = 0;
  // Log("VCData %s\n", VC.Data);
  // Log("VC Length %d\n", VC.Length);

  u8 *HexCodesHead = SectionEnd + 2;
  // Log("HexCodesHead %s\n", HexCodesHead);
  SectionEnd = ParseTestSection(HexCodesHead, End);
  // Log("Total Length %d\n", End - FileHead);
  // Log("File Length %d\n", TestFile->Length);
  // Log("Current C %d\n", SectionEnd - HexCodesHead);
  // TODO(aen): Why can this end up beyond the end of the file buffer size?
  // Log("HexCodesHead Full %.*s\n", SectionEnd - HexCodesHead, HexCodesHead);
  if (SectionEnd >= End)
    Fail("Unable to find hex codes end\n");
  // SectionEnd = SkipWhite(SectionEnd + 2);
  buffer HexCodeStrings;
  HexCodeStrings.Data = HexCodesHead;
  HexCodeStrings.Length = SectionEnd - HexCodesHead;
  // Log("HexCodeStrings Data %s, %d\n", HexCodeStrings.Data,
  //     HexCodeStrings.Length);
  // buffer HexCodes;
  // HexCodes.Data = NewTempBuffer(1024);
  // ParseHexCodes(&HexCodeStrings, &HexCodes);
  // Log("HexCodes Length %d\n", HexCodes.Length);
  // Log("HexCodes Data %s\n", HexCodes.Data);
  // DumpHex("HexCodes", HexCodes.Data, HexCodes.Length);

  u8 *DecompiledHead = SkipWhite(SectionEnd + 2);
  SectionEnd = ParseTestSection(DecompiledHead, End);

  buffer Decompiled;
  Decompiled.Data = DecompiledHead;
  Decompiled.Length = SectionEnd - DecompiledHead;
  // Decompiled.Data[Decompiled.Length] = 0;
  // Decompiled.Length++;
  // Log("Decompiled Length %d\n", Decompiled.Length);
  // Log("Decompiled %s\n", Decompiled.Data);

  parser Parser;
  Parser.CalcPath(TestFilename);
  token_list TLA;
  Parser.Reset(&VC);
  Parser.ToTokenList(&TLA);

  token_list TLB;
  macro_list ML;
  ParseMacros(&Parser, &TLA, &TLB, &ML);
  token_list TLC;
  ExpandMacros(&ML, &TLB, &TLC);
  char *PreprocessedVC = (char *)NewTempBuffer(TEMP_BUFFER_SIZE);
  TLC.Minify((u8 *)PreprocessedVC);
  // Log("Preprocessed VC: %s\n", PreprocessedVC);

  buffer CompiledOutput;
  CompiledOutput.Data = (u8 *)NewTempBuffer(TEMP_BUFFER_SIZE);
  //=========
  u64 GeneratedByteCount = 0;
  CompileToBuffer(
      COMPILE_TYPE_MAP,
      PreprocessedVC,
      CompiledOutput.Data,
      TEMP_BUFFER_SIZE,
      &GeneratedByteCount);
  // Log("AssertCompile: GeneratedByteCount %d\n", GeneratedByteCount);
  CompiledOutput.Length = GeneratedByteCount;

  //..const u8 ExpectedBytes[] = TheBytes;
  // AssertBytes(CompiledOutput->Data, GeneratedByteCount, HexCodeString);
  // AssertCompile(
  //     PreprocessedVC, &CompiledOutput, TEMP_BUFFER_SIZE, &HexCodeStrings);
  // Log("Compiled output length %d\n", CompiledOutput.Length);
  // Log("Decompiled length %d\n", Decompiled.Length);
  // Log("Generated length %d\n", StringLength(Temp));
  //=========

  buffer ScriptOffsetTable;
  ScriptOffsetTable.Data = (u8 *)NewTempBuffer(TEMP_BUFFER_SIZE);
  WriteScriptOffsetTableToBuffer(&ScriptOffsetTable);

  // NOTE(aen): After compilation, we still need to prefix the whole blob with
  // the script offset table header.
  buffer FullBinary;
  u64 FullBinaryLength = ScriptOffsetTable.Length + CompiledOutput.Length;
  FullBinary.Data = (u8 *)NewTempBuffer(FullBinaryLength);
  memcpy(FullBinary.Data, ScriptOffsetTable.Data, ScriptOffsetTable.Length);
  memcpy(
      FullBinary.Data + ScriptOffsetTable.Length,
      CompiledOutput.Data,
      CompiledOutput.Length);
  FullBinary.Length = FullBinaryLength;
  // DumpHex("Full Binary Hex", FullBinary.Data, FullBinary.Length);

  AssertBytes(FullBinary.Data, FullBinary.Length, &HexCodeStrings);

  buffer Output;
  Output.Data = (u8 *)NewTempBuffer(TEMP_BUFFER_SIZE);
  Output.Length = 0;
  Decompile(&FullBinary, &Output, 1000, DECOMPILE);
  b64 BothEmpty = !Decompiled.Length && !Output.Length;
  if (!BothEmpty)
  {
    b64 LengthsMatch = Decompiled.Length == Output.Length;
    b64 IsMatch =
        !memcmp((char *)Decompiled.Data, (char *)Output.Data, Output.Length);
    if (LengthsMatch && IsMatch) {}
    else
    {
      Log("\n");
      Log("Original Source\n%s", VC.Data);
      DumpHex("Compiled Binary", FullBinary.Data, FullBinary.Length);
      Log("\nDecompiled Source\n%s\n\n", Output.Data);
      DumpHex("Expected Decompilation", Decompiled.Data, Decompiled.Length);
      DumpHex("Got Decompilation", Output.Data, Output.Length);
      Fail(
          "Expected\n%.*s\nBut got\n%.*s\n",
          Decompiled.Length,
          Decompiled.Data,
          Output.Length,
          Output.Data);
    }
  }

  // Log("%s\n", TempBuffer);
  // Parser
  //     .Reset()

  //         Parser.Load("test_data/004_macro_with_parameters.vc");
  // Parser.ToTokenList(&TLA);
  // token_list TLB;
  // macro_list ML;
  // ParseMacros(&Parser, &TLA, &TLB, &ML);
  // token_list TLC;
  // ExpandMacros(&ML, &TLB, &TLC);
  // TLC.Minify((u8 *)TempBuffer);
  // Assert(StringsMatch(TempBuffer, "event{a=flags[123];}"));

  // Log("[Test] Compile\n");
  // AssertCompile("event{}", "ff");
  // AssertCompile("event{vgadump();}", "01 45 ff");
  // AssertCompile(
  //     "event{if(a)vgadump();}", "05 01 0c 00 00 00 02 00 ff 01 01 45 ff");

  Log("OK\n");
}

void
TestPools()
{
  char FancyString[1024];
  for (u64 Loop = 0; Loop < 2; Loop++)
  {
    Log("[Test] TestPools: Pass #%d\n", 1 + Loop);

    u64 TempBufferSize = TEMP_BUFFER_SIZE;
    u64 NewTempBufferCount = POOL_SIZE / TempBufferSize - 1;
    FormatU64(NewTempBufferCount, FancyString);
    Log("\tNewTempBuffer %d x%s\n", TempBufferSize, FancyString);
    for (u64 i = 0; i < NewTempBufferCount; i++)
    {
      char *T = NewTempBuffer(TempBufferSize);
      Assert(T);
      for (int j = 0; j < TempBufferSize; j++)
        Assert(T[j] == 0);
    }

    char *BufferString = "Hello!";
    u64 BufferLen = StringLength(BufferString) + 1;
    u64 NewBufferCount = POOL_SIZE / sizeof(buffer);
    FormatU64(NewBufferCount, FancyString);
    Log("\tNewBuffer x%s\n", FancyString);
    for (u64 i = 0; i < NewBufferCount; i++)
    {
      buffer *B = NewBuffer((u8 *)BufferString, BufferLen);
      Assert(B->Length == 7);
      Assert(StringsMatch((char *)B->Data, BufferString));
    }

    u64 NewTokenCount = TOKEN_POOL_SIZE / sizeof(token);
    FormatU64(NewTokenCount, FancyString);
    Log("\tNewToken x%s\n", FancyString);
    for (u64 i = 0; i < NewTokenCount; i++)
    {
      token *T = NewToken();
      Assert(T);
    }
    ResetPool(&TokenPool);

    u64 NewMacroCount = POOL_SIZE / (sizeof(macro) + (sizeof(token) * 104));
    FormatU64(NewMacroCount, FancyString);
    Log("\tNewMacro x%s\n", FancyString);
    for (u64 i = 0; i < NewMacroCount; i++)
    {
      macro *M = NewMacro();
      Assert(M);
    }

    ResetPool(&TempPool);
    ResetPool(&BufferPool);
    ResetPool(&TokenPool);
    ResetPool(&MacroPool);
  }

  u64 NewTokenListCount = POOL_SIZE / sizeof(token_list);
  FormatU64(NewTokenListCount, FancyString);
  Log("[Test] TokenList x%s\n", FancyString);
  for (u64 i = 0; i < NewTokenListCount; i++)
  {
    token_list TokenList;
    Assert(TokenList.NumTokens == 0);
  }
  ResetPool(&TokenPool);

  // TODO(aen): Figure why I can only allocate peanuts for macro lists, whereas
  // Ican allocate token lists like gangbusters... I don't get it.
  u64 NewMacroListCount =
      POOL_SIZE /
      (sizeof(macro_list) + (DEFAULT_NUM_MACROS_PER_LIST * sizeof(macro)));
  FormatU64(NewMacroListCount, FancyString);
  Log("[Test] MacroList x%s\n", FancyString);
  for (u64 i = 0; i < NewMacroListCount; i++)
  {
    macro_list MacroList;
    Assert(MacroList.NumMacros == 0);
  }

  ResetPool(&TokenPool);
  ResetPool(&MacroPool);

  // TODO(aen): Get rid of this global buffer. A bunch of stuff relies on it
  // currently and we should just be allocating as we need locally.
  TempBuffer = NewTempBuffer(TEMP_BUFFER_SIZE);
}

void
TestFormatU64(u64 Num, char *Expect)
{
  char Output[1024];
  FormatU64(Num, Output);
  Assert(StringsMatch(Expect, Output));
}

void
RunTests()
{
  Log("RunTests\n");

  Log("[Test] FormatU64\n");
  TestFormatU64(0, "0");
  TestFormatU64(12, "12");
  TestFormatU64(123, "123");
  TestFormatU64(1234, "1,234");
  TestFormatU64(12345, "12,345");
  TestFormatU64(123456, "123,456");
  TestFormatU64(1234567, "1,234,567");
  TestFormatU64(12345678, "12,345,678");
  TestFormatU64(123456789, "123,456,789");

  Log("[Test] StringsMatch\n");
  Assert(StringsMatch("", ""));
  Assert(StringsMatch("foo", "foo"));
  Assert(!StringsMatch("foo", "bar"));
  Assert(!StringsMatch("foo", "fo"));
  Assert(!StringsMatch("fo", "foo"));

  TestPools();

  Log("[Test] AtEnd, C, Next...");
  parser Parser;
  Parser.Reset(NewBuffer("test."));
  Assert(!Parser.AtEnd());
  Assert(*Parser.C == 't');
  Parser.Next();
  Assert(*Parser.C == 'e');
  Parser.SkipPast(".", 1);
  Assert(Parser.AtEnd());
  Log("OK\n");

  Log("[Test] Skip whitespace...");
  Parser.Reset(NewBuffer("abc/*foo*///bar\nxyz"));
  Parser.SkipWhite();
  Assert(*Parser.C == 'a');
  Parser.C += 3;
  Parser.SkipWhite();
  Assert(*Parser.C == 'x');
  Log("OK\n");

  token_list TokenList;
  TokenList.SetMaxTokens(100);

  Log("[Test] Long symbols...");
  Parser.Reset(NewBuffer("+= -= *= /= %= ++ -- && || <= >= =="));
  Parser.ToTokenList(&TokenList);
  TokenList.Minify((u8 *)TempBuffer);
  char *LongTL[] = {
      "+=", "-=", "*=", "/=", "%=", "++", "--", "&&", "||", "<=", ">=", "=="};
  ExpectTokenList(&TokenList, 12, LongTL);
  Log("OK\n");

  Log("[Test] #define with space and flush value...");
  Parser.Reset(NewBuffer("#define Cecil @4@\nevent{}"));
  TokenList.Reset();
  Parser.ToTokenList(&TokenList);
  TestMacroParsing(&TokenList, "Cecil", 0, "4");
  Log("OK\n");

  Log("[Test] #define with flush @ and space-padded value...");
  Parser.Reset(NewBuffer("#define Cecil@ 4 @\nevent{}"));
  TokenList.Reset();
  Parser.ToTokenList(&TokenList);
  TestMacroParsing(&TokenList, "Cecil", 0, "4");
  Log("OK\n");

  Log("[Test] #define with non-flush @ and space-padded value...");
  Parser.Reset(NewBuffer("#define Cecil @ 4 @\nevent{}"));
  TokenList.Reset();
  Parser.ToTokenList(&TokenList);
  TestMacroParsing(&TokenList, "Cecil", 0, "4");
  Log("OK\n");

  Log("[Test] #define with immediate EOF...");
  Parser.Reset(NewBuffer("#define Cecil@4@"));
  TokenList.Reset();
  Parser.ToTokenList(&TokenList);
  TestMacroParsing(&TokenList, "Cecil", 0, "4");
  Assert(TokenList.AtEnd());
  Log("OK\n");

  Log("[Test] #define with flush empty parameter list...");
  token_list ParamListEmpty;
  Parser.Reset(NewBuffer("#define DrawTextBox() @foo@"));
  TokenList.Reset();
  Parser.ToTokenList(&TokenList);
  TestMacroParsing(&TokenList, "DrawTextBox", &ParamListEmpty, "foo");
  Assert(TokenList.AtEnd());
  Log("OK\n");

  Log("[Test] #define with flush parameters...");
  token_list ParamListDims;
  ParamListDims.AddToken("x", 1, TT_IDENT, 0, 0);
  ParamListDims.AddToken("y", 1, TT_IDENT, 0, 0);
  ParamListDims.AddToken("w", 1, TT_IDENT, 0, 0);
  ParamListDims.AddToken("h", 1, TT_IDENT, 0, 0);
  Parser.Reset(
      NewBuffer("#define DrawTextBox(x,y,w,h)@ CallScript(3,x,y,w,h) @"));
  TokenList.Reset();
  Parser.ToTokenList(&TokenList);
  TestMacroParsing(
      &TokenList, "DrawTextBox", &ParamListDims, "callscript(3,x,y,w,h)");
  Assert(TokenList.AtEnd());
  Log("OK\n");

  Log("[Test] #define with non-flush parameters...");
  Parser.Reset(NewBuffer(
      "#define DrawTextBox ( x , y , w , h ) @CallScript(3,x,y,w,h)@"));
  TokenList.Reset();
  Parser.ToTokenList(&TokenList);
  TestMacroParsing(
      &TokenList, "DrawTextBox", &ParamListDims, "callscript(3,x,y,w,h)");
  Assert(TokenList.AtEnd());
  Log("OK\n");

  Log("[Test] #define expansion...");
  Parser.Reset(
      NewBuffer("#define DrawTextBox(x,y,w,h) "
                "@CallScript(3,x,y,w,h)@\nevent{DrawTextBox(11,22,33,44);}"));
  Parser.ToTokenList(&TokenList);
  macro_list MacroListDefExpansion;
  token_list TokenListDefExpansionWithoutMacros;
  ParseMacros(
      &Parser,
      &TokenList,
      &TokenListDefExpansionWithoutMacros,
      &MacroListDefExpansion);
  token_list TokenListDefExpansion;
  ExpandMacros(
      &MacroListDefExpansion,
      &TokenListDefExpansionWithoutMacros,
      &TokenListDefExpansion);
  TokenListDefExpansion.Minify((u8 *)TempBuffer);
  Assert(StringsMatch("event{callscript(3,11,22,33,44);}", TempBuffer));
  Log("OK\n");

  AssertCompileDecompile("000_empty");
  AssertCompileDecompile("001_whitespace_and_comments");
  AssertCompileDecompile("002_empty_event");
  AssertCompileDecompile("003_simple_macro");
  AssertCompileDecompile("004_macro_with_parameters");
  AssertCompileDecompile("005_simple_include");
  AssertCompileDecompile("006_if_literal_empty");
  AssertCompileDecompile("007_if_literal_exec");
  AssertCompileDecompile("008_if_literal_exec_block");
  AssertCompileDecompile("009_if_literal_exec_else_exec");
  AssertCompileDecompile("010_if_literal_exec_exec");
  AssertCompileDecompile("011_return");
  AssertCompileDecompile("012_if_literal_then_return");
  AssertCompileDecompile("013_if_literal_then_return_else_return");
  AssertCompileDecompile("014_if_literal_then_return_return");
  AssertCompileDecompile("015_while_literal_return");
  AssertCompileDecompile("016_while_literal_return_return");
  AssertCompileDecompile("017_while_literal_empty");
  AssertCompileDecompile("018_for0_empty");
  AssertCompileDecompile("019_for0_if_var0_return");
  AssertCompileDecompile("020_for1_if_var0_return");
  AssertCompileDecompile("021_for1_empty");
  AssertCompileDecompile("022_nested_switch");
  AssertCompileDecompile("023_nested_switch2");
  AssertCompileDecompile("024_repeated_switch");
  AssertCompileDecompile("025_switch_case_if_switch");
  AssertCompileDecompile("026_for0_if");
  AssertCompileDecompile("027_math");

  Log("PASS\n");
}
