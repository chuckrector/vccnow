#ifndef V1ASSETS_HPP
#define V1ASSETS_HPP

#include "types.hpp"
#include "util.hpp"

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
  u16 MapWidth;
  u16 MapHeight;
  u8 IsCompressed;
  u8 Padding[27];
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
  u8 Padding[3]; // 16
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

struct v1zone
{
  char Name[15];
  u8 Padding0;
  u16 CallEvent;
  u8 Percent;
  u8 Delay;
  u8 AcceptAdjacentActivation;
  char SaveDescription[30];
  u8 Padding1; // 52
};

struct v1chr_filename
{
  char Filename[13];
};

#define V1MAP_NUM_TILE_LAYERS 2

struct v1map
{
  v1map_header Header;
  v1zone Zones[128];                // 128*52=6656
  v1chr_filename ChrFilenames[100]; // 100*13=1300
};

int
VERGE1MapLayerSizeInBytes(int Width, int Height)
{
  return Width * Height * sizeof(u16);
}

int
VERGE1ObstructionLayerSizeInBytes(int Width, int Height)
{
  return Width * Height * sizeof(u8);
}

u32
VERGE1MapScriptsOffset(u8 *Data)
{
  u8 *Start = Data;

  v1map_header *H = (v1map_header *)Data;
  Data += sizeof(v1map_header);

  Data += VERGE1MapLayerSizeInBytes(H->MapWidth, H->MapHeight) *
          V1MAP_NUM_TILE_LAYERS;
  Data += VERGE1ObstructionLayerSizeInBytes(H->MapWidth, H->MapHeight);
  Data += sizeof(v1map::Zones);
  Data += sizeof(v1map::ChrFilenames);

  u32 NumEntities = *(u32 *)Data;
  Data += sizeof(u32);
  Data += sizeof(v1entity) * NumEntities;

  u8 NumMovementScripts = *Data++;
  u32 MovementScriptsBufferSize = *(u32 *)Data;
  Data += sizeof(u32);
  Data += sizeof(u32) * NumMovementScripts; // NOTE(aen): Offset lookups
  Data += MovementScriptsBufferSize;

  u64 ResultU64 = Data - Start;
  u32 Result = SafeTruncateU64(ResultU64);
  return (Result);
}

#endif // V1ASSETS_HPP
