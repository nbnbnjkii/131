#include "../../sys/android/app/src/main/cpp/ObjectDynamics.h"
#include "../../sys/android/app/src/main/cpp/RaycastPlacement.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace sandbox;using namespace physics;
 CollisionMesh floor;floor.addBox({-20,-.002f,-20},{20,0,20});floor.build();
 Object before{3,0,0,0};before.y=1;before.tiltX=.9f;Object after=before;after.y=-1;after.tiltX=1.05f;
 assert(sweepObjectFloor(before,after,floor));assert(after.vy==0);for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f})assert(objectCorner(after,x,y,z).y>-.01f);
 std::array<float,3> lo{-20,-3,-20},hi{20,15,20};ObjectDynamics sim;World w;
 for(int rotation=0;rotation<4;rotation++){
  w.objects={{3,rotation,0,0}};w.objects[0].y=5;w.objects[0].vy=-25;w.objects[0].tiltX=1.2f;w.objects[0].spinZ=3;
  for(int i=0;i<1000;i++){sim.update(w,floor,1.f/60,-1,lo,hi);assert(w.objects.size()==1);for(float x:{-1.f,1.f})for(float y:{-1.f,1.f})for(float z:{-1.f,1.f})assert(objectCorner(w.objects[0],x,y,z).y>-.025f);}
 }
 w.objects={{3,0,0,0}};w.objects[0].y=-6;sim.update(w,floor,1.f/60,-1,lo,hi);assert(w.objects.size()==1&&w.objects[0].y>=0&&w.objects[0].floorRecoveryUsed);
 w.objects[0].y=-6;sim.update(w,floor,1.f/60,-1,lo,hi);assert(w.objects[0].y< -5); // never a repeated rescue loop
 CollisionMesh empty;empty.build();w.objects={{3,0,0,0}};w.objects[0].y=-6;sim.update(w,empty,1.f/60,-1,lo,hi);assert(w.objects[0].y< -5); // no invented floor
 w.objects.clear();w.selected=3;w.x=0;w.y=1.7f;w.z=4;w.pitch=-.6f;
 auto preview=RaycastPlacement::preview(w,floor,empty,lo,hi);assert(preview&&preview->y>.49f&&!preview->anchored);auto placed=RaycastPlacement::commitPreview(w,preview,floor,empty,lo,hi);assert(placed&&placed->y==preview->y);
 std::cout<<"Swept rotating thin door, subfloor rescue without loop, real-surface requirement and lifted spawn OK\n";
}
