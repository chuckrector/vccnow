// **********************************************************************
// *                                                                    *
// *                     The Verge-C Compiler v.0.10                    *
// *                     Copyright (C)1997 BJ Eirich                    *
// *                                                                    *
// * Module: FUNCLIB.C                                                  *
// *                                                                    *
// * Description: This simply parses and generates the output code      *
// * for the built in library functions.                                *
// *                                                                    *
// * Portability: ANSI C. Should compile on any 32-bit compiler.        *
// *                                                                    *
// **********************************************************************

#include "funclib.hpp"
#include "compile.hpp"
#include "op_codes.hpp"

/*  --  Added by Ric --
 * Added functions:
 *       VCBox(x1,y1,x2,y2); (21/Apr/98)
 *       VCCharName(x, y, party.dat index, center); (21/Apr/98)
 *       VCItemName(x, y, items.dat index, center); (21/Apr/98)
 *       VCItemDesc(x, y, items.dat index, center); (21/Apr/98)
 *       VCItemImage(x, y, items.dat index, greyflag); (22/Apr/98)
 *       VCATextNum(x, y, number, align);              (24/Apr/98)
 *       VCSpc(x, y, speech portrait, greyflag);       (24/Apr/98)
 *       CallVCScript(event number, option var1...);   (25/Apr/98)
 *           (used for CallEffect and CallScript)
 *       VCLine(x1, y1, x2, y2, color);                (??/???/??)
 *       GetMagic (character, spell);                  (??/???/??)
 *       BindKey(key code, script);                    (03/May/98)
 *       TextMenu(x,y,flag,ptr,"opt1","opt2",..);      (04/May/98)
 *       ItemMenu(roster order index);                 (03/May/98)
 *       EquipMenu(roster order index);                (03/May/98)
 *       MagicMenu(roster order index);                (03/May/98)
 *       StatusScreen(roster order index);             (03/May/98)
 *       VCCr2(x, y, speech portrait, greyflag);       (03/May/98)
 *       VCSpellName(x, y, magic.dat index, center);   (??/???/??)
 *       VCSpellDesc(x, y, magic.dat index, center);   (??/???/??)
 *       VCSpellImage(x, y, magic.dat index, greyflag);(??/???/??)
 *       MagicShop(spell1, spell2, spell3, ... spell12); (??/??/??)
 *       VCTextBox(x,y,ptr,"opt1","opt2",..);          (04/May/98)
 *       PlayVAS("filename",speed);                    (04/May/98):NichG
 *       VCMagicImage(x, y, items.dat index, greyflag);(04/May/98)
 */
#include "nichgvc.hpp"
#include "ricvc.hpp"

/* -- -- */

void
GenericFunc(u64 IdCode, u64 NumArgs)
{
  EmitC(EXEC);
  EmitC(IdCode);
  if (!NumArgs)
  {
    Expect("(");
    Expect(")");
    Expect(";");
    return;
  }
  if (NumArgs == 1)
  {
    Expect("(");
    EmitOperand();
    Expect(")");
    Expect(";");
    return;
  }
  Expect("("); // NumArgs is greater than 1
  for (u64 Index = 1; Index < NumArgs; Index++)
  {
    EmitOperand();
    Expect(",");
  }
  EmitOperand();
  Expect(")");
  Expect(";");
}

void
MapSwitch()
{
  EmitC(EXEC); // Emit function exec code.
  EmitC(1);    // Emit which function code to exec.
  Expect("(");
  GetString();
  EmitString(GlobalToken); // Emit the map filename.
  Expect(",");
  EmitOperand();
  Expect(",");
  EmitOperand();
  Expect(",");
  EmitOperand();
  Expect(")");
  Expect(";");
}

/*
Text ()
{ unsigned char t;
  char bufr[31], curcnt,linectr;
  int tokenptr;
  int lstoken,lscur;

         EmitC (EXEC);
         EmitW (6);
         Expect ("(");
         t = ExpectNumber ();
         EmitC (t);

         GrabString ();
         tokenptr = 0;
         linectr = 0;

         while (linectr<3)
         {
            curcnt = 0;
            linectr ++;

            while (curcnt < 31)
            {      bufr[curcnt] = token[tokenptr];
                   if (token[tokenptr] == ' ')
                      { lstoken = tokenptr;
                        lscur = curcnt;
                      }
                   tokenptr ++;
                   curcnt ++;
            }
         }
         Expect (";");
}
*/

void
Text()
{
  EmitC(EXEC);
  EmitC(6);
  Expect("(");
  EmitOperand();
  Expect(",");
  GetString();
  EmitString(GlobalToken);
  Expect(",");
  GetString();
  EmitString(GlobalToken);
  Expect(",");
  GetString();
  EmitString(GlobalToken);
  Expect(")");
  Expect(";");
}

void
DoReturn()
{
  EmitC(ENDSCRIPT);
  Expect(";");
}

void
PlayMusic()
{
  EmitC(EXEC);
  EmitC(11);
  Expect("(");
  GetString();
  EmitString(GlobalToken);
  Expect(")");
  Expect(";");
}

void
Banner()
{
  EmitC(EXEC);
  EmitC(18);
  Expect("(");
  GetString();
  EmitString(GlobalToken);
  Expect(",");
  EmitOperand();
  Expect(")");
  Expect(";");
}

void
Prompt()
{
  EmitC(EXEC);
  EmitC(22);
  Expect("(");
  EmitOperand();
  Expect(",");
  GetString();
  EmitString(GlobalToken);
  Expect(",");
  GetString();
  EmitString(GlobalToken);
  Expect(",");
  GetString();
  EmitString(GlobalToken);
  Expect(",");
  EmitOperand();
  Expect(",");
  GetString();
  EmitString(GlobalToken);
  Expect(",");
  GetString();
  EmitString(GlobalToken);
  Expect(")");
  Expect(";");
}

void
ChainEvent()
{
  u64 NumArgs = 0;

  EmitC(EXEC);
  EmitC(23);
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
CallEvent()
{
  u64 NumArgs = 0;

  EmitC(EXEC);
  EmitC(24);
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
SText()
{
  EmitC(EXEC);
  EmitC(33);
  Expect("(");
  EmitOperand();
  Expect(",");
  GetString();
  EmitString(GlobalToken);
  Expect(",");
  GetString();
  EmitString(GlobalToken);
  Expect(",");
  GetString();
  EmitString(GlobalToken);
  Expect(")");
  Expect(";");
}

void
Shop()
{
  u64 NumArgs = 1;

  EmitC(EXEC);
  EmitC(47);
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
ChangeCHR()
{
  EmitC(EXEC);
  EmitC(49);
  Expect("(");
  EmitOperand();
  Expect(",");
  GetString();
  EmitString(GlobalToken);
  Expect(")");
  Expect(";");
}

void
VCPutPCX()
{
  EmitC(EXEC);
  EmitC(51);
  Expect("(");
  GetString();
  EmitString(GlobalToken);
  Expect(",");
  EmitOperand();
  Expect(",");
  EmitOperand();
  Expect(")");
  Expect(";");
}

void
VCLoadPCX()
{
  EmitC(EXEC);
  EmitC(54);
  Expect("(");
  GetString();
  EmitString(GlobalToken);
  Expect(",");
  EmitOperand();
  Expect(")");
  Expect(";");
}

void
PlayFLI()
{
  EmitC(EXEC);
  EmitC(56);
  Expect("(");
  GetString();
  EmitString(GlobalToken);
  Expect(")");
  Expect(";");
}

void
VCText()
{
  EmitC(EXEC);
  EmitC(59);
  Expect("(");
  EmitOperand();
  Expect(",");
  EmitOperand();
  Expect(",");
  GetString();
  EmitString(GlobalToken);
  Expect(")");
  Expect(";");
}

void
Quit()
{
  EmitC(EXEC);
  EmitC(62);
  Expect("(");
  GetString();
  EmitString(GlobalToken);
  Expect(")");
  Expect(";");
}

void
VCCenterText()
{
  EmitC(EXEC);
  EmitC(63);
  Expect("(");
  EmitOperand();
  Expect(",");
  GetString();
  EmitString(GlobalToken);
  Expect(")");
  Expect(";");
}

void
Sys_DisplayPCX()
{
  EmitC(EXEC);
  EmitC(67);
  Expect("(");
  GetString();
  EmitString(GlobalToken);
  Expect(")");
  Expect(";");
}

void
NewGame()
{
  EmitC(EXEC);
  EmitC(70);
  Expect("(");
  GetString();
  EmitString(GlobalToken);
  Expect(")");
  Expect(";");
}

void
PartyMove()
{
  EmitC(EXEC);
  EmitC(73);
  Expect("(");
  GetString();
  EmitString(GlobalToken);
  Expect(")");
  Expect(";");
}

void
EntityMove()
{
  EmitC(EXEC);
  EmitC(74);
  Expect("(");
  EmitOperand();
  Expect(",");
  GetString();
  EmitString(GlobalToken);
  Expect(")");
  Expect(";");
}

void
VCLoadRaw()
{
  EmitC(EXEC);
  EmitC(79);
  Expect("(");
  GetString();
  EmitString(GlobalToken);
  Expect(",");
  EmitOperand();
  Expect(",");
  EmitOperand();
  Expect(",");
  EmitOperand();
  Expect(")");
  Expect(";");
}

void
OutputCode(u64 FunctionIndex)
{
  switch (FunctionIndex)
  {
    case 0: MapSwitch(); break;
    case 1: GenericFunc(2, 3); break; // warp
    case 2: GenericFunc(3, 1); break; // addcharacter
    case 3: GenericFunc(4, 1); break; // soundeffect
    case 4: GenericFunc(5, 1); break; // giveitem
    case 5: Text(); break;
    case 6: GenericFunc(7, 4); break; // alter F tile
    case 7: GenericFunc(8, 4); break; // alter B tile
    case 8: GenericFunc(9, 0); break; // fakebattle
    case 9: DoReturn(); break;
    case 10: PlayMusic(); break;
    case 11: GenericFunc(12, 0); break; // stopmusic
    case 12: GenericFunc(13, 0); break; // healall
    case 13: GenericFunc(14, 3); break; // alterparallax
    case 14: GenericFunc(15, 1); break; // fadein
    case 15: GenericFunc(16, 1); break; // fadeout
    case 16: GenericFunc(17, 1); break; // removecharacter
    case 17: Banner(); break;
    case 18: GenericFunc(19, 0); break; // enforceanimation
    case 19: GenericFunc(20, 0); break; // waitkeyup
    case 20: GenericFunc(21, 2); break; // destroyitem
    case 21: Prompt(); break;
    case 22: ChainEvent(); break;
    case 23: CallEvent(); break;
    case 24: GenericFunc(25, 2); break; // heal
    case 25: GenericFunc(26, 3); break; // earthquake
    case 26: GenericFunc(27, 0); break; // savemenu
    case 27: GenericFunc(28, 0); break; // enablesave
    case 28: GenericFunc(29, 0); break; // disablesave
    case 29: GenericFunc(30, 1); break; // revivechar
    case 30: GenericFunc(31, 2); break; // restoremp
    case 31: GenericFunc(32, 0); break; // redraw
    case 32: SText(); break;
    case 33: GenericFunc(34, 0); break; // disablemenu
    case 34: GenericFunc(35, 0); break; // enablemenu
    case 35: GenericFunc(36, 1); break; // wait
    case 36: GenericFunc(37, 2); break; // setface
    case 37: GenericFunc(38, 4); break; // mappalettegradient
    case 38: GenericFunc(39, 1); break; // boxfadeout
    case 39: GenericFunc(40, 1); break; // boxfadein
    case 40: GenericFunc(41, 1); break; // givegp
    case 41: GenericFunc(42, 1); break; // takegp
    case 42: GenericFunc(43, 3); break; // changezone
    case 43: GenericFunc(44, 2); break; // getitem
    case 44: GenericFunc(45, 2); break; // forceequip
    case 45: GenericFunc(46, 2); break; // givexp
    case 46: Shop(); break;
    case 47: GenericFunc(48, 5); break; // palettemorph
    case 48: ChangeCHR(); break;
    case 49: GenericFunc(50, 0); break; // readcontrols
    case 50: VCPutPCX(); break;
    case 51: GenericFunc(52, 1); break; // hooktimer
    case 52: GenericFunc(53, 1); break; // hookretrace
    case 53: VCLoadPCX(); break;
    case 54: GenericFunc(55, 5); break; // vcblitimage
    case 55: PlayFLI(); break;
    case 56: GenericFunc(57, 0); break; // vcclear
    case 57: GenericFunc(58, 4); break; // vcclearregion
    case 58: VCText(); break;
    case 59: GenericFunc(60, 5); break; // vctblitimage
    case 60: GenericFunc(61, 0); break; // exit
    case 61: Quit(); break;
    case 62: VCCenterText(); break;
    case 63: GenericFunc(64, 0); break; // resettimer
    case 64: GenericFunc(65, 3); break; // vcblittile
    case 65: GenericFunc(66, 0); break; // sys_clearscreen
    case 66: Sys_DisplayPCX(); break;
    case 67: GenericFunc(68, 0); break; // oldstartupmenu
    case 68: GenericFunc(69, 0); break; // vgadump
    case 69: NewGame(); break;
    case 70: GenericFunc(71, 0); break; // loadmenu
    case 71: GenericFunc(72, 1); break; // delay
    case 72: PartyMove(); break;
    case 73: EntityMove(); break;
    case 74: GenericFunc(75, 0); break; // autoon
    case 75: GenericFunc(76, 0); break; // autooff
    case 76: GenericFunc(77, 2); break; // entitymovescript
    case 77: GenericFunc(78, 3); break; // vctextnum
    case 78: VCLoadRaw(); break;

    case 79: GenericFunc(80, 4); break; /* -- ric:21/Apr/98 - VCBox       -- */
    case 80: GenericFunc(81, 4); break; /* -- ric:21/Apr/98 - VCCharName  -- */
    case 81: GenericFunc(82, 4); break; /* -- ric:21/Apr/98 - VCItemName  -- */
    case 82: GenericFunc(83, 4); break; /* -- ric:21/Apr/98 - VCItemDesc  -- */
    case 83: GenericFunc(84, 4); break; /* -- ric:22/Apr/98 - VCItemImage -- */
    case 84: GenericFunc(85, 4); break; /* -- ric:24/Apr/98 - VCATextNum  -- */
    case 85: GenericFunc(86, 4); break; /* -- ric:24/Apr/98 - VCSpc       -- */
    case 86: CallVCScript(87); break;   /* -- ric:25/Apr/98 - CallEffect  -- */
    case 87: CallVCScript(88); break;   /* -- ric:25/Apr/98 - CallScript  -- */
    case 88: GenericFunc(89, 5); break; /* -- NichG:??/??/?? - VCLine     -- */
    case 89: GenericFunc(90, 2); break; /* -- NichG:??/??/?? - GetMagic   -- */
    case 90: GenericFunc(91, 2); break; /* -- ric:03/May/98 - BindKey     -- */
    case 91: TextMenu(92); break;       /* -- ric:04/May/98 - TextMenu    -- */
    case 92: GenericFunc(93, 1); break; /* -- ric:03/May/98 - ItemMenu    -- */
    case 93: GenericFunc(94, 1); break; /* -- ric:03/May/98 - EquipMenu   -- */
    case 94: GenericFunc(95, 1); break; /* -- ric:03/May/98 - MagicMenu   -- */
    case 95: GenericFunc(96, 1); break; /* -- ric:03/May/98 - StatusScreen -- */
    case 96: GenericFunc(97, 4); break; /* -- ric:24/Apr/98 - VCCr2       -- */
    case 97:
      GenericFunc(98, 4);
      break; /* -- NichG\Ric: ??/??/?? - VCSpellName -- */
    case 98:
      GenericFunc(99, 4);
      break; /* -- NichG\Ric: ??/??/?? - VCSpellDesc -- */
    case 99:
      GenericFunc(100, 4);
      break;                      /* -- NichG\Ric: ??/??/?? - VCSpellImage -- */
    case 100: MagicShop(); break; /* -- NichG: ??/??/?? - MagicShop -- */
    case 101: VCTextBox(102); break; /* -- ric:04/May/98 - VCTextBox   -- */
    case 102:
      PlayVAS();
      break; /* -- NichG: ??/??/?? - PlayVAS   -- */
      //           case 103: GenericFunc(104,4); break;/* -- ric:04/May/98 -
      //           VCMagicImage -- */ case 104: GenericFunc(105,1); break;/* --
      //           xBig_D:05/May/98 - VCLayerWrite -- */

    default: err("*error* Internal error: Unknown std function.");
  }
}
