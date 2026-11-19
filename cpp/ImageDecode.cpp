// One shared stb_image implementation for city PNG/JPEG and procedural PNG materials.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 8192
#include "third_party/stb_image.h"
