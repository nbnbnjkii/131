#include "../../sys/android/app/src/main/cpp/BuildingCollision.h"
#include <cassert>
#include <iostream>
using P=city::CollisionPoint;
void quad(std::vector<sandbox::WallSegment>&out,P a,P b,P c,P d,bool opaque=true){city::addBuildingWall(out,a,b,c,opaque);city::addBuildingWall(out,a,c,d,opaque);}
int main(){
 sandbox::World world;world.walls.clear();world.objects.clear();
 // Full-height facade panels leave a 1.2 m real doorway; lintel must not fill it.
 quad(world.wallSegments,{-3,0,0},{-.6f,0,0},{-.6f,3,0},{-3,3,0});
 quad(world.wallSegments,{.6f,0,0},{3,0,0},{3,3,0},{.6f,3,0});
 quad(world.wallSegments,{-3,2.1f,0},{3,2.1f,0},{3,3,0},{-3,3,0});
 assert(!world.wallSegments.empty());
 assert(world.blocked({-2,0,0,.6f,2,.6f,0,0,0}));
 assert(!world.blocked({0,0,0,.6f,2,.6f,0,0,0}));
 world.x=0;world.z=2;for(int i=0;i<60;i++)world.move(0,1,.016f);assert(world.z< -2);
 world.x=-2;world.z=2;for(int i=0;i<60;i++)world.move(0,1,.016f);assert(world.z>.3f);
 std::vector<sandbox::WallSegment> ignored;
 quad(ignored,{-4,0,0},{4,0,0},{4,2.5f,0},{-4,2.5f,0},false); // alpha fence
 quad(ignored,{-2,0,0},{2,0,0},{2,1,0},{-2,1,0}); // rubble
 quad(ignored,{0,0,0},{.1f,0,0},{.1f,4,0},{0,4,0}); // pole
 quad(ignored,{-4,0,-4},{4,0,-4},{4,0,4},{-4,0,4}); // floor
 assert(ignored.empty());
 sandbox::WallSegment diagonal{-3,-3,3,3};
 assert(!sandbox::intersectsWall({-2,0,2,.6f,2,.6f,0,0,0},diagonal));
 assert(sandbox::intersectsWall({0,0,0,.6f,2,.6f,0,0,0},diagonal));
 std::cout<<"Doorway walking, solid facade, thin diagonal collision and decoration filtering OK\n";
}
