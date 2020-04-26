#include "code.h"
#include "compile.h"

/*  -- Ric's extensions to VCLIB.C --
 * Copyright (C)1998 Richard Lau
 * Added function:
 *       CallVCScript(idcode)                          (25/Apr/98)
 *       TextMenu(x,y,flag,ptr,"opt1","opt2",..);      (04/May/98)
 *       VCTextBox(x,y,ptr,"opt1","opt2",..);     (04/May/98)
 */

void
CallVCScript(unsigned char idcode) /* -- ric: 25/Apr/98 -- */
{
  char *ptr, *optr, varctr = 0;

  EmitC(EXEC);
  EmitC(idcode);
  Expect("(");
  ptr = cpos;
  EmitC(varctr);
  EmitOperand();
  while (!NextIs(")"))
  {
    Expect(",");
    EmitOperand();
    varctr++;
  }
  optr = cpos;
  cpos = ptr;
  EmitC(varctr);
  cpos = optr;
  Expect(")");
  Expect(";");
}

void
TextMenu(unsigned char idcode) /* -- ric: 04/May/98 -- */
{
  char *ptr, *optr, varctr = 1;

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

  ptr = cpos;
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
  optr = cpos;
  cpos = ptr;
  EmitC(varctr);
  cpos = optr;
  Expect(")");
  Expect(";");
}

void
VCTextBox(unsigned char idcode) /* -- ric: 04/May/98 -- */
{
  char *ptr, *optr, varctr = 1;

  EmitC(EXEC);
  EmitC(idcode);
  Expect("(");
  EmitOperand(); // x
  Expect(",");
  EmitOperand(); // y
  Expect(",");
  EmitOperand(); // ptr
  Expect(",");

  ptr = cpos;
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
  optr = cpos;
  cpos = ptr;
  EmitC(varctr);
  cpos = optr;
  Expect(")");
  Expect(";");
}
