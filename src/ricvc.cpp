#include "code.hpp"
#include "compile.hpp"

/*  -- Ric's extensions to VCLIB.C --
 * Copyright (C)1998 Richard Lau
 * Added function:
 *       CallVCScript(idcode)                          (25/Apr/98)
 *       TextMenu(x,y,flag,ptr,"opt1","opt2",..);      (04/May/98)
 *       VCTextBox(x,y,ptr,"opt1","opt2",..);     (04/May/98)
 */

void
CallVCScript(u64 idcode)
{
  u8 *ptr, *optr;
  u64 varctr = 0;

  EmitC(EXEC);
  EmitC(idcode);
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
TextMenu(u64 idcode)
{
  u8 *ptr, *optr;
  u64 varctr = 1;

  EmitC(EXEC);
  EmitC(idcode);
  Expect("(");
  EmitOperand(); // x
  Expect(",");
  EmitOperand(); // y
  Expect(",");
  EmitOperand(); // flagidx
  Expect(",");
  EmitOperand(); // ptr
  Expect(",");

  ptr = CompileGuy.GeneratedCodeLocation;
  EmitC(varctr);
  GetString();
  EmitString(token);
  while (!NextIs(")"))
  {
    Expect(",");
    GetString();
    EmitString(token);
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
VCTextBox(u64 idcode)
{
  u8 *ptr, *optr;
  u64 varctr = 1;

  EmitC(EXEC);
  EmitC(idcode);
  Expect("(");
  EmitOperand(); // x
  Expect(",");
  EmitOperand(); // y
  Expect(",");
  EmitOperand(); // ptr
  Expect(",");

  ptr = CompileGuy.GeneratedCodeLocation;
  EmitC(varctr);
  GetString();
  EmitString(token);
  while (!NextIs(")"))
  {
    Expect(",");
    GetString();
    EmitString(token);
    varctr++;
  }
  optr = CompileGuy.GeneratedCodeLocation;
  CompileGuy.GeneratedCodeLocation = ptr;
  EmitC(varctr);
  CompileGuy.GeneratedCodeLocation = optr;
  Expect(")");
  Expect(";");
}
