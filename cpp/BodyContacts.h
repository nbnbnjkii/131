#pragma once
#include "ObjectOverlap.h"
namespace sandbox {
// The same oriented solid is used by overlap, support rays and swept contacts.
// Frozen/anchored bodies remain solid; only their inverse mass is zero.
inline physics::Vec3 bodyContactPoint(const Object&o,float x,float y,float z){
 Object upright=o;upright.tiltX=upright.tiltZ=0;
 auto p=objectCorner(upright,x,y,z);
 if(o.type==3&&std::abs(o.doorAngle)>.0001f){
  float hx=-.55f,hz=0;for(int i=0;i<o.rotation;i++){float old=hx;hx=hz;hz=-old;}
  float px=p.x-o.x-hx,pz=p.z-o.z-hz,c=std::cos(o.doorAngle),s=std::sin(o.doorAngle);
  p.x=o.x+hx+c*px+s*pz;p.z=o.z+hz-s*px+c*pz;
 }
 auto b=World::bounds(o);auto d=p-physics::Vec3{b.x,b.y,b.z};
 float py=std::cos(o.tiltX)*d.y-std::sin(o.tiltX)*d.z,pz=std::sin(o.tiltX)*d.y+std::cos(o.tiltX)*d.z;
 return {b.x+std::cos(o.tiltZ)*d.x-std::sin(o.tiltZ)*py,b.y+std::sin(o.tiltZ)*d.x+std::cos(o.tiltZ)*py,b.z+pz};
}
inline void appendBodySolid(physics::CollisionMesh&mesh,const Object&o){
 physics::Vec3 v[8];int n=0;
 for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f})v[n++]=bodyContactPoint(o,x,y,z);
 constexpr int faces[12][3]={{0,1,3},{0,3,2},{4,6,7},{4,7,5},{0,4,5},{0,5,1},{2,3,7},{2,7,6},{0,2,6},{0,6,4},{1,5,7},{1,7,3}};
 for(const auto&f:faces)mesh.add({v[f[0]],v[f[1]],v[f[2]]});
}
// SAT includes full containment, which a triangle-surface overlap misses
// when a thin door fits entirely inside a larger chassis collider.
inline bool bodiesOverlap(const Object&a,const Object&b,float margin=.008f){
 physics::Vec3 pa[8],pb[8];int n=0;
 for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f}){pa[n]=bodyContactPoint(a,x,y,z);pb[n++]=bodyContactPoint(b,x,y,z);}
 physics::Vec3 ax[3]={physics::normalized(pa[4]-pa[0]),physics::normalized(pa[2]-pa[0]),physics::normalized(pa[1]-pa[0])};
 physics::Vec3 bx[3]={physics::normalized(pb[4]-pb[0]),physics::normalized(pb[2]-pb[0]),physics::normalized(pb[1]-pb[0])};
 auto separated=[&](physics::Vec3 axis){
  if(physics::length(axis)<1e-5f)return false;
  axis=physics::normalized(axis);float al=1e9f,ah=-1e9f,bl=1e9f,bh=-1e9f;
  for(int i=0;i<8;i++){float av=physics::dot(pa[i],axis),bv=physics::dot(pb[i],axis);al=std::min(al,av);ah=std::max(ah,av);bl=std::min(bl,bv);bh=std::max(bh,bv);}
  return std::min(ah,bh)-std::max(al,bl)<=margin;
 };
 for(auto axis:ax)if(separated(axis))return false;
 for(auto axis:bx)if(separated(axis))return false;
 for(auto x:ax)for(auto y:bx)if(separated(physics::cross(x,y)))return false;
 return true;
}
inline bool overlapsBodies(const World&w,Object candidate,int index,int held,physics::Vec3 centre){
 auto b=World::bounds(candidate);candidate.x=centre.x;candidate.z=centre.z;candidate.y=centre.y-b.h/2;
 for(size_t i=0;i<w.objects.size();i++)if(int(i)!=index&&int(i)!=held&&!w.objects[i].destroyed){
  auto other=World::bounds(w.objects[i]);float reach=(b.w+b.h+b.d+other.w+other.h+other.d)*.5f;
  if(std::abs(candidate.x-other.x)>reach||std::abs(centre.y-other.y)>reach||std::abs(candidate.z-other.z)>reach)continue;
  if(bodiesOverlap(candidate,w.objects[i]))return true;
 }
 return false;
}
struct ContactMaterial {
 float staticFriction=.7f,dynamicFriction=.5f,restitution=.1f;
};
inline ContactMaterial contactMaterial(const Object&o){
 if(o.type==8)return {.9f,.7f,.05f};
 if(o.type==11)return {.6f,.4f,.1f};
 return {};
}
inline void applyContactFriction(Object&o,float dt,float gravity=18.f){
 auto material=contactMaterial(o);float speed=std::hypot(o.vx,o.vz);
 float stop=material.staticFriction*gravity*dt;
 if(speed<=stop){o.vx=o.vz=0;return;}
 float scale=std::max(0.f,1-material.dynamicFriction*gravity*dt/speed);
 o.vx*=scale;o.vz*=scale;
}
}
