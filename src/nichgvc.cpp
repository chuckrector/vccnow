#include "code.h"
#include "compile.h"

void
MagicShop()
{
  char *ptr;
  char *optr;
  char varctr = 1;

  EmitC(EXEC);
  EmitC(101);
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
PlayVAS()
{
  EmitC(EXEC);
  EmitC(103);
  Expect("(");
  GetString();
  EmitString(token);
  Expect(",");
  EmitOperand(); // speed
  Expect(",");
  EmitOperand(); // x size
  Expect(",");
  EmitOperand(); // y size
  Expect(",");
  EmitOperand(); // x placement
  Expect(",");
  EmitOperand(); // y placement

  Expect(")");
  Expect(";");
}
