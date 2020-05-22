#define PKind(index)@ Flags[7000+index-1] @
#define PMaxHP(index)@ Flags[7010+index-1] @
#define PMaxMP(index)@ Flags[7020+index-1] @
#define PHP(index)@ Flags[7030+index-1] @
#define PMP(index)@ Flags[7040+index-1] @
#define PHit(index)@ Flags[7050+index-1] @
#define PDod(index)@ Flags[7060+index-1] @
#define PMag(index)@ Flags[7070+index-1] @
#define PMgr(index)@ Flags[7080+index-1] @
#define PRea(index)@ Flags[7090+index-1] @
#define PMbl(index)@ Flags[7100+index-1] @
#define PFer(index)@ Flags[7110+index-1] @
#define PAtk(index)@ Flags[7120+index-1] @
#define PDef(index)@ Flags[7130+index-1] @
#define PXP(index)@ Flags[7140+index-1] @
#define PGP(index)@ Flags[7150+index-1] @
#define PTime(index)@ Flags[7160+index-1] @
#define PCounterChance(index)@ Flags[7170+index-1] @
#define PCounter(index)@ Flags[7180+index-1] @
#define PEAttack(index)@ Flags[7190+index-1] @
#define PEAbsorb(index)@ Flags[7200+index-1] @
#define PENil(index)@ Flags[7210+index-1] @
#define PEHalf(index)@ Flags[7220+index-1] @
#define PEDbl(index)@ Flags[7230+index-1] @

#define PSAttack(index)@ Flags[7240+index-1] @
#define PSAbsorb(index)@ Flags[7250+index-1] @
#define PSNil(index)@ Flags[7260+index-1] @
#define PSHalf(index)@ Flags[7270+index-1] @
#define PSDbl(index)@ Flags[7280+index-1] @

#define PStatus(index)@ Flags[7290+index-1] @
#define PAI(index)@ Flags[7300+index-1] @

#define SPoison(index)@ Bit(7240+index-1,0) @
#define SBlind(index)@ Bit(7240+index-1,1) @
#define SConfused(index)@ Bit(7240+index-1,2) @
#define SParalyzed(index)@ Bit(7240+index-1,3) @
#define SCharmed(index)@ Bit(7240+index-1,4) @
#define SAsleep(index)@ Bit(7240+index-1,5) @
#define SStone(index)@ Bit(7240+index-1,5) @
#define SHaste(index)@ Bit(7240+index-1,6) @
#define SSlow(index)@ Bit(7240+index-1,7) @
#define SStop(index)@ Bit(7240+index-1,8) @
#define SUndead(index)@ Bit(7240+index-1,9) @
#define SMute(index)@ Bit(7240+index-1,10) @
#define SHumanoid(index)@ Bit(7240+inddex-1,11) @
#define SDefending(index)@ Bit(7240+index-1,12) @

#define IsEAtk(index,element)@ Bit(7190+index-1,element) @
#define IsEAbs(index,element)@ Bit(7200+index-1,element) @
#define IsENil(index,element)@ Bit(7210+index-1,element) @
#define IsEHalf(index,element)@ Bit(7220+index-1,element) @
#define IsEDbl(index,element)@ Bit(7230+index-1,element) @

#define IsSAtk(index,element)@ Bit(7240+index-1,element) @
#define IsSAbs(index,element)@ Bit(7250+index-1,element) @
#define IsSNil(index,element)@ Bit(7260+index-1,element) @
#define IsSHalf(index,element)@ Bit(7270+index-1,element) @
#define IsSDbl(index,element)@ Bit(7280+index-1,element) @

#define EHealing@ 0 @
#define EEarth@ 1 @
#define EWater@ 2 @
#define EWind@ 3 @
#define EFire@ 4 @
#define ELight@ 5 @
#define EDarkness@ 6 @
#define ELightening@ 7 @
#define ESonic@ 8 @
#define EDrain@ 9 @
#define EIron@ 10 @

#define DoneYet@ Flags[7500] @
#define TurnOver@ Flags[7501] @
#define MaxTime@ Flags[7502] @
#define RetVal@ Flags[7503] @
#define WepnSound@ Flags[7504] @
#define IsCtr@ Flags[7505] @

#define pi(v)@ PartyIndex(v)+1 @

#define Init@ 0 @
#define Main@ 1 @
#define CharTurn@ 2 @
#define MonTurn@ 3 @
#define SpellMenu@ 4 @
#define ItemMenu@ 5 @
#define ChooseTarget@ 6 @
#define Die@ 7 @
#define Numbers@ 8 @
#define Miss@ 9 @ 
#define DrawStat@ 10 @
#define DrawTimeBars@ 11 @
#define SetUpChars@ 12 @
#define SetUpEnemies@ 13 @
#define MakeMon@ 14 @
#define LoadWepn@ 15 @
#define Hook@ 16 @
#define BatFX(n)@ 16+n @
#define SpellFX(n)@ 19+n @
#define ItemFX(n)@ 27+n @

#define SavX@ Flags[6003] @
#define SavY@ Flags[6004] @
#define MapNum@ Flags[6005] @
#define BattleBG@ Flags[6008] @

#define FaceDown()@ 0 @
#define FaceUp()@ 1 @
#define FaceRight()@ 2 @
#define FaceLeft()@ 3 @

#define CHRFrame(Entity,Frame)@ (10240*Entity)+(1024*Frame) @
#define Arrow()@ 102400 @
#define Free()@ 102656 @
