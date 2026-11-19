#include "../../sys/android/app/src/main/cpp/ObjectDynamics.h"
#include <cassert>
#include <iostream>
int main(){
 physics::CollisionMesh mesh;mesh.addBox({-10,-.2f,-10},{10,0,10});mesh.build();
 // Read-only support checks must never depenetrate a sleeping, supported body.
 sandbox::World w;w.objects={{12,0,0,0}};w.objects[0].y=-.004f;w.objects[0].awake=false;
 sandbox::ObjectDynamics sim;auto original=w.objects[0];
 for(int i=0;i<2400;i++)sim.update(w,mesh,1.f/120,-1,{-20,-5,-20},{20,20,20});
 assert(w.objects[0].x==original.x&&w.objects[0].y==original.y&&w.objects[0].z==original.z);
 assert(!w.objects[0].awake&&w.objects[0].vy==0);
 for(int type:{3,8,12}){
  sandbox::Object o{type,0,0,0};o.y=.002f;o.vx=1;o.vy=-3;
  auto energy=[&](){sandbox::ContactInertia inertia(o);return o.vx*o.vx+o.vy*o.vy+o.vz*o.vz+inertia.ix*o.spinX*o.spinX+inertia.iz*o.spinZ*o.spinZ;};
  float before=energy();sandbox::solveCityContactVelocities(o,mesh);
  assert(energy()<=before+1e-4f);
  assert(std::abs(o.vy)<.03f);
 }
 std::cout<<"Sleeping support checks do not lift bodies; static contact impulses dissipate impact energy OK\n";
}
