cmake_minimum_required(VERSION 3.22.1)
project(realms LANGUAGES C CXX)

# The real Android entry point: GameActivity -> native_app_glue -> GLES sandbox.
# No WebView, npm, JavaScript bundle or remote content is included in this APK.
add_library(realms SHARED main.cpp AndroidOut.cpp Renderer.cpp TextureLoader.cpp ImageDecode.cpp GltfScene.cpp GltfRenderer.cpp ga_shim/system_utils.cpp)
target_include_directories(realms PRIVATE ga_shim)
target_compile_features(realms PRIVATE cxx_std_17)
find_package(game-activity REQUIRED CONFIG)
find_package(game-text-input REQUIRED CONFIG)
target_link_libraries(realms game-activity::game-activity game-text-input::game-text-input EGL GLESv3 android log)
