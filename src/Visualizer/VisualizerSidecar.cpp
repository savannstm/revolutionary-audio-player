#include <array>
#include <atomic>
#include <iostream>
#include <thread>

#ifdef _WIN32
#include <GL/glew.h>

#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#endif

#include <GLFW/glfw3.h>
#include <projectM-4/projectM.h>

template <typename O, typename T>
[[nodiscard]] constexpr auto as(T&& arg) -> O {
    return static_cast<O>(std::forward<T>(arg));
}

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using i32 = std::int32_t;
using f32 = float;
using f64 = double;
using std::array;
using std::atomic;
using std::cerr;
using std::cout;

constexpr const char* VISUALIZER_SHARED_MEMORY_LABEL = "rap-visualizer";

constexpr f64 DOUBLE_CLICK_DELTA = 0.25;

constexpr u16 PRESET_PATH_LIMIT = 512;
constexpr u16 DESIRED_SAMPLE_COUNT = 2048;
constexpr u8 F32_SAMPLE_SIZE = sizeof(f32);

constexpr u8 ICON_WIDTH = 32;
constexpr u8 ICON_HEIGHT = 32;

constexpr u16 MIN_WIDTH = 800;
constexpr u16 MIN_HEIGHT = 600;

enum class AudioChannels : u8 {
    Zero = 0,
    Mono = 1,
    Stereo = 2,
    Quad = 4,
    Surround51 = 6,
    Surround71 = 8,

    Surround102 = 12,
    Surround222 = 24,
};

struct VisualizerSharedData {
    array<f32, DESIRED_SAMPLE_COUNT> audioBuffer;
    array<char, PRESET_PATH_LIMIT> presetPath;

    atomic<u16> bufferSize = 8192;
    atomic<u16> fps = 60;
    atomic<u16> meshWidth = 80;
    atomic<u16> meshHeight = 40;

    atomic<bool> loadPreset = false;
    atomic<bool> presetRequested = false;

    atomic<bool> running = true;
    atomic<bool> hasNewData = false;

    atomic<bool> newTrack = false;

    atomic<AudioChannels> channels = AudioChannels::Zero;
};

// NOLINTBEGIN
constinit array<u8, size_t(ICON_WIDTH * ICON_HEIGHT * 4)> ICON_DATA = {
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 10,  204, 0, 0, 94,  204, 0, 0, 70,  204, 0, 0, 26,
    204, 0, 0, 3,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 33,  204, 0, 0, 223, 204, 0, 0, 243, 204, 0, 0, 211,
    204, 0, 0, 166, 204, 0, 0, 113, 204, 0, 0, 63,  204, 0, 0, 25,
    204, 0, 0, 4,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   205, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 63,  204, 0, 0, 245, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 244, 204, 0, 0, 216,
    204, 0, 0, 174, 204, 0, 0, 88,  204, 0, 0, 1,   204, 0, 0, 0,
    3,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   205, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 102, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 223, 204, 0, 0, 37,  204, 0, 0, 0,
    204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 108, 204, 0, 0, 232, 204, 0, 0, 251, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 127, 204, 0, 0, 0,
    204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 9,   204, 0, 0, 39,  204, 0, 0, 83,  204, 0, 0, 137,
    204, 0, 0, 226, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 217, 204, 0, 0, 32,
    204, 0, 0, 0,   204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 8,
    204, 0, 0, 195, 204, 0, 0, 255, 204, 0, 0, 189, 204, 0, 0, 130,
    204, 0, 0, 183, 204, 0, 0, 224, 204, 0, 0, 249, 204, 0, 0, 121,
    204, 0, 0, 0,   204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   2,   0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 5,
    204, 0, 0, 44,  204, 0, 0, 96,  204, 0, 0, 132, 204, 0, 0, 156,
    204, 0, 0, 239, 204, 0, 0, 255, 204, 0, 0, 138, 204, 0, 0, 0,
    204, 0, 0, 5,   204, 0, 0, 32,  204, 0, 0, 76,  204, 0, 0, 89,
    204, 0, 0, 7,   204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 1,   204, 0, 0, 61,  204, 0, 0, 166,
    204, 0, 0, 232, 204, 0, 0, 254, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 235, 204, 0, 0, 157,
    204, 0, 0, 49,  204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   205, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 12,  204, 0, 0, 125, 204, 0, 0, 236, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 226, 204, 0, 0, 99,  204, 0, 0, 3,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 15,
    204, 0, 0, 153, 204, 0, 0, 253, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 245, 204, 0, 0, 111, 204, 0, 0, 1,
    204, 0, 0, 0,   204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 5,   204, 0, 0, 141,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 236, 204, 0, 0, 220, 204, 0, 0, 248,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 240, 204, 0, 0, 81,
    204, 0, 0, 0,   204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 90,  204, 0, 0, 246,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 206,
    204, 0, 0, 112, 204, 0, 0, 46,  204, 0, 0, 60,  204, 0, 0, 232,
    204, 0, 0, 255, 204, 0, 0, 200, 204, 0, 0, 148, 204, 0, 0, 233,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 205,
    204, 0, 0, 26,  204, 0, 0, 0,   204, 0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 27,  204, 0, 0, 208, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 250, 204, 0, 0, 147, 204, 0, 0, 26,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 71,  204, 0, 0, 248,
    204, 0, 0, 255, 204, 0, 0, 141, 204, 0, 0, 0,   204, 0, 0, 59,
    204, 0, 0, 197, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 110, 204, 0, 0, 0,   204, 0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 112, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 254, 204, 0, 0, 140, 204, 0, 0, 7,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 114, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 108, 204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 36,  204, 0, 0, 199, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 190, 204, 0, 0, 11,  204, 0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    204, 0, 0, 19,  204, 0, 0, 198, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 176, 204, 0, 0, 15,  204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 158, 204, 0, 0, 255,
    204, 0, 0, 250, 204, 0, 0, 75,  204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 61,  204, 0, 0, 234, 204, 0, 0, 255,
    204, 0, 0, 232, 204, 0, 0, 41,  204, 0, 0, 0,   0,   0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 74,  204, 0, 0, 243, 204, 0, 0, 255, 204, 0, 0, 244,
    204, 0, 0, 80,  204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 13,  204, 0, 0, 197, 204, 0, 0, 255,
    204, 0, 0, 237, 204, 0, 0, 47,  204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 34,  204, 0, 0, 186, 204, 0, 0, 255,
    204, 0, 0, 247, 204, 0, 0, 65,  204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 16,
    204, 0, 0, 30,  204, 0, 0, 21,  204, 0, 0, 1,   204, 0, 0, 0,
    204, 0, 0, 139, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 254,
    204, 0, 0, 200, 204, 0, 0, 28,  204, 0, 0, 0,   0,   0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 36,  204, 0, 0, 228, 204, 0, 0, 255,
    204, 0, 0, 218, 204, 0, 0, 25,  204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 32,  204, 0, 0, 203, 204, 0, 0, 248, 204, 0, 0, 255,
    204, 0, 0, 251, 204, 0, 0, 118, 204, 0, 0, 51,  204, 0, 0, 26,
    204, 0, 0, 23,  204, 0, 0, 52,  204, 0, 0, 132, 204, 0, 0, 204,
    204, 0, 0, 224, 204, 0, 0, 211, 204, 0, 0, 143, 204, 0, 0, 33,
    204, 0, 0, 192, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 242, 204, 0, 0, 52,  204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 70,  204, 0, 0, 247, 204, 0, 0, 255,
    204, 0, 0, 192, 204, 0, 0, 10,  204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 58,  204, 0, 0, 245, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 254, 204, 0, 0, 246, 204, 0, 0, 240, 204, 0, 0, 219,
    204, 0, 0, 216, 204, 0, 0, 238, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 197,
    204, 0, 0, 224, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 240, 204, 0, 0, 52,  204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 113, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 161, 204, 0, 0, 1,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 58,  204, 0, 0, 243, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 248, 204, 0, 0, 252, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 253, 204, 0, 0, 160,
    204, 0, 0, 240, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 240, 204, 0, 0, 52,  204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 158, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 127, 204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 58,  204, 0, 0, 243, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 252, 204, 0, 0, 185, 204, 0, 0, 145, 204, 0, 0, 88,
    204, 0, 0, 63,  204, 0, 0, 81,  204, 0, 0, 144, 204, 0, 0, 203,
    204, 0, 0, 217, 204, 0, 0, 195, 204, 0, 0, 114, 204, 0, 0, 16,
    204, 0, 0, 243, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 240, 204, 0, 0, 52,  204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 14,  204, 0, 0, 199, 204, 0, 0, 255, 204, 0, 0, 254,
    204, 0, 0, 94,  204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 58,  204, 0, 0, 243, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 208, 204, 0, 0, 27,  204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 16,
    204, 0, 0, 24,  204, 0, 0, 12,  204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 235, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 255,
    204, 0, 0, 240, 204, 0, 0, 52,  204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 39,  204, 0, 0, 231, 204, 0, 0, 255, 204, 0, 0, 245,
    204, 0, 0, 63,  204, 0, 0, 0,   0,   0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 58,  204, 0, 0, 243, 204, 0, 0, 255, 204, 0, 0, 253,
    204, 0, 0, 107, 204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 214, 204, 0, 0, 217, 204, 0, 0, 216, 204, 0, 0, 255,
    204, 0, 0, 240, 204, 0, 0, 52,  204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 78,  204, 0, 0, 250, 204, 0, 0, 255, 204, 0, 0, 230,
    204, 0, 0, 37,  204, 0, 0, 0,   0,   0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 58,  204, 0, 0, 243, 204, 0, 0, 255, 204, 0, 0, 202,
    204, 0, 0, 22,  204, 0, 0, 0,   204, 0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    204, 0, 0, 176, 204, 0, 0, 181, 204, 0, 0, 112, 204, 0, 0, 255,
    204, 0, 0, 238, 204, 0, 0, 46,  204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 123, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 208,
    204, 0, 0, 18,  204, 0, 0, 0,   0,   0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 51,  204, 0, 0, 241, 204, 0, 0, 255, 204, 0, 0, 99,
    204, 0, 0, 0,   204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    204, 0, 0, 117, 204, 0, 0, 180, 204, 0, 0, 23,  204, 0, 0, 129,
    204, 0, 0, 114, 204, 0, 0, 9,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 112, 204, 0, 0, 255, 204, 0, 0, 255, 204, 0, 0, 172,
    204, 0, 0, 4,   204, 0, 0, 0,   0,   0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 10,  204, 0, 0, 118, 204, 0, 0, 126, 204, 0, 0, 14,
    204, 0, 0, 0,   204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    204, 0, 0, 52,  204, 0, 0, 172, 204, 0, 0, 13,  204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 22,  204, 0, 0, 141, 204, 0, 0, 171, 204, 0, 0, 57,
    204, 0, 0, 0,   203, 0, 0, 0,   0,   0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    204, 0, 0, 8,   204, 0, 0, 132, 204, 0, 0, 31,  204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   209, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 3,   204, 0, 0, 0,
    205, 0, 0, 0,   204, 0, 0, 0,   0,   0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 61,  204, 0, 0, 48,  204, 0, 0, 0,
    204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 9,   204, 0, 0, 26,  204, 0, 0, 0,
    204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,   204, 0, 0, 0,
    204, 0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,
    0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0,   0,   0, 0, 0
};

// NOLINTEND

class SharedMemory {
   public:
    VisualizerSharedData* data = nullptr;

#ifdef _WIN32
    HANDLE hMapFile = nullptr;

    auto create(const char* const name) -> bool {
        hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            sizeof(VisualizerSharedData),
            name
        );

        if (hMapFile == nullptr) {
            return false;
        }

        data = (VisualizerSharedData*)MapViewOfFile(
            hMapFile,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            sizeof(VisualizerSharedData)
        );

        if (data == nullptr) {
            CloseHandle(hMapFile);
            return false;
        }

        new (data) VisualizerSharedData();
        return true;
    }

    ~SharedMemory() {
        if (data != nullptr) {
            UnmapViewOfFile(data);
        }

        if (hMapFile != nullptr) {
            CloseHandle(hMapFile);
        }
    }
#else
    i32 fd = -1;

    auto create(const char* const name) -> bool {
        fd = shm_open(name, O_CREAT | O_RDWR, 0666);
        if (fd == -1) {
            return false;
        }

        if (ftruncate(fd, sizeof(VisualizerSharedData)) == -1) {
            close(fd);
            return false;
        }

        data = (VisualizerSharedData*)mmap(
            nullptr,
            sizeof(VisualizerSharedData),
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            0
        );

        if (data == MAP_FAILED) {
            close(fd);
            return false;
        }

        new (data) VisualizerSharedData();
        return true;
    }

    ~SharedMemory() {
        if ((data != nullptr) && data != MAP_FAILED) {
            munmap(data, sizeof(VisualizerSharedData));
            shm_unlink("rap-visualizer");
        }

        if (fd != -1) {
            close(fd);
        }
    }
#endif
};

struct FullscreenState {
    i32 windowX = 0;
    i32 windowY = 0;
    i32 windowW;
    i32 windowH;
    bool fullscreen = false;
};

struct WindowContext {
    projectm* pm;
    VisualizerSharedData* shared;
    f64 lastClickTime = 0.0;
    FullscreenState fsState;
};

void toggleFullscreen(GLFWwindow* const window, FullscreenState& state) {
    if (!state.fullscreen) {
        glfwGetWindowPos(window, &state.windowX, &state.windowY);
        glfwGetWindowSize(window, &state.windowW, &state.windowH);

        GLFWmonitor* const monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* const mode = glfwGetVideoMode(monitor);

        glfwSetWindowMonitor(
            window,
            monitor,
            0,
            0,
            mode->width,
            mode->height,
            mode->refreshRate
        );

        state.fullscreen = true;
    } else {
        glfwSetWindowMonitor(
            window,
            nullptr,
            state.windowX,
            state.windowY,
            state.windowW,
            state.windowH,
            0
        );

        state.fullscreen = false;
    }
}

void resizeCallback(
    GLFWwindow* const window,
    const i32 width,
    const i32 height
) {
    glViewport(0, 0, width, height);
    auto* const ctx = as<WindowContext*>(glfwGetWindowUserPointer(window));

    if (ctx != nullptr) {
        projectm_set_window_size(ctx->pm, width, height);
    }
}

void mouseClickCallback(
    GLFWwindow* const window,
    const i32 button,
    const i32 action,
    const i32 mods
) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        auto* const ctx = as<WindowContext*>(glfwGetWindowUserPointer(window));
        const f64 now = glfwGetTime();
        const f64 delta = now - ctx->lastClickTime;

        ctx->lastClickTime = now;

        if (delta < DOUBLE_CLICK_DELTA) {
            toggleFullscreen(window, ctx->fsState);
        }
    }
}

auto main(i32 argc, char* argv[]) -> i32 {
    SharedMemory sharedMemory;
    if (!sharedMemory.create(VISUALIZER_SHARED_MEMORY_LABEL)) {
        println(cerr, "Failed to create shared memory");
        return 1;
    }

    if (glfwInit() == 0) {
        println(cerr, "Failed to initialize GLFW");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);

    GLFWwindow* const window =
        glfwCreateWindow(MIN_WIDTH, MIN_HEIGHT, argv[1], nullptr, nullptr);

    if (window == nullptr) {
        println(cerr, "Failed to create GLFW window");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

#ifdef _WIN32
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        println(cerr, "Failed to initialize GLEW");
        return 1;
    }
#endif

    projectm* const projectM = projectm_create();
    if (projectM == nullptr) {
        println(cerr, "Failed to create projectM instance");
        return 1;
    }

    glfwSetWindowSizeLimits(
        window,
        MIN_WIDTH,
        MIN_HEIGHT,
        GLFW_DONT_CARE,
        GLFW_DONT_CARE
    );

    const auto glfwIcon =
        GLFWimage{ .width = 32, .height = 32, .pixels = ICON_DATA.data() };

    glfwSetWindowIcon(window, 1, &glfwIcon);

    auto* const ctx = new WindowContext{ .pm = projectM,
                                         .shared = sharedMemory.data,
                                         .fsState = FullscreenState{} };
    glfwSetWindowUserPointer(window, ctx);

    glfwSetFramebufferSizeCallback(window, resizeCallback);
    glfwSetMouseButtonCallback(window, mouseClickCallback);

    i32 width;
    i32 height;

    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    projectm_set_window_size(projectM, width, height);
    projectm_set_fps(projectM, sharedMemory.data->fps.load());
    projectm_set_mesh_size(
        projectM,
        sharedMemory.data->meshWidth.load(),
        sharedMemory.data->meshHeight.load()
    );
    projectm_set_texture_search_paths(
        projectM,
        const_cast<const char**>(&argv[2]),
        1
    );

    println(cerr, "initialized");

    f64 nextFrameTime = glfwGetTime();

    while ((glfwWindowShouldClose(window) == 0) &&
           sharedMemory.data->running.load()) {
        const f64 fps = sharedMemory.data->fps.load();
        const f64 frameInterval = 1.0 / fps;

        const f64 currentTime = glfwGetTime();

        if (currentTime < nextFrameTime) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        nextFrameTime = currentTime + frameInterval;

        if (sharedMemory.data->loadPreset.exchange(false)) {
            println(
                cout,
                "Loading preset: {}",
                sharedMemory.data->presetPath.data()
            );

            projectm_load_preset_file(
                projectM,
                sharedMemory.data->presetPath.data(),
                true
            );
        }

        if (sharedMemory.data->newTrack.exchange(false)) {
            projectm_load_preset_file(
                projectM,
                sharedMemory.data->presetPath.data(),
                true
            );
        }

        if (sharedMemory.data->hasNewData.exchange(false)) {
            const u16 bufferSize = sharedMemory.data->bufferSize.load();
            const AudioChannels channels = sharedMemory.data->channels.load();

            projectm_pcm_add_float(
                projectM,
                sharedMemory.data->audioBuffer.data(),
                bufferSize / (F32_SAMPLE_SIZE * u8(channels)),
                projectm_channels(channels)
            );
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        projectm_opengl_render_frame(projectM);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    projectm_destroy(projectM);
    glfwTerminate();

    return 0;
}

#ifdef _WIN32
// NOLINTBEGIN
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main(__argc, __argv);
}

// NOLINTEND
#endif
