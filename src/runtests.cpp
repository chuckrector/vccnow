#include "decompile.hpp"
#include "log.hpp"
#include "v1vc_macro.hpp"
#include "v1vc_parser.hpp"
#include "v1vc_token.hpp"
#include <stdlib.h>
#include <string.h>

// NOTE(aen): Token list index should be pointing at the head of a macro
void
TestMacroParsing(
    TokenList_t *TokenList,
    const char *ExpectedName,
    TokenList_t *ExpectedParamList,
    const char *ExpectedExpansion)
{
  Macro_t *Macro = NewMacro();
  Macro->ParseFrom(TokenList);
  ASSERT(Macro->Token->Length == strlen(ExpectedName));
  ASSERT(!memcmp(Macro->Token->Text, ExpectedName, Macro->Token->Length));
  if (ExpectedParamList)
  {
    ASSERT(Macro->ParamList.NumTokens == ExpectedParamList->NumTokens);
    for (int i = 0; i < Macro->ParamList.NumTokens; i++)
    {
      Token_t *A = Macro->ParamList.Get(i);
      Token_t *B = ExpectedParamList->Get(i);
      ASSERT(A->Length == B->Length);
      ASSERT(!strncmp(A->Text, B->Text, A->Length));
    }
    u64 MinifiedLength = Macro->Expansion.Minify((u8 *)TempBuffer);
    ASSERT(!memcmp(TempBuffer, ExpectedExpansion, MinifiedLength));
  }
}

// NOTE(aen): Expect token list to have same # tokens and text values
void
ExpectTokenList(TokenList_t *TokenList, u64 NumTokens, char *TokenTextList[])
{
  ASSERT(NumTokens == TokenList->NumTokens);
  for (u64 i = 0; i < NumTokens; i++)
  {
    Token_t *Token = TokenList->Data[i];
    ASSERT(!memcmp(TokenTextList[i], Token->Text, Token->Length));
  }
}

void
DumpHex(const char *Title, u8 *Buffer, u64 Length)
{
  Log("%s\n\t", Title);
  if (!Length)
  {
    Log("<empty>\n");
    return;
  }
  for (int N = 0; N < Length; N++)
  {
    if (N && !(N % 8))
      Log("\n\t");
    Log("%02x ", Buffer[N]);
  }
  Log("\n");
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
ParseHexCodes(Buffer_t *Input, Buffer_t *Output)
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
    u8 Code = (u8)strtol((const char *)Head, NULL, 16);
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
AssertBytes(u8 *Buffer, u64 GeneratedByteCount, Buffer_t *HexCodeString)
{
  Buffer_t HexCodes;
  HexCodes.Data = new u8[1024];
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
AssertBytes(char *Buffer, u64 GeneratedByteCount, Buffer_t *HexCodeString)
{
  AssertBytes((u8 *)Buffer, GeneratedByteCount, HexCodeString);
}

#define ASSERT_BYTES_AUTO(Buffer, Gen, Expected)                               \
  AssertBytes(Buffer, Gen, Expected, sizeof(Expected) / sizeof(Expected[0]))

// void
// AssertCompile(
//     const char *Text,
//     Buffer_t *CompiledOutput,
//     u64 CompiledOutputLimit,
//     Buffer_t *HexCodeString)
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
AssertCompileDecompile(const char *TestName)
{
  Log("[Test] %s...", TestName);

  char TestFilename[TEMP_BUFFER_SIZE];
  sprintf_s(TestFilename, TEMP_BUFFER_SIZE, "test_data/%s.test", TestName);

  Buffer_t *TestFile = Load(TestFilename);
  // Log("TestFile: %s\n", TestFile->Data);

  u8 *FileHead = TestFile->Data;
  u8 *End = FileHead + TestFile->Length;
  u8 *SectionEnd = ParseTestSection(FileHead, End);
  // Log("A\n");
  if (SectionEnd >= End)
    Fail("Unable to find script end\n");
  // Log("B\n");
  Buffer_t VC;
  u64 VCLength = SectionEnd - FileHead;
  if (VCLength < 0 || VCLength > 10000)
    Fail("VCLength out of bounds: %d\n", VCLength);
  VC.Data = new u8[VCLength + 1];
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
  Buffer_t HexCodeStrings;
  HexCodeStrings.Data = HexCodesHead;
  HexCodeStrings.Length = SectionEnd - HexCodesHead;
  // Log("HexCodeStrings Data %s, %d\n", HexCodeStrings.Data,
  //     HexCodeStrings.Length);
  // Buffer_t HexCodes;
  // HexCodes.Data = new u8[1024];
  // ParseHexCodes(&HexCodeStrings, &HexCodes);
  // Log("HexCodes Length %d\n", HexCodes.Length);
  // Log("HexCodes Data %s\n", HexCodes.Data);
  // DumpHex("HexCodes", HexCodes.Data, HexCodes.Length);

  u8 *DecompiledHead = SkipWhite(SectionEnd + 2);
  SectionEnd = ParseTestSection(DecompiledHead, End);

  Buffer_t Decompiled;
  Decompiled.Data = DecompiledHead;
  Decompiled.Length = SectionEnd - DecompiledHead;
  // Decompiled.Data[Decompiled.Length] = 0;
  // Decompiled.Length++;
  // Log("Decompiled Length %d\n", Decompiled.Length);
  // Log("Decompiled %s\n", Decompiled.Data);

  Parser_t Parser;
  Parser.CalcPath(TestFilename);
  TokenList_t TLA;
  Parser.Reset(&VC);
  Parser.ToTokenList(&TLA);

  TokenList_t TLB;
  MacroList_t ML;
  ParseMacros(&Parser, &TLA, &TLB, &ML);
  TokenList_t TLC;
  ExpandMacros(&ML, &TLB, &TLC);
  char *PreprocessedVC = new char[TEMP_BUFFER_SIZE];
  TLC.Minify((u8 *)PreprocessedVC);
  // Log("Preprocessed VC: %s\n", PreprocessedVC);

  Buffer_t CompiledOutput;
  CompiledOutput.Data = new u8[TEMP_BUFFER_SIZE];
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
  // Log("Generated length %d\n", strlen(Temp));
  //=========

  Buffer_t ScriptOffsetTable;
  ScriptOffsetTable.Data = new u8[TEMP_BUFFER_SIZE];
  WriteScriptOffsetTableToBuffer(&ScriptOffsetTable);

  // NOTE(aen): After compilation, we still need to prefix the whole blob with
  // the script offset table header.
  Buffer_t FullBinary;
  u64 FullBinaryLength = ScriptOffsetTable.Length + CompiledOutput.Length;
  FullBinary.Data = new u8[FullBinaryLength];
  memcpy(FullBinary.Data, ScriptOffsetTable.Data, ScriptOffsetTable.Length);
  memcpy(
      FullBinary.Data + ScriptOffsetTable.Length,
      CompiledOutput.Data,
      CompiledOutput.Length);
  FullBinary.Length = FullBinaryLength;
  // DumpHex("Full Binary Hex", FullBinary.Data, FullBinary.Length);

  AssertBytes(FullBinary.Data, FullBinary.Length, &HexCodeStrings);

  Buffer_t Output;
  Output.Data = new u8[TEMP_BUFFER_SIZE];
  Output.Length = 0;
  Decompile(&FullBinary, &Output);
  bool64 BothEmpty = !Decompiled.Length && !Output.Length;
  if (!BothEmpty)
  {
    bool64 IsMatch =
        !strcmp((const char *)Decompiled.Data, (const char *)Output.Data);
    if (!IsMatch)
    {
      Log("\n");
      Log("Original Source\n%s", VC.Data);
      DumpHex("Compiled Binary", FullBinary.Data, FullBinary.Length);
      Log("\nDecompiled Source\n%s\n\n", Output.Data);
      DumpHex("Expected Decompilation", Decompiled.Data, Decompiled.Length);
      DumpHex("Got Decompilation", Output.Data, Output.Length - 1);
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
  // TokenList_t TLB;
  // MacroList_t ML;
  // ParseMacros(&Parser, &TLA, &TLB, &ML);
  // TokenList_t TLC;
  // ExpandMacros(&ML, &TLB, &TLC);
  // TLC.Minify((u8 *)TempBuffer);
  // ASSERT(!strcmp(TempBuffer, "event{a=flags[123];}"));

  // Log("[Test] Compile\n");
  // AssertCompile("event{}", "ff");
  // AssertCompile("event{vgadump();}", "01 45 ff");
  // AssertCompile(
  //     "event{if(a)vgadump();}", "05 01 0c 00 00 00 02 00 ff 01 01 45 ff");

  Log("OK\n");
}

void
RunTests()
{
  Log("RunTests\n");
  TokenList_t TokenList;
  Parser_t Parser;

  Log("[Test] AtEnd, C, Next...");
  Parser.Reset(NewBuffer("test."));
  ASSERT(!Parser.AtEnd());
  ASSERT(*Parser.C == 't');
  Parser.Next();
  ASSERT(*Parser.C == 'e');
  Parser.SkipPast(".", 1);
  ASSERT(Parser.AtEnd());
  Log("OK\n");

  Log("[Test] Skip whitespace...");
  Parser.Reset(NewBuffer("abc/*foo*///bar\nxyz"));
  Parser.SkipWhite();
  ASSERT(*Parser.C == 'a');
  Parser.C += 3;
  Parser.SkipWhite();
  ASSERT(*Parser.C == 'x');
  Log("OK\n");

  Log("[Test] Long symbols...");
  Parser.Reset(NewBuffer("+= -= *= /= %= ++ -- && || <= >= =="));
  TokenList.Reset();
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
  ASSERT(TokenList.AtEnd());
  Log("OK\n");

  Log("[Test] #define with flush empty parameter list...");
  TokenList_t ParamListEmpty;
  Parser.Reset(NewBuffer("#define DrawTextBox() @foo@"));
  TokenList.Reset();
  Parser.ToTokenList(&TokenList);
  TestMacroParsing(&TokenList, "DrawTextBox", &ParamListEmpty, "foo");
  ASSERT(TokenList.AtEnd());
  Log("OK\n");

  Log("[Test] #define with flush parameters...");
  TokenList_t ParamListDims;
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
  ASSERT(TokenList.AtEnd());
  Log("OK\n");

  Log("[Test] #define with non-flush parameters...");
  Parser.Reset(NewBuffer(
      "#define DrawTextBox ( x , y , w , h ) @CallScript(3,x,y,w,h)@"));
  TokenList.Reset();
  Parser.ToTokenList(&TokenList);
  TestMacroParsing(
      &TokenList, "DrawTextBox", &ParamListDims, "callscript(3,x,y,w,h)");
  ASSERT(TokenList.AtEnd());
  Log("OK\n");

  Log("[Test] #define expansion...");
  Parser.Reset(
      NewBuffer("#define DrawTextBox(x,y,w,h) "
                "@CallScript(3,x,y,w,h)@\nevent{DrawTextBox(11,22,33,44);}"));
  Parser.ToTokenList(&TokenList);
  MacroList_t MacroListDefExpansion;
  TokenList_t TokenListDefExpansionWithoutMacros;
  ParseMacros(
      &Parser,
      &TokenList,
      &TokenListDefExpansionWithoutMacros,
      &MacroListDefExpansion);
  TokenList_t TokenListDefExpansion;
  ExpandMacros(
      &MacroListDefExpansion,
      &TokenListDefExpansionWithoutMacros,
      &TokenListDefExpansion);
  TokenListDefExpansion.Minify((u8 *)TempBuffer);
  ASSERT(!strcmp("event{callscript(3,11,22,33,44);}", TempBuffer));
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

  Log("PASS\n");
}
