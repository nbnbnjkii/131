#include "../../sys/android/app/src/main/cpp/ToolInteraction.h"
#include <cassert>
int main(){
 sandbox::World w;w.walls.clear();w.wallSegments.clear();w.objects={{5,0,0,-4}};w.x=0;w.y=.7f;w.z=0;
 sandbox::ToolInteraction t;assert(!t.grab(w,nullptr));
 t.select(w,sandbox::HandTool::GRAVITY_GUN);assert(t.grab(w,nullptr));assert(t.holding());
 t.pull(w,.1f);t.rotate(w);assert(w.objects[0].rotation==1);
 t.select(w,sandbox::HandTool::HAND_EMPTY);assert(!t.holding()&&w.objects[0].rotation==0&&w.objects.size()==1);
 physics::CollisionMesh wall;wall.addBox({-3,0,-2},{3,3,-1.9f});wall.build();assert(!t.grab(w,&wall));
 t.select(w,sandbox::HandTool::GRAVITY_GUN);assert(!t.grab(w,&wall));assert(t.grab(w,nullptr));
 for(int i=0;i<60;i++)t.pull(w,1.f/60);
 assert(t.release(w));assert(!t.holding()&&w.objects.size()==1);
 assert(w.objects[0].z>-4);
 assert(!t.release(w));
}
