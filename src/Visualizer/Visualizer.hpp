#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"

#ifdef _WIN32
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#include <projectM-4/projectM.h>

#include <QThread>

constexpr f64 DOUBLE_CLICK_DELTA = 0.25;
constexpr u16 MIN_WIDTH = 800;
constexpr u16 MIN_HEIGHT = 600;

struct FullscreenState {
    i32 windowX = 0;
    i32 windowY = 0;
    i32 windowW = MIN_WIDTH;
    i32 windowH = MIN_HEIGHT;
    bool fullscreen = false;
};

struct WindowContext {
    projectm* pm;
    f64 lastClickTime = 0.0;
    FullscreenState fsState;
};

class Visualizer : public QThread {
    Q_OBJECT

   public:
    explicit Visualizer(const string& windowTitle, const string& texturePath);
    ~Visualizer() override;

    void submitAudioData(const f32* samples);
    void loadPreset(const string& presetPath);
    void setSettings(u16 width, u16 height, u16 newFPS);

    static constexpr u16 MIN_MESH_SIZE = 10;
    static constexpr u16 MAX_MESH_SIZE = 200;

    static constexpr u16 DEFAULT_MESH_WIDTH = 80;
    static constexpr u16 DEFAULT_MESH_HEIGHT = 40;

    static constexpr u16 MIN_FPS = 24;
    static constexpr u16 MAX_FPS = 360;

   protected:
    void run() override;

   private:
    static void resizeCallback(GLFWwindow* window, i32 width, i32 height);
    static void
    mouseClickCallback(GLFWwindow* window, i32 button, i32 action, i32 mods);
    static void toggleFullscreen(GLFWwindow* window, FullscreenState& state);

    static constexpr u16 PRESET_PATH_LIMIT = 512;

    array<f32, FFT_SAMPLE_COUNT> samples;

    mutex audioMutex;
    mutex presetMutex;

    string windowTitle;
    string texturePath;

    string presetPath;

    atomicU16 fps{ MIN_FPS };
    atomicU16 meshWidth{ DEFAULT_MESH_WIDTH };
    atomicU16 meshHeight{ DEFAULT_MESH_HEIGHT };

    atomicBool hasNewData{ false };
    atomicBool loadPreset_{ false };
};