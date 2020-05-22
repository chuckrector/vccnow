#include "compile.hpp"
#include "op_codes.hpp"

/*  -- Ric's extensions to VCLIB.C --
 * Copyright (C)1998 Richard Lau
 * Added function:
 *       CallVCScript(idcode)                          (25/Apr/98)
 *       TextMenu(x,y,flag,ptr,"opt1","opt2",..);      (04/May/98)
 *       VCTextBox(x,y,ptr,"opt1","opt2",..);     (04/May/98)
 */

void
CallVCScript(u64 IdCode)
{
  u8 *ptr, *optr;
  u64 varctr = 0;

  EmitC(EXEC);
  EmitC(IdCode);
  Expect("(");
  ptr = CompileGuy.GeneratedCodeLocation;
  EmitC(varctr);
  EmitOperand();
  while (!NextIs(")"))
  {
    Expect(",");
    EmitOperand();
    varctr++;
  }
  optr = CompileGuy.GeneratedCodeLocation;
  CompileGuy.GeneratedCodeLocation = ptr;
  EmitC(varctr);
  CompileGuy.GeneratedCodeLocation = optr;
  Expect(")");
  Expect(";");
}

void
TextMenu(u64 IdCode)
{
  u64 NumArgs = 1;

  EmitC(EXEC);
  EmitC(IdCode);
  Expect("(");
  EmitOperand(); // x
  Expect(",");
  EmitOperand(); // y
  Expect(",");
  EmitOperand(); // flagidx
  Expect(",");
  EmitOperand(); // NumArgsPointer
  Expect(",");

  u8 *NumArgsPointer = CompileGuy.GeneratedCodeLocation;
  EmitC(NumArgs);
  GetString();
  EmitString(GlobalToken);
  while (!NextIs(")"))
  {
    Expect(",");
    GetString();
    EmitString(GlobalToken);
    NumArgs++;
  }
  u8 *SavePointer = CompileGuy.GeneratedCodeLocation;
  CompileGuy.GeneratedCodeLocation = NumArgsPointer;
  EmitC(NumArgs);
  CompileGuy.GeneratedCodeLocation = SavePointer;
  Expect(")");
  Expect(";");
}

void
VCTextBox(u64 IdCode)
{
  u64 NumArgs = 1;

  EmitC(EXEC);
  EmitC(IdCode);
  Expect("(");
  EmitOperand(); // x
  Expect(",");
  EmitOperand(); // y
  Expect(",");
  EmitOperand(); // NumArgsPointer
  Expect(",");

  u8 *NumArgsPointer = CompileGuy.GeneratedCodeLocation;
  EmitC(NumArgs);
  GetString();
  EmitString(GlobalToken);
  while (!NextIs(")"))
  {
    Expect(",");
    GetString();
    EmitString(GlobalToken);
    NumArgs++;
  }
  u8 *SavePointer = CompileGuy.GeneratedCodeLocation;
  CompileGuy.GeneratedCodeLocation = NumArgsPointer;
  EmitC(NumArgs);
  CompileGuy.GeneratedCodeLocation = SavePointer;
  Expect(")");
  Expect(";");
}
