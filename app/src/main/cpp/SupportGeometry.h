#pragma once
#include "BodyContacts.h"
#include <tuple>
#include "RigidContactMath.h"
namespace sandbox {
inline bool lowerBodySample(const Object&o,float x,float y,float z){
 return (std::abs(x)>.5f&&x*std::sin(o.tiltZ)<-.05f)
     ||(std::abs(y)>.5f&&y*std::cos(o.tiltZ)*std::cos(o.tiltX)<-.05f)
     ||(std::abs(z)>.5f&&-z*std::cos(o.tiltZ)*std::sin(o.tiltX)<-.05f);
}
inline bool comInSupport(std::vector<physics::Vec3>points,physics::Vec3 centre){
 if(points.empty())return false;
 std::sort(points.begin(),points.end(),[](auto a,auto b){return std::tie(a.x,a.z)<std::tie(b.x,b.z);});
 auto cross=[](auto a,auto b,auto c){return (b.x-a.x)*(c.z-a.z)-(b.z-a.z)*(c.x-a.x);};
 std::vector<physics::Vec3> hull;
 for(auto p:points){while(hull.size()>1&&cross(hull[hull.size()-2],hull.back(),p)<=0)hull.pop_back();hull.push_back(p);}
 size_t lower=hull.size();
 for(size_t i=points.size()-1;i-->0;){auto p=points[i];while(hull.size()>lower&&cross(hull[hull.size()-2],hull.back(),p)<=0)hull.pop_back();hull.push_back(p);}
 if(hull.size()>1)hull.pop_back();
 if(hull.size()==1)return std::hypot(centre.x-hull[0].x,centre.z-hull[0].z)<.015f;
 if(hull.size()==2){auto a=hull[0],d=hull[1]-a;float t=std::clamp(((centre.x-a.x)*d.x+(centre.z-a.z)*d.z)/std::max(1e-9f,d.x*d.x+d.z*d.z),0.f,1.f);return std::hypot(centre.x-a.x-d.x*t,centre.z-a.z-d.z*t)<.015f;}
 for(size_t i=0;i<hull.size();i++){auto a=hull[i],b=hull[(i+1)%hull.size()];if(cross(a,b,centre)<-.015f*std::hypot(b.x-a.x,b.z-a.z))return false;}
 return true;
}
// Distributed Coulomb friction generates the missing contact torque (including
// spin about a horizontal panel's vertical axis). It is bounded by mu*N*dt,
// not an angular multiplier or an imposed resting angle.
inline void applyPatchFriction(Object&o,const std::vector<physics::Vec3>&points,float dt){
 if(points.empty())return;
 auto b=World::bounds(o);ContactInertia inertia(o);std::vector<physics::Vec3>accumulated(points.size());
 auto material=contactMaterial(o);float budget=18.f*dt/points.size();
 for(int pass=0;pass<8;pass++)for(size_t k=0;k<points.size();k++){
  size_t i=pass%2?points.size()-1-k:k;auto r=points[i]-physics::Vec3{b.x,b.y,b.z};
  auto velocity=inertia.velocity(o,r);velocity.y=0;float speed=physics::length(velocity);if(speed<1e-7f)continue;
  auto direction=velocity*(1/speed);auto candidate=accumulated[i]-direction*(speed/inertia.denominator(r,direction));float size=physics::length(candidate);
  if(size>material.staticFriction*budget&&size>1e-9f)candidate=candidate*(material.dynamicFriction*budget/size);
  inertia.apply(o,r,candidate-accumulated[i]);accumulated[i]=candidate;
 }
}

}
