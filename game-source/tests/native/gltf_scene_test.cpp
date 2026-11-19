#include "../../sys/android/app/src/main/cpp/GltfScene.h"
#include <cassert>
#include "../../sys/android/app/src/main/cpp/CityGraffiti.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include "../../sys/android/app/src/main/cpp/LevelPlacement.h"
#include "../../sys/android/app/src/main/cpp/DebugPlayer.h"
bool read(const std::string&p,std::vector<unsigned char>&b){std::ifstream f(p,std::ios::binary);if(!f)return false;b.assign(std::istreambuf_iterator<char>(f),{});return true;}
int main(){
 // JSON glTF + external .bin, missing normals, node TRS and untextured material.
 const std::string json=R"({"asset":{"version":"2.0"},"scene":0,"scenes":[{"nodes":[0]}],"nodes":[{"mesh":0,"translation":[4,2,7],"scale":[2,1,3]}],"buffers":[{"uri":"floor.bin","byteLength":72}],"bufferViews":[{"buffer":0,"byteLength":72}],"accessors":[{"bufferView":0,"componentType":5126,"count":6,"type":"VEC3"}],"meshes":[{"primitives":[{"attributes":{"POSITION":0}}]}]})";
 float points[]={-2,0,-2,2,0,2,2,0,-2,-2,0,-2,-2,0,2,2,0,2};
 city::ReadFile fixture=[&](const std::string&p,std::vector<unsigned char>&out){if(p=="fixture/scene.gltf"){out.assign(json.begin(),json.end());return true;}if(p=="fixture/floor.bin"){out.resize(sizeof(points));std::memcpy(out.data(),points,sizeof(points));return true;}return false;};
 city::Scene external;std::string details;assert(external.load("fixture/scene.gltf",fixture,details));assert(external.batches.size()==1&&external.batches[0].vertices.size()==6);assert(external.materials.empty());assert(std::abs(external.maximum[2]-external.minimum[2]-80)<.001f);
 // Named untextured furniture receives a procedural material, roughness and world-scale UVs.
 std::string cabinetJson=json;auto marker=cabinetJson.find("\"mesh\":0");cabinetJson.insert(marker,"\"name\":\"Old_Cabinet\",");
 city::ReadFile cabinetRead=[&](const std::string&p,std::vector<unsigned char>&out){
  if(p=="fixture/scene.gltf"){out.assign(cabinetJson.begin(),cabinetJson.end());return true;}
  if(p=="textures/wood.png")return read("sys/android/app/src/main/assets/textures/wood.png",out);
  return fixture(p,out);
 };
 city::Scene cabinet;assert(cabinet.load("fixture/scene.gltf",cabinetRead,details));
 assert(cabinet.materials.size()==1&&cabinet.materials[0].roughness>.9f&&cabinet.materials[0].image==0&&cabinet.materials[0].worldSpaceUV);
 assert(cabinet.images[0].width==512&&cabinet.batches[0].vertices[0].u!=cabinet.batches[0].vertices[1].u);
city::Scene scene;std::string log;assert(!scene.load("/missing.glb",read,log));log.clear();bool ok=scene.load("sys/android/app/src/main/assets/models/abandoned_city_street_for_games.glb",read,log);std::cout<<log<<std::endl;assert(ok);assert(scene.batches.size()==92);std::vector<sandbox::MeshVertex> graffiti;sandbox::appendCityGraffiti(graffiti,*scene.collision);assert(!graffiti.empty());assert(scene.images.size()==8);assert(scene.materials.size()==6);assert(!scene.colliders.empty());sandbox::World world;world.walls=scene.colliders;world.wallSegments=scene.wallSegments;world.x=scene.spawnX;world.z=scene.spawnZ;assert(!world.blocked({world.x,0,world.z,.6f,2,.6f,0,0,0}));for(auto&i:scene.images){std::cout<<i.width<<'x'<<i.height<<'\n';assert(i.width>0&&i.height>0&&i.rgba.size()==size_t(i.width)*i.height*4);}std::cout<<"spawn "<<scene.spawnX<<' '<<scene.spawnZ<<" bounds "<<scene.minimum[0]<<' '<<scene.minimum[1]<<' '<<scene.minimum[2]<<" -> "<<scene.maximum[0]<<' '<<scene.maximum[1]<<' '<<scene.maximum[2]<<std::endl;
 assert(scene.fenceBarriers&&scene.fenceBarriers->size()>0);
 std::cout<<"City player-only fence barrier triangles: "<<scene.fenceBarriers->size()<<"\n";
 size_t opaqueTriangles=0;
 for(const auto &batch:scene.batches)if(!batch.decorativeWire&&(batch.material<0||scene.materials[batch.material].alphaMode==0))opaqueTriangles+=batch.vertices.size()/3;
 assert(scene.collision&&scene.collision->size()>opaqueTriangles);int wires=0;for(const auto&batch:scene.batches)if(batch.decorativeWire)wires++;assert(wires==7);
 std::cout<<"City additional solid fence triangles: "<<scene.collision->size()-opaqueTriangles<<"\n";
 world.x=-3.4f;world.z=32.7f;
 assert(!world.blocked({world.x,0,world.z,.85f,2,.85f,0,0,0}));
 city::SpawnPoint screenshot;assert(city::findRoadSpawn(world,scene.minimum,scene.maximum,-3.4f,32.7f,screenshot));
 assert(screenshot.playerInRegion);
 assert(city::recoverToRoad(world,scene.minimum,scene.maximum,-3.4f,32.7f,true));
 assert(world.x==-3.4f&&world.y==1.7f&&world.z==32.7f);
 for(int i=0;i<100;i++)world.move(0,1,.016f);
 std::cout<<"Screenshot road walk endpoint "<<world.x<<' '<<world.z<<std::endl;
 assert(world.z<26.f); // Previously support-mask cells stopped ordinary street walking.
 city::SpawnPoint anchor;assert(city::findRoadSpawn(world,scene.minimum,scene.maximum,0,28,anchor));
 std::cout<<"reachable "<<anchor.reachableCells<<std::endl;assert(anchor.reachableCells>=64);world.x=anchor.x;world.z=anchor.z;
 float carX=0,carZ=0;assert(city::findCarParking(world,scene.minimum,scene.maximum,anchor.x+3,anchor.z-9,carX,carZ));
 city::LoadOptions options;options.level=false;options.targetExtent=4.025f;options.x=carX;options.z=carZ;options.yaw=1.57079632679f;
 city::Scene car;std::string carLog;assert(car.load("sys/android/app/src/main/assets/models/uaz_469_1973_classic.glb",read,carLog,options));
 assert(!car.batches.empty()&&!car.materials.empty()&&!car.images.empty());
 assert(std::abs(car.maximum[0]-car.minimum[0]-4.025f)<.001f);
 assert(car.maximum[2]-car.minimum[2]<2.f&&car.maximum[1]<2.1f&&car.minimum[1]==0);
 assert(car.colliders.size()==1&&car.colliders[0].h<=1.2f);
 // Replacement UAZ must decode with all embedded textures and retain upright ground placement.
 city::Scene uaz;std::string uazLog;options.targetExtent=4.025f;options.y=-1.25f;
 assert(uaz.load("sys/android/app/src/main/assets/models/uaz_469_1973_classic.glb",read,uazLog,options));
 assert(!uaz.batches.empty()&&!uaz.images.empty());
 assert(std::abs(uaz.minimum[1]+1.25f)<.001f);
 assert(std::abs(uaz.maximum[0]-uaz.minimum[0]-4.025f)<.01f);
 assert(uaz.maximum[1]-uaz.minimum[1]>1.8f&&uaz.maximum[1]-uaz.minimum[1]<2.1f);
 std::cout<<"UAZ dimensions "<<uaz.maximum[0]-uaz.minimum[0]<<" "<<uaz.maximum[1]-uaz.minimum[1]<<" "<<uaz.maximum[2]-uaz.minimum[2]<<" triangles "<<uaz.collision->size()<<"\n";
 std::cout<<"UAZ: "<<uazLog<<" height "<<uaz.maximum[1]-uaz.minimum[1]<<"\n";
 // Optional prop colliders remain supported by the API; the 0.7 game does not enable them.
 world.walls.insert(world.walls.end(),car.colliders.begin(),car.colliders.end());
 assert(world.blocked({carX,0,carZ,.6f,2,.6f,0,0,0}));
 city::SpawnPoint finalSpawn;assert(city::findRoadSpawn(world,scene.minimum,scene.maximum,0,28,finalSpawn));
 assert(!world.blocked({finalSpawn.x,0,finalSpawn.z,3,2,3,0,0,0}));
 assert(finalSpawn.reachableCells>anchor.reachableCells*.85f); // Car must not sever the street.
 world.x=carX-2.8f;world.z=carZ;world.yaw=0;
 assert(!world.blocked({world.x,0,world.z,.6f,2,.6f,0,0,0}));
 for(int i=0;i<100;i++)world.move(1,0,.016f);
 assert(world.x<carX-2.2f); // Actual movement stops at the car, not just a box-query test.
 std::cout<<"New spawn "<<finalSpawn.x<<' '<<finalSpawn.z<<"; connected cells "<<finalSpawn.reachableCells<<"; car "<<carX<<' '<<carZ<<"; car height "<<car.maximum[1]<<std::endl;
}
