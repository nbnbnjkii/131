#include "../../sys/android/app/src/main/cpp/SceneCollision.h"
#include "../../sys/android/app/src/main/cpp/PlayerPhysics.h"
#include <cassert>
#include <iostream>
int main(){
 city::Scene s;city::Material m;m.alphaMode=2;m.image=0;m.wrapS=m.wrapT=33071;s.materials.push_back(m);
 city::Image image;image.width=16;image.height=16;image.rgba.assign(16*16*4,255);
 // Texture atlas: lower half fence, upper half completely empty (not an invisible wall).
 for(int y=8;y<16;y++)for(int x=0;x<16;x++)image.rgba[(y*16+x)*4+3]=0;
 s.images.push_back(image);
 city::Batch batch;batch.material=0;
 batch.vertices={{-2,0,0,0,0,1,0,0},{2,0,0,0,0,1,1,0},{2,4,0,0,0,1,1,1},
 {-2,0,0,0,0,1,0,0},{2,4,0,0,0,1,1,1},{-2,4,0,0,0,1,0,1}};
 s.batches.push_back(batch);city::buildSceneCollision(s);assert(s.collision->size()>0);
 physics::PlayerController p;assert(p.overlaps(*s.collision,0,1.7f,0));
 assert(!p.overlaps(*s.collision,0,4.f,0));
 float x=0,y=1.7f,z=1;p.reset(x,y,z);
 for(int i=0;i<25;i++)p.update(*s.collision,0,1,0,1.f/120,false,x,y,z);
 assert(z>.24f);
 // Both sides of a zero-thickness alpha fence are solid.
 x=0;y=1.7f;z=-1;p.reset(x,y,z);
 for(int i=0;i<25;i++)p.update(*s.collision,0,-1,0,1.f/120,false,x,y,z);
 assert(z<-.24f);
 s.materials[0].alphaMode=1;city::buildSceneCollision(s);assert(p.overlaps(*s.collision,0,1.7f,0));
 std::cout<<"BLEND/MASK fence collision, two-sided blocking and transparent opening OK\n";
}
