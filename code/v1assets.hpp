#ifndef V1ASSETS_HPP
#define V1ASSETS_HPP

#include "types.hpp"

struct v1map_header
{
  u8 Version;
  char VspFilename[13];
  char MusicFilename[13];
  u8 LayerControlFlag;
  u8 ParallaxMul;
  u8 ParallaxDiv;
  char Description[30];
  u8 ShowName;
  u8 SaveFlag;
  u16 StartX;
  u16 StartY;
  u8 Hide;
  u8 Warp;
  u16 Width;
  u16 Height;
  u8 IsCompressed;
  char Padding[27];
};

struct v1entity
{
  u16 WorldPixelX;
  u16 WorldPixelY;
  u8 Facing;
  u8 Moving;
  u8 MovementCounter;
  u8 FrameCounter;
  u8 SpecialFrame;
  u8 CharacterIndex;
  u8 MovementPatternCode;
  u8 CanBeActivated;
  u8 CannotBeObstructed;
  char Padding[3]; // 16
  u32 ActivationScript;
  u32 MovementScript; // 16+8=24
  u8 Speed;
  u8 SpeedCounter;
  u16 Step;
  u16 Delay;
  u16 Data1;
  u16 Data2;
  u16 Data3;
  u16 Data4;
  u16 DelayCounter;
  u16 WasActivated;
  u16 BoundingBoxX1;
  u16 BoundingBoxY1;
  u16 BoundingBoxX2;
  u16 BoundingBoxY2; // 24+26=50
  u8 CurrentCommandCode;
  u8 CurrentCommandArg;
  u32 ScriptParsingOffset;
  u8 Face;
  u8 Chasing;
  u8 ChaseSpeed;
  u8 ChaseDistance; // 50+10=60
  u16 CurrentTileX;
  u16 CurrentTileY;
  u32 FutureExpansion;  // 60+8=68
  char Description[20]; // 68+20=88
};

/*
module.exports = {
  version: T.u8,
  vspFilename: T.stringFixed(13),
  musicFilename: T.stringFixed(13),
  layerControlFlag: T.u8,
  parallax: {
    mul: T.u8,
    div: T.u8,
  },
  description: T.stringFixed(30),
  showName: T.u8,
  saveFlag: T.u8,
  startX: T.u16,
  startY: T.u16,
  hide: T.u8,
  warp: T.u8,
  width: T.u16,
  height: T.u16,
  isCompressed: T.u8, // "not yet supported" in v1 source
  padding: T.list(T.u8, 27),
  layers: T.list(T.list(T.u16, ({record}) => record.width * record.height), 2),
  obstructionLayer: T.list(T.u8, ({record}) => record.width * record.height),
  zone: T.list(V1_ZONE, 128),
  characterFilenames: T.list(T.stringFixed(13), 100),
  entityCount: T.u32,
  entities: T.list(V1_ENTITY, ({record}) => record.entityCount),
  movementScriptCount: T.u8,
  movementScriptBufferSize: T.u32,
  movementScriptOffsets: T.list(T.u32, ({record}) =>
record.movementScriptCount), movementScriptBuffer: T.list(T.u8, ({record}) =>
record.movementScriptBufferSize), scriptCount: T.u32, scriptOffsets:
T.list(T.u32, ({record}) => record.scriptCount), scriptBuffer: T.list(T.u8,
({reader}) => reader.remaining),
}
*/

#endif // V1ASSETS_HPP