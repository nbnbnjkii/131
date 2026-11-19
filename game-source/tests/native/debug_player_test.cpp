#include "../../sys/android/app/src/main/cpp/DebugPlayer.h"
#include <cassert>
#include <iostream>
int main(){
 city::SpawnConfig config;std::string error;
 assert(city::parseSpawnConfig(R"({"enabled":true,"position":{"x":1.25,"y":1.7,"z":8.5},"yawDegrees":90})",config,error));
 assert(config.enabled&&!config.startFlying&&config.x==1.25f&&config.z==8.5f);
 assert(city::parseSpawnConfig(R"({"enabled":true,"position":{"x":2,"y":15,"z":3}})",config,error));
 assert(!config.startFlying&&config.y==15);
 assert(!city::parseSpawnConfig("{bad",config,error));
 assert(!city::parseSpawnConfig(R"({"position":{"x":99999,"y":1.7,"z":0}})",config,error));
 assert(!city::parseSpawnConfig(R"({"position":{"x":"invalid","y":1.7,"z":0}})",config,error));
 sandbox::World world;world.walls={{0,1,0,4,2,4,0,0,0}};world.objects.clear();world.x=world.z=0;world.y=1.7f;
 assert(world.blocked({0,0,0,.6f,2,.6f,0,0,0}));
 for(int i=0;i<100;i++)city::fly(world,1,0,1,.016f);
 assert(world.x>5&&world.y>6); // Cross solid wall, rise without gravity.
 float y=world.y;city::fly(world,0,0,0,.05f);assert(world.y==y);
 for(int i=0;i<10;i++)city::fly(world,0,0,-1,.016f);
 assert(world.y<y);
 std::array<float,3> lo{-15,0,-15},hi{15,10,15};
 world.x=world.z=0;world.y=20;
 assert(city::recoverToRoad(world,lo,hi,0,0));
 assert(world.y==1.7f&&!world.blocked({world.x,0,world.z,.85f,2,.85f,0,0,0}));
 // A valid exact position is not rounded to the grid.
 world.x=10.1234f;world.z=10.5678f;world.y=9;
 assert(city::recoverToRoad(world,lo,hi,world.x,world.z));
 assert(world.x==10.1234f&&world.z==10.5678f&&world.y==1.7f);
 // Locally free but fenced in must relocate on landing too.
 world.walls={{-2,1,0,.2f,2,4,0,0,0},{2,1,0,.2f,2,4,0,0,0},{0,1,-2,4,2,.2f,0,0,0},{0,1,2,4,2,.2f,0,0,0}};
 world.x=world.z=0;assert(city::recoverToRoad(world,lo,hi,0,0));assert(std::abs(world.x)>3||std::abs(world.z)>3);
 world.walls={{0,1,0,40,2,40,0,0,0}};assert(!city::recoverToRoad(world,lo,hi,0,0));
 std::cout<<"Spawn config, collision-free flight, vertical controls and safe landing OK\n";
}
