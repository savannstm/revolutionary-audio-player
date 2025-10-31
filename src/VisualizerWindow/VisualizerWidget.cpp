#include "VisualizerWidget.hpp"

VisualizerWidget::VisualizerWidget(
    const f32* visualizerBuffer,
    QWidget* parent
) :
    parent(parent),
    buffer(visualizerBuffer) {
    setMinimumSize(MIN_VISUALIZER_SIZE);

    connect(&updateTimer, &QTimer::timeout, this, [&] -> void { update(); });
    updateTimer.start(SECOND_MS / u16(screen()->refreshRate()));
}

VisualizerWidget::~VisualizerWidget() {
    makeCurrent();

    projectm_destroy(projectM);
    projectM = nullptr;

    doneCurrent();
}

void VisualizerWidget::initializeGL() {
    makeCurrent();
    const auto ver = QOpenGLContext::currentContext()->format().version();

    if (ver < std::pair(3, 3)) {
        QMessageBox::critical(
            parent,
            tr("OpenGL 3.3 is not supported"),
            tr(
                "Your system does not support OpenGL 3.3. Visualizer won't work. Try to update your GPU drivers, install system updates, or install Mesa, if you're using Linux."
            )
        );
        destroy();
        return;
    }

    QMessageBox::information(
        parent,
        "OpenGL version",
        QString("%1.%2").arg(ver.first).arg(ver.second)
    );

#ifdef Q_OS_WINDOWS
    glewExperimental = GL_TRUE;
    u32 result = glewInit();

    if (result != GLEW_OK || (result = glGetError()) != GLEW_OK) {
        QMessageBox::critical(
            parent,
            tr("Failed to initialize GLEW"),
            tr(
                "Failed to initialize GLEW: %1. It might be because OpenGL context is not made current, checking that: %2, or because your PC is not sufficient to support required OpenGL context."
            )
                .arg(glewGetErrorString(result))
                .arg(
                    context() == QOpenGLContext::currentContext()
                        ? tr("context IS current")
                        : tr("context IS NOT current")
                )
        );
        destroy();
        return;
    }
#endif

    projectM = projectm_create();

    if (projectM == nullptr) {
        QMessageBox::critical(
            parent,
            tr("Unable to initialize projectM"),
            tr(
                "Unable to initialize projectM. It might be because OpenGL context is not made current, checking that: %1, or because your PC is not sufficient to support required OpenGL context. We also could've screwed building the app, so you might want to report it."
            )
                .arg(
                    context() == QOpenGLContext::currentContext()
                        ? tr("context IS current")
                        : tr("context IS NOT current")
                )
        );
        destroy();
        return;
    }

    const QString texturesPath =
        QApplication::applicationDirPath() + "/visualizer/textures";
    const string texturesPathString = texturesPath.toStdString();

    array<cstr, 1> searchPaths = { texturesPathString.c_str() };
    cstr* const searchPathsPtr = searchPaths.data();

    projectm_set_window_size(projectM, width(), height());
    projectm_set_fps(projectM, u16(screen()->refreshRate()));
    projectm_set_mesh_size(projectM, DEFAULT_MESH_WIDTH, DEFAULT_MESH_HEIGHT);
    projectm_set_texture_search_paths(projectM, searchPathsPtr, 1);
}

void VisualizerWidget::resizeGL(const i32 width, const i32 height) {
    glViewport(0, 0, width, height);
    projectm_set_window_size(projectM, width, height);
}

void VisualizerWidget::paintGL() {
    makeCurrent();
    projectm_opengl_render_frame(projectM);
}