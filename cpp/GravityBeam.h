#pragma once
#include "MeshGeometry.h"
#include "CollisionMesh.h"
namespace sandbox {
inline std::array<physics::Vec3,2> gravityBeamEnds(const World&w,physics::Vec3 target){
 physics::Vec3 forward{-std::sin(w.yaw)*std::cos(w.pitch),std::sin(w.pitch),-std::cos(w.yaw)*std::cos(w.pitch)},right{std::cos(w.yaw),0,-std::sin(w.yaw)};
 auto up=physics::normalized(physics::cross(right,forward));
 return {physics::Vec3{w.x,w.y,w.z}+forward*.55f+right*.28f-up*.22f,target};
}
inline void appendGravityBeam(std::vector<MeshVertex>&v,const World&w,physics::Vec3 target){
 auto ends=gravityBeamEnds(w,target);auto direction=physics::normalized(ends[1]-ends[0]);
 auto side=physics::normalized(physics::cross(direction,std::abs(direction.y)>.95f?physics::Vec3{1,0,0}:physics::Vec3{0,1,0}));auto up=physics::cross(side,direction);
 auto point=[&](int end,int i){float angle=i*6.2831853f/8;return ends[end]+side*(std::cos(angle)*.012f)+up*(std::sin(angle)*.012f);};
 for(int i=0;i<8;i++){auto a=point(0,i),b=point(1,i),c=point(1,i+1),d=point(0,i+1);for(auto p:{a,b,c,a,c,d})v.push_back({p.x,p.y,p.z,.15f,1,.45f,0,1,0,0,0,float(Plain)});}
}
}
