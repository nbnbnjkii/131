#pragma once
#include "MeshGeometry.h"
#include "CollisionMesh.h"
namespace sandbox {
// Small painted pixel-letter easter egg, projected onto a real facade near spawn.
inline void appendCityGraffiti(std::vector<MeshVertex>&v,const physics::CollisionMesh&city){
 physics::CollisionMesh::RayHit hit;
 if(!city.raycast({-3.4f,4.5f,28},{-1,0,0},25,hit)||std::abs(hit.normal.x)<.8f)return;
 // К А Р А С Ь [space] Г Е Й
 const char*glyphs[]={"10010101001100010100100101001010010","01110100011000111111100011000110001","11110100011000111110100001000010000","01110100011000111111100011000110001","01111100001000010000100001000001111","10000100001000011110100011000111110","00000000000000000000000000000000000","11111100001000010000100001000010000","11111100001000011110100001000011111","01010100011001110101110011000110001"};
 constexpr float pixel=.055f;
 auto origin=hit.position+hit.normal*.012f;
 for(int letter=0;letter<10;letter++)for(int row=0;row<7;row++)for(int col=0;col<5;col++)if(glyphs[letter][row*5+col]=='1'){
  float y=origin.y+(6-row)*pixel,z=origin.z-(letter*6+col-30)*pixel;
  appendCube(v,{origin.x,y,z,.008f,pixel*.88f,pixel*.88f,.8f,.78f,.65f,Plain});
 }
}
}
