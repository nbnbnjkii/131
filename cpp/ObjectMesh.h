#pragma once
#include "MeshGeometry.h"
namespace sandbox {
inline void appendObject(std::vector<MeshVertex>&v,const sandbox::Object&o,bool preview=false,bool valid=false){
 size_t begin=v.size();
 auto body=sandbox::World::bounds(o);
 body.material=o.type==3||o.type==4||o.type==5||o.type==6?sandbox::Wood:o.type==7?sandbox::Fabric:body.material;
 (void)preview;(void)valid; // Preview uses identical geometry/materials; hologram is a render pass.
 // Local-space furniture parts rotated by 90 degree increments.
 auto part=[&](float x,float y,float z,float w,float h,float d,float r,float g,float b,int material=-1){
  for(int i=0;i<o.rotation;i++){float old=x;x=z;z=-old;std::swap(w,d);}
  appendCube(v,{o.x+x,y+o.y,o.z+z,w,h,d,r,g,b,material>=0?material:o.type==4||o.type==5?sandbox::Wood:o.type==7?sandbox::Fabric:sandbox::Plain});
 };
 // Small smooth knobs, including normals and spherical UVs, rotated with the whole object.
 auto knob=[&](float x,float y,float z,float radius){
  constexpr int rings=6,sides=12;
  auto vertex=[&](int ring,int side){
   float a=3.14159265f*ring/rings,b=6.2831853f*side/sides;
   float nx=std::sin(a)*std::cos(b),ny=std::cos(a),nz=std::sin(a)*std::sin(b);
   float px=x+radius*nx,pz=z+radius*nz;
   for(int turn=0;turn<o.rotation;turn++){float old=px;px=pz;pz=-old;old=nx;nx=nz;nz=-old;}
   return MeshVertex{o.x+px,o.y+y+radius*ny,o.z+pz,.22f,.24f,.25f,nx,ny,nz,float(side)/sides,float(ring)/rings,float(Plain)};
  };
  for(int r=0;r<rings;r++)for(int s=0;s<sides;s++){
   for(auto vtx:{vertex(r,s),vertex(r+1,s),vertex(r+1,s+1),vertex(r,s),vertex(r+1,s+1),vertex(r,s+1)})v.push_back(vtx);
  }
 };
 if(o.type==3){
  part(0,1.15f,0,1.1f,2.3f,.045f,1,1,1,Wood);
  for(int side:{-1,1}){part(.4f,1.05f,side*.035f,.075f,.14f,.025f,.22f,.24f,.25f,Plain);knob(.4f,1.05f,side*.046f,.024f);}
 }else if(o.type==4||o.type==5){bool table=o.type==4;float w=table?1.8f:.6f,d=table?1.f:.65f,top=table?.82f:.46f;
  part(0,top,0,w,.15f,d,.54f,.4f,.27f);
  for(int a:{-1,1})for(int b:{-1,1})part(a*(w/2-.07f),top/2,b*(d/2-.07f),.1f,top,.1f,.43f,.31f,.2f);
  if(!table)part(0,.79f,-.27f,w,.6f,.12f,.54f,.4f,.27f);
 }else if(o.type==6){
  // Deep carcass with an open upper shelving bay and lower cupboard doors.
  part(0,.06f,0,1.4f,.12f,.8f,1,1,1,Wood);part(0,2.04f,0,1.4f,.12f,.8f,1,1,1,Wood);
  for(int side:{-1,1}){
   part(side*.65f,1.05f,0,.1f,1.86f,.8f,1,1,1,Wood);
   part(side*.315f,.53f,.35f,.61f,.8f,.06f,1,1,1,Wood);
   knob(side*.09f,.62f,.39f,.025f);
  }
  part(0,1.05f,-.37f,1.2f,1.86f,.06f,1,1,1,Wood);
  for(float y:{.96f,1.33f,1.69f})part(0,y,0,1.2f,.055f,.73f,1,1,1,Wood);
 }else if(o.type==2){
  for(int side:{-1,1}){part(side*.96f,1.2f,0,.08f,2.4f,.22f,.68f,.69f,.63f,sandbox::Wood);part(0,1.2f+side*1.16f,0,2,.08f,.22f,.68f,.69f,.63f,sandbox::Wood);}part(0,1.2f,0,.06f,2.4f,.2f,.68f,.69f,.63f,sandbox::Wood);part(0,1.2f,0,1.84f,2.24f,.05f,1,1,1,sandbox::Glass);
 }else if(o.type==7){part(0,.32f,0,2.3f,.6f,.95f,.33f,.4f,.36f);part(0,.78f,-.38f,2.3f,.44f,.2f,.33f,.4f,.36f);for(int a:{-1,1})part(a*1.04f,.65f,0,.22f,.4f,.95f,.33f,.4f,.36f);
 }else appendCube(v,body);
 int surface=World::fixedSurface(o.type);
 if(surface>=0)for(size_t i=begin;i<v.size();i++){if(v[i].layer!=Plain||(o.type!=3&&o.type!=6)){v[i].layer=float(surface);v[i].r=v[i].g=v[i].b=1;}}
}
}
