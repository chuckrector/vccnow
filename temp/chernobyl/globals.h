// globals.h
// this has all the define's I want in my scripts
// all of these are just for reability's sake
// not to mention my own personal amusement

#define Outside@ Flags[5999] @
#define GlobalX@ Flags[6001] @
#define GlobalY@ Flags[6002] @
#define SavX@ Flags[6003] @
#define SavY@ Flags[6004] @
#define MapNum@ Flags[6005] @
#define MapMusicPos@ Flags[6006] @
#define CurMusic@ Flags[6007] @
#define BattleBG@ Flags[6008] @
#define SavPMode@ Flags[6009] @
#define SavPMult@ Flags[6010] @
#define SavPDiv@ Flags[6011] @
#define CurMusicPos@ Flags[6012] @
#define RunChance@ Flags[6013] @
#define Dark@ Flags[6014] @
#define Cheating@ Flags[6015] @

#define FaceDown()@ 0 @
#define FaceUp()@ 1 @
#define FaceRight()@ 2 @
#define FaceLeft()@ 3 @

#define pi(n)@ PartyIndex(n)+1 @

#define DrawTextBox(x,y,w,h)@ CallScript(3,x,y,w,h) @
#define EraseTextBox(x,y,w,h)@ CallScript(4,x,y,w,h) @

