#ifndef V1MAP_HPP
#define V1MAP_HPP

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

#endif // V1MAP_HPP