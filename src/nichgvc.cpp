#include "code.hpp"
#include "compile.hpp"

void
MagicShop()
{
  u8 *ptr;
  u8 *optr;
  u64 varctr = 1;

  EmitC(EXEC);
  EmitC(101);
  Expect("(");
  CompileGuy_t *CG = &CompileGuy;
  ptr = CG->GeneratedCodeLocation;
  EmitC(varctr);
  EmitOperand();
  while (!NextIs(")"))
  {
    Expect(",");
    EmitOperand();
    varctr++;
  }
  optr = CG->GeneratedCodeLocation;
  CG->GeneratedCodeLocation = ptr;
  EmitC(varctr);
  CG->GeneratedCodeLocation = optr;
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
