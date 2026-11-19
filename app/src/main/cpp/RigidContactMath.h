#pragma once
#include "SandboxWorld.h"
#include "CollisionMesh.h"
namespace sandbox {
// Generalized axes of Rz(tiltZ)*Rx(tiltX); yaw remains catalogue-controlled.
struct ContactInertia {
 physics::Vec3 axisX;float ix,iz;
 explicit ContactInertia(const Object&o){
  auto b=World::bounds(o);axisX={std::cos(o.tiltZ),std::sin(o.tiltZ),0};
  ix=(b.h*b.h+b.d*b.d)/12;
  float iy=(b.w*b.w+b.d*b.d)/12,localZ=(b.w*b.w+b.h*b.h)/12;
  iz=iy*std::sin(o.tiltX)*std::sin(o.tiltX)+localZ*std::cos(o.tiltX)*std::cos(o.tiltX);
 }
 physics::Vec3 velocity(const Object&o,physics::Vec3 r)const{
  return physics::Vec3{o.vx,o.vy,o.vz}+physics::cross(axisX*o.spinX+physics::Vec3{0,0,o.spinZ},r);
 }
 float denominator(physics::Vec3 r,physics::Vec3 direction)const{
  auto torque=physics::cross(r,direction);float tx=physics::dot(torque,axisX);
  return 1+tx*tx/ix+torque.z*torque.z/iz;
 }
 void apply(Object&o,physics::Vec3 r,physics::Vec3 impulse)const{
  o.vx+=impulse.x;o.vy+=impulse.y;o.vz+=impulse.z;
  auto torque=physics::cross(r,impulse);o.spinX+=physics::dot(torque,axisX)/ix;o.spinZ+=torque.z/iz;
 }
};
inline float resolveStaticImpact(Object&o,physics::Vec3 point,physics::Vec3 normal,float restitution){
 ContactInertia inertia(o);auto b=World::bounds(o);auto r=point-physics::Vec3{b.x,b.y,b.z};
 float incoming=physics::dot(inertia.velocity(o,r),normal);
 if(incoming>=0)return 0;
 float impulse=-(1+(incoming< -2?restitution:0))*incoming/inertia.denominator(r,normal);
 inertia.apply(o,r,normal*impulse);return impulse;
}
}
