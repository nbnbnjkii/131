#pragma once
#include <string>
#include <array>
#include <cctype>
#include "SandboxWorld.h"
namespace sandbox {
struct SurfaceStyle {int layer;std::array<float,3> color;float roughness;const char*texture;};
inline SurfaceStyle materialForName(std::string name){
 for(char &c:name)if(static_cast<unsigned char>(c)<128)c=char(std::tolower(static_cast<unsigned char>(c)));
 auto has=[&](const char*word){return name.find(word)!=std::string::npos;};
 if(has("sofa")||has("couch")||has("диван")||has("Диван"))return {Fabric,{.17f,.22f,.19f},1,"fabric"};
 if(has("door")&&(has("metal")||has("steel")))return {Rust,{.19f,.22f,.23f},.6f,"rust"};
 if(has("chair")||has("cabinet")||has("wardrobe")||has("table")||has("door")||has("стул")||has("шкаф")||has("двер"))return {Wood,{.42f,.29f,.16f},.94f,"wood"};
 if(has("wall")||has("plaster")||has("стен"))return {Concrete,{.52f,.51f,.48f},.98f,"concrete"};
 return {Plain,{1,1,1},.9f,""};
}
}
