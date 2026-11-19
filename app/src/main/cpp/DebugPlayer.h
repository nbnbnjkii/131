#pragma once
#include "LevelPlacement.h"
#include "third_party/json.hpp"

namespace city {
struct SpawnConfig {
    bool enabled=false,startFlying=false;
    float x=-3.4f,y=1.7f,z=32.7f,yawDegrees=0;
};
inline bool parseSpawnConfig(const std::string &text,SpawnConfig &config,std::string &error){
    try{
        auto j=nlohmann::json::parse(text);SpawnConfig parsed;
        parsed.enabled=j.value("enabled",false);parsed.startFlying=j.value("startFlying",false);
        auto p=j.at("position");parsed.x=p.at("x").get<float>();parsed.y=p.at("y").get<float>();parsed.z=p.at("z").get<float>();
        parsed.yawDegrees=j.value("yawDegrees",0.f);
        if(!std::isfinite(parsed.x)||!std::isfinite(parsed.y)||!std::isfinite(parsed.z)||!std::isfinite(parsed.yawDegrees)||std::abs(parsed.x)>512||std::abs(parsed.z)>512||parsed.y < -100||parsed.y>200)throw std::runtime_error("spawn outside debug limits");
        // Height no longer forces noclip: gravity handles elevated exact spawns.
        config=parsed;return true;
    }catch(const std::exception&e){error=e.what();return false;}
}
inline void fly(sandbox::World &world,float right,float forward,float vertical,float dt){
    dt=std::clamp(dt,0.f,.05f);float length=std::max(1.f,std::sqrt(right*right+forward*forward+vertical*vertical));
    float speed=8.f*dt/length;
    world.x=std::clamp(world.x+(std::cos(world.yaw)*right-std::sin(world.yaw)*std::cos(world.pitch)*forward)*speed,-512.f,512.f);
    world.z=std::clamp(world.z+(-std::sin(world.yaw)*right-std::cos(world.yaw)*std::cos(world.pitch)*forward)*speed,-512.f,512.f);
    world.y=std::clamp(world.y+(std::sin(world.pitch)*forward+vertical)*speed,-100.f,200.f);
}
// Relocate to the nearest open point in the MAIN component if embedded or fenced in.
// y is the CAMERA height, not the player's feet. Ground walking is one-level at y=1.7.
inline bool recoverToRoad(sandbox::World &world,const std::array<float,3>&lo,const std::array<float,3>&hi,
                          float preferredX,float preferredZ,bool preserveValidPosition=true){
    SpawnPoint safe;
    if(!findRoadSpawn(world,lo,hi,preferredX,preferredZ,safe))return false;
    if(!preserveValidPosition||!safe.playerInRegion||world.blocked({world.x,0,world.z,.85f,2,.85f,0,0,0})){
        world.x=safe.x;world.z=safe.z;
    }
    world.y=1.7f;
    return true;
}
}
