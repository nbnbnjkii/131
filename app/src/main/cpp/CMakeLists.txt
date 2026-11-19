cmake_minimum_required(VERSION 3.22.1)
project(realms LANGUAGES C CXX)

# The real Android entry point: GameActivity -> native_app_glue -> GLES sandbox.
# No WebView, npm, JavaScript bundle or remote content is included in this APK.
add_library(realms SHARED main.cpp AndroidOut.cpp Renderer.cpp TextureLoader.cpp ImageDecode.cpp GltfScene.cpp GltfRenderer.cpp)
target_compile_features(realms PRIVATE cxx_std_17)
find_package(game-activity REQUIRED CONFIG)
target_link_libraries(realms game-activity::game-activity EGL GLESv3 android log)
