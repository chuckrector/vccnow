#include "compile.hpp"
#include "op_codes.hpp"

void
MagicShop()
{
  u64 NumArgs = 1;

  EmitC(EXEC);
  EmitC(101);
  Expect("(");
  compile_guy *CG = &CompileGuy;
  u8 *NumArgsPointer = CG->GeneratedCodeLocation;
  EmitC(NumArgs);
  EmitOperand();
  while (!NextIs(")"))
  {
    Expect(",");
    EmitOperand();
    NumArgs++;
  }
  u8 *SavePointer = CG->GeneratedCodeLocation;
  CG->GeneratedCodeLocation = NumArgsPointer;
  EmitC(NumArgs);
  CG->GeneratedCodeLocation = SavePointer;
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
  EmitString(GlobalToken);
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
