#include "../../sys/android/app/src/main/cpp/RaycastPlacement.h"
#include "../../sys/android/app/src/main/cpp/ObjectDynamics.h"
#include <cassert>
#include <iostream>
using namespace physics;
int main(){
 sandbox::World w;w.scenery.clear();w.walls.clear();w.wallSegments.clear();w.x=0;w.y=1.7f;w.z=0;
 CollisionMesh room,built;room.addBox({-3,-.2f,-5},{3,0,3});room.addBox({-3,3,-5},{3,3.2f,3});room.addBox({-3,0,-4.2f},{3,3,-4});room.addBox({-3.2f,0,-5},{-3,3,3});room.addBox({3,0,-5},{3.2f,3,3});room.build();built.build();
 std::array<float,3> lo{-10,-1,-10},hi{10,10,10};
 // A 2.8m wall fits in a 3m room by sliding down, while staying flush to its support wall.
 w.surface=sandbox::Wood;w.selected=0;auto wall=sandbox::RaycastPlacement::preview(w,room,built,lo,hi);assert(wall&&wall->surface==sandbox::Concrete);
 auto b=sandbox::World::bounds(*wall);assert(wall->y>=-.001f&&wall->y+b.h<=3.001f);
 assert(std::abs(b.z-b.d/2+4)<.001f);
 for(int type:{9,10}){w.selected=type;auto block=sandbox::RaycastPlacement::preview(w,room,built,lo,hi);assert(block&&block->surface==sandbox::World::fixedSurface(type));}
 // A floor slab aimed at a vertical wall stays 3 x 3 x .18, in every yaw.
 w.selected=1;
 for(int rotation=0;rotation<4;rotation++){
  w.rotation=rotation;auto slab=sandbox::RaycastPlacement::preview(w,room,built,lo,hi);assert(slab&&slab->anchored);
  auto flat=sandbox::World::bounds(*slab);assert(flat.h==.18f&&flat.w==3&&flat.d==3);
  assert(slab->tiltX==0&&slab->tiltZ==0&&std::abs(flat.z-flat.d/2+4)<.001f);
 }
 w.rotation=0;
 // Commit the shown transform even if the look direction moved slightly after the frame.
 w.selected=1;w.rotation=0;auto shown=sandbox::RaycastPlacement::preview(w,room,built,lo,hi);assert(shown);
 w.yaw=.05f;auto placed=sandbox::RaycastPlacement::commitPreview(w,shown,room,built,lo,hi);assert(placed&&placed->x==shown->x&&placed->y==shown->y&&placed->z==shown->z&&placed->rotation==shown->rotation);
 w.rotation=1;assert(!sandbox::RaycastPlacement::commitPreview(w,shown,room,built,lo,hi));w.rotation=0;w.yaw=0;
 // Ceiling attachment at less than two metres works; it does not drop when dynamics run.
 w.selected=1;w.pitch=1.3f;auto ceiling=sandbox::RaycastPlacement::preview(w,room,built,lo,hi);assert(ceiling&&ceiling->anchored);
 assert(std::abs(ceiling->y+.18f-3)<.001f);w.objects.push_back(*ceiling);
 sandbox::ObjectDynamics physics;for(int i=0;i<300;i++)physics.update(w,room,1.f/120,-1,lo,hi);
 assert(w.objects[0].y==ceiling->y);w.objects.clear();
 // Attach to the top of another block, using its actual volume, not city ground.
 sandbox::Object base{1,0,0,-3};w.objects.push_back(base);auto bb=sandbox::World::bounds(base);
 built.addBox({bb.x-bb.w/2,bb.y-bb.h/2,bb.z-bb.d/2},{bb.x+bb.w/2,bb.y+bb.h/2,bb.z+bb.d/2});built.build();
 w.selected=5;w.pitch=-std::atan2(1.7f-.18f,3.f);auto stacked=sandbox::RaycastPlacement::preview(w,room,built,lo,hi);assert(stacked);
 assert(std::abs(stacked->y-.68f)<.001f);w.objects.clear();
 // Upper floor never snaps to the ground floor after placing a construction block.
 CollisionMesh floors,empty;floors.addBox({-8,-.2f,-8},{8,0,8});floors.addBox({-8,3.8f,-8},{8,4,8});floors.addBox({-8,7,-8},{8,7.2f,8});floors.build();empty.build();
 w.y=5.7f;w.selected=1;w.pitch=-.65f;auto upstairs=sandbox::RaycastPlacement::preview(w,floors,empty,lo,hi);assert(upstairs);
 assert(std::abs(upstairs->y-4.5f)<.001f);w.objects.push_back(*upstairs);
 for(int i=0;i<300;i++)physics.update(w,floors,1.f/120,-1,lo,hi);
 assert(w.objects[0].y>3.99f);
 const char*path="/tmp/ekron-material-save.txt";
 assert(w.save(path));sandbox::World restored;assert(restored.load(path));assert(restored.objects[0].surface==sandbox::Concrete);std::remove(path);
 // Too tall still means too tall: do not tunnel into a low ceiling to allow construction.
 CollisionMesh low;low.addBox({-4,-.2f,-5},{4,0,3});low.addBox({-4,2.2f,-5},{4,2.4f,3});low.addBox({-4,0,-4.2f},{4,2.4f,-4});low.build();
 w.objects.clear();w.y=1.7f;w.pitch=0;w.selected=0;assert(!sandbox::RaycastPlacement::preview(w,low,empty,lo,hi));
 std::cout<<"Indoor wall fit, ceiling attachment, block stacking, upper-floor stability and low-ceiling rejection OK\n";
}
