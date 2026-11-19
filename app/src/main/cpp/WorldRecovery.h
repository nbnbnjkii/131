#pragma once
#include "PlayerPhysics.h"
#include "SandboxWorld.h"
#include <optional>
namespace sandbox {
inline bool needsPlayerRecovery(const World&w,const std::array<float,3>&lo,const std::array<float,3>&hi){
 return !std::isfinite(w.x)||!std::isfinite(w.y)||!std::isfinite(w.z)||w.y<lo[1]-4||w.x<lo[0]-.5f||w.x>hi[0]+.5f||w.z<lo[2]-.5f||w.z>hi[2]+.5f;
}
inline std::optional<physics::Vec3> recoveryPoint(const World&w,const physics::CollisionMesh&city,const physics::CollisionMesh&built){
 physics::PlayerController capsule;
 for(int ring=0;ring<7;ring++)for(int i=0;i<(ring?12:1);i++){
  float angle=i*6.2831853f/12;float x=-3.4f+ring*std::cos(angle),z=32.7f+ring*std::sin(angle);
  float floor=city.groundHeight(x,z,4,-10);if(!std::isfinite(floor))continue;
  float y=floor+1.701f;
  if(!capsule.overlaps(city,x,y,z)&&!capsule.overlaps(built,x,y,z))return physics::Vec3{x,y,z};
 }
 // If the road is completely built over, use the highest actual roof at the spawn instead of falling forever.
 float floor=std::max(city.groundHeight(-3.4f,32.7f,256,-10),built.groundHeight(-3.4f,32.7f,256,-10));
 if(std::isfinite(floor)&&!capsule.overlaps(city,-3.4f,floor+1.701f,32.7f)&&!capsule.overlaps(built,-3.4f,floor+1.701f,32.7f))return physics::Vec3{-3.4f,floor+1.701f,32.7f};
 (void)w;return {};
}
inline std::optional<Object> carRespawn(const World&w,Object parking,const physics::CollisionMesh&city,const std::array<float,3>&lo,const std::array<float,3>&hi){
 for(int ring=0;ring<4;ring++)for(int i=0;i<(ring?8:1);i++){
  Object car=parking;float angle=i*6.2831853f/8;car.x+=ring*4.5f*std::cos(angle);car.z+=ring*4.5f*std::sin(angle);
  auto b=World::bounds(car);float support=-1e6f;
  for(int sx:{-1,1})for(int sz:{-1,1}){
   float h=city.groundHeight(car.x+sx*b.w*.45f,car.z+sz*b.d*.45f,3,lo[1]-3);if(std::isfinite(h))support=std::max(support,h);
  }
  if(support<-1e5f)continue;
  car.y=support+.5f;b=World::bounds(car);
  if(b.x-b.w/2<lo[0]||b.x+b.w/2>hi[0]||b.z-b.d/2<lo[2]||b.z+b.d/2>hi[2])continue;
  if(std::abs(w.x-b.x)<b.w/2+.6f&&std::abs(w.z-b.z)<b.d/2+.6f)continue;
  if(city.intersectsBox({b.x,b.y,b.z},{b.w/2-.01f,b.h/2-.02f,b.d/2-.01f}))continue;
  bool blocked=false;for(const auto&o:w.objects){if(o.frozen)continue;auto other=World::bounds(o);if(World::overlap(b,other)&&std::abs(b.y-other.y)<(b.h+other.h)/2)blocked=true;}
  if(!blocked)return car;
 }return {};
}
}
