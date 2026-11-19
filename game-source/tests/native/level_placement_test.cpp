#include "../../sys/android/app/src/main/cpp/LevelPlacement.h"
#include <cassert>
#include <iostream>
int main(){
 sandbox::World world;world.walls.clear();world.objects.clear();world.x=world.z=0;
 // A locally empty fenced pocket was accepted by the previous spawn test.
 world.walls={{-2,1,0,.2f,2,4,0,0,0},{2,1,0,.2f,2,4,0,0,0},{0,1,-2,4,2,.2f,0,0,0},{0,1,2,4,2,.2f,0,0,0}};
 assert(!world.blocked({0,0,0,.6f,2,.6f,0,0,0}));
 city::SpawnPoint spawn;std::array<float,3> lo{-15,0,-15},hi{15,2,15};
 assert(city::findRoadSpawn(world,lo,hi,0,0,spawn));
 assert(spawn.reachableCells>2000&&!spawn.playerInRegion);
 assert(std::abs(spawn.x)>3||std::abs(spawn.z)>3);
 assert(!world.blocked({spawn.x,0,spawn.z,3,2,3,0,0,0}));
 world.objects.push_back({0,0,spawn.x,spawn.z});
 city::SpawnPoint restored;assert(city::findRoadSpawn(world,lo,hi,spawn.x,spawn.z,restored));
 assert(!world.blocked({restored.x,0,restored.z,3,2,3,0,0,0}));
 world.x=restored.x;world.z=restored.z;
 float x,z;assert(city::findCarParking(world,lo,hi,8,8,x,z));
 assert(!world.blocked({x,0,z,2.5f,1.2f,5,0,0,0}));
 assert(std::hypot(x-world.x,z-world.z)>=6);
 std::cout<<"Fenced spawn rejection, saved construction avoidance and car parking OK\n";
}
