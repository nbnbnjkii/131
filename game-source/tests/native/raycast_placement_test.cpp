#include "../../sys/android/app/src/main/cpp/RaycastPlacement.h"
#include "../../sys/android/app/src/main/cpp/PlayerPhysics.h"
#include <cassert>
#include <cstdio>
#include <iostream>
using namespace physics;
int main(){
 {
  sandbox::World cars;cars.objects.clear();cars.x=0;cars.y=1.7f;cars.z=0;cars.pitch=-.4f;cars.selected=8;
  physics::CollisionMesh road,empty;road.addBox({-20,-.2f,-20},{20,0,20});road.build();empty.build();
  std::array<float,3> low{-20,-1,-20},high{20,10,20};
  auto car=sandbox::RaycastPlacement::preview(cars,road,empty,low,high);assert(car&&car->type==8&&!car->anchored);
  cars.objects.push_back(*car);assert(!sandbox::RaycastPlacement::valid(cars,*car,road,empty,low,high));
  sandbox::Object metal{11,0,6,-5};metal.surface=sandbox::Rust;cars.objects.push_back(metal);
  const char*save="/tmp/ekron-car-metal-save.txt";assert(cars.save(save));sandbox::World loaded;assert(loaded.load(save));assert(loaded.objects.size()==2&&loaded.objects[0].type==8&&loaded.objects[1].type==11);std::remove(save);
 }

 sandbox::World w;w.scenery.clear();w.walls.clear();w.wallSegments.clear();w.x=0;w.y=1.7f;w.z=0;w.selected=5;
 CollisionMesh city,built;city.addBox({-10,-.2f,-10},{10,0,10});city.addBox({-5,0,-6},{5,6,-5});city.build();built.build();
 std::array<float,3> lo{-10,-1,-10},hi{10,8,10};
 auto place=sandbox::RaycastPlacement::preview(w,city,built,lo,hi);assert(place);
 auto box=sandbox::World::bounds(*place);assert(std::abs(box.z-box.d/2+5)<.02f);assert(place->y>.5f); // Flush to wall at actual eye ray height, not y=0.
 assert(!city.intersectsBox({box.x,box.y,box.z},{box.w/2-.003f,box.h/2-.003f,box.d/2-.003f}));
 w.objects.push_back(*place);assert(!sandbox::RaycastPlacement::preview(w,city,built,lo,hi));w.objects.clear();
 w.pitch=1.2f;assert(!sandbox::RaycastPlacement::preview(w,city,built,lo,hi)); // Sky is not a target.
 w.pitch=-.4f;place=sandbox::RaycastPlacement::preview(w,city,built,lo,hi);assert(place&&std::abs(place->y-.5005f)<.001f);
 w.pitch=-1.2f;assert(sandbox::RaycastPlacement::preview(w,city,built,lo,hi)); // Indoor reach is allowed if the actual player capsule stays clear.
 w.pitch=0;w.z=-3.6f;assert(sandbox::RaycastPlacement::preview(w,city,built,lo,hi));
 w.z=-4.7f;assert(!sandbox::RaycastPlacement::preview(w,city,built,lo,hi));
 // v1 migration and persistent wall-mounted height.
 const char*path="/tmp/ekron-placement-save.txt";
 {std::ofstream f(path);f<<"EKRON_SANDBOX_1\n1\n5 0 0 -4\n";}
 assert(w.load(path)&&w.objects[0].y==0);w.objects[0].y=2.35f;assert(w.save(path));sandbox::World restored;assert(restored.load(path));assert(std::abs(restored.objects[0].y-2.35f)<.001f);std::remove(path);
 // Boundary is a constraint, not a box with a climbable roof. Y remains grounded on real floor.
 CollisionMesh floor;floor.addBox({-20,-1,-20},{20,0,20});floor.build();PlayerController player;player.setBoundary(-2,2,-2,2);
 float x=0,y=1.7f,z=0;player.reset(x,y,z);
 for(int i=0;i<400;i++)player.update(floor,1,1,0,1.f/120,false,x,y,z);
 assert(x<=1.751f&&z>=-1.751f&&std::abs(y-1.7f)<.01f);
 player.jump();for(int i=0;i<180;i++)player.update(floor,1,1,0,1.f/120,false,x,y,z);
 assert(x<=1.751f&&z>=-1.751f&&std::abs(y-1.7f)<.01f);
 std::cout<<"Raycast floor/wall/sky, minimum distance, persisted Y and unclimbable boundary OK\n";
}
