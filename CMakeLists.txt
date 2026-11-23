cmake_minimum_required(VERSION 3.20)
project(FloatingLibrary LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Every source file of the project, one module per feature.
add_executable(FloatingLibrary
    src/main.cpp            # window, input, frame loop, on-screen labels
    src/Shader.cpp          # load/compile/link GLSL from shaders/
    src/Texture.cpp         # PNG/JPG -> GPU textures, cubemap (ImageIO)
    src/Mesh.cpp            # VBO/EBO/VAO, the unit cube
    src/Model.cpp           # Assimp .glb loading + embedded textures
    src/Camera.cpp          # first-person fly camera
    src/Lights.cpp          # moonlight + candle point lights
    src/Renderer.cpp        # shared drawing for shadow + colour passes
    src/Library.cpp         # the round library room
    src/FloatingBooks.cpp   # parent/child hierarchy of 12 books
    src/ShadowMap.cpp       # shadow mapping
    src/PostProcess.cpp     # HDR + bloom + tone mapping
    src/TextOverlay.cpp     # bitmap-font labels
)

target_link_libraries(FloatingLibrary PRIVATE
    glfw3
    assimp::assimp
    glm::glm
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
    "-framework QuartzCore"
    "-framework Metal"
    "-framework OpenGL"
    "-framework CoreGraphics"
    "-framework ImageIO"
)
