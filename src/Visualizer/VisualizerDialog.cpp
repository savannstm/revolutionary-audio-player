#include "VisualizerDialog.hpp"

#include "Constants.hpp"
#include "Enums.hpp"
#include "Logger.hpp"

constexpr QSize VISUALIZER_DIALOG_SIZE = QSize(400, 240);

VisualizerDialog::VisualizerDialog(
    shared_ptr<Settings> settings_,
    QWidget* const parent
) :
    QDialog(parent, Qt::Window),
    settings(std::move(settings_)) {
    if (!initSharedMemory()) {
        reject();
        return;
    };

    setWindowTitle(tr("Visualizer Settings"));
    setFixedSize(VISUALIZER_DIALOG_SIZE);

    auto* const layout = new QFormLayout(this);

    fpsSpin = new QSpinBox(this);
    fpsSpin->setRange(MIN_FPS, MAX_FPS);
    fpsSpin->setValue(settings->visualizer.fps);

    meshWidthSpin = new QSpinBox(this);
    meshHeightSpin = new QSpinBox(this);
    meshWidthSpin->setRange(MIN_MESH_SIZE, MAX_MESH_SIZE);
    meshHeightSpin->setRange(MIN_MESH_SIZE, MAX_MESH_SIZE);
    meshWidthSpin->setValue(settings->visualizer.meshWidth);
    meshHeightSpin->setValue(settings->visualizer.meshHeight);

    presetButton = new QPushButton(tr("Load Preset"), this);
    applyButton = new QPushButton(tr("Apply Settings"), this);

    layout->addRow("FPS:", fpsSpin);
    layout->addRow(tr("Mesh Width:"), meshWidthSpin);
    layout->addRow(tr("Mesh Height:"), meshHeightSpin);
    layout->addRow(presetButton);
    layout->addRow(applyButton);

    connect(
        applyButton,
        &QPushButton::clicked,
        this,
        &VisualizerDialog::applySettings
    );

    connect(
        presetButton,
        &QPushButton::clicked,
        this,
        &VisualizerDialog::loadPreset
    );

    const QString visualizerPath = QApplication::applicationDirPath() +
#ifdef Q_OS_WINDOWS
                                   "/rap-visualizer.exe";
#else
                                   "/rap-visualizer";
#endif

    process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardOutput, this, [this] -> void {
        const QByteArray data = process->readAllStandardOutput();

        LOG_INFO(data);
    });

    connect(process, &QProcess::readyReadStandardError, this, [this] -> void {
        const QByteArray data = process->readAllStandardError();

        // Doesn't work with stdout
        if (data == "initialized\n") {
            emit ready();
        }

        LOG_ERROR(data);
    });

    connect(
        process,
        &QProcess::finished,
        this,
        [this](const i32 exitCode, const QProcess::ExitStatus exitStatus)
            -> void {
        LOG_INFO(
            u"Visualizer exited with code: "_s + QString::number(exitCode)
        );
        reject();
        return;
    }
    );

    process->start(
        visualizerPath,
        { tr("Visualizer"),
          QApplication::applicationDirPath() + "/visualizer/textures" },
        QProcess::ReadOnly
    );

    if (!process->waitForStarted(SECOND_MS * 2)) {
        reject();
        return;
    }

    applySettings();
    openPreset(settings->visualizer.presetPath);

    sharedData->running.store(true);
}

VisualizerDialog::~VisualizerDialog() {
    sharedData->running.store(false);

    if (process != nullptr) {
        process->terminate();
        process->waitForFinished(SECOND_MS * 2);
        process->kill();
        process = nullptr;
    }

#ifdef Q_OS_WINDOWS
    if (sharedData != nullptr) {
        UnmapViewOfFile(sharedData);
    }

    if (hMapFile != nullptr) {
        CloseHandle(hMapFile);
    }
#else
    if (sharedData != nullptr && sharedData != MAP_FAILED) {
        munmap(sharedData, sizeof(VisualizerSharedData));
    }

    if (shmFd != -1) {
        ::close(shmFd);
    }
#endif
}

auto VisualizerDialog::initSharedMemory() -> bool {
#ifdef Q_OS_WINDOWS
    hMapFile = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        VISUALIZER_SHARED_MEMORY_LABEL
    );

    if (hMapFile == nullptr) {
        hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            sizeof(VisualizerSharedData),
            VISUALIZER_SHARED_MEMORY_LABEL
        );
    }

    if (hMapFile == nullptr) {
        return false;
    }

    sharedData = as<VisualizerSharedData*>(MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(VisualizerSharedData)
    ));
#else
    shmFd = shm_open(VISUALIZER_SHARED_MEMORY_LABEL, O_RDWR, 0666);

    if (shmFd == -1) {
        shmFd =
            shm_open(VISUALIZER_SHARED_MEMORY_LABEL, O_CREAT | O_RDWR, 0666);

        if (shmFd != -1) {
            ftruncate(shmFd, sizeof(VisualizerSharedData));
        }
    }

    if (shmFd == -1) {
        return false;
    }

    sharedData = (VisualizerSharedData*)mmap(
        nullptr,
        sizeof(VisualizerSharedData),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shmFd,
        0
    );

    if (sharedData == MAP_FAILED) {
        sharedData = nullptr;
    }
#endif

    return sharedData != nullptr;
}

void VisualizerDialog::applySettings() {
    sharedData->fps.store(fpsSpin->value());
    sharedData->meshWidth.store(meshWidthSpin->value());
    sharedData->meshHeight.store(meshHeightSpin->value());

    settings->visualizer.fps = fpsSpin->value();
    settings->visualizer.meshWidth = meshWidthSpin->value();
    settings->visualizer.meshHeight = meshHeightSpin->value();
}

void VisualizerDialog::setChannels(const AudioChannels channels) {
    sharedData->channels.store(channels);
    sharedData->bufferSize.store(
        channels == AudioChannels::Surround51 ? MIN_BUFFER_SIZE_3BYTES
                                              : MIN_BUFFER_SIZE
    );
}

void VisualizerDialog::addSamples(const f32* const buffer) {
    memcpy(
        sharedData->audioBuffer.data(),
        buffer,
        sharedData->bufferSize.load()
    );

    sharedData->hasNewData.store(true);
}

void VisualizerDialog::clear() {
    memset(sharedData->audioBuffer.data(), 0, sharedData->bufferSize.load());
    sharedData->hasNewData.store(true);
}

void VisualizerDialog::loadPreset() {
    const QString presetPath = QFileDialog::getOpenFileName(
        this,
        tr("Select Preset"),
        QString(),
        tr("Milkdrop Presets (*.milk)")
    );

    openPreset(presetPath);
}

void VisualizerDialog::openPreset(const QString& presetPath) {
    if (presetPath.isEmpty()) {
        return;
    }

    const QByteArray utf8Path = presetPath.toUtf8();

    memcpy(
        sharedData->presetPath.data(),
        utf8Path.constData(),
        utf8Path.size()
    );

    sharedData->presetPath[utf8Path.size()] = '\0';
    sharedData->loadPreset.store(true);

    settings->visualizer.presetPath = presetPath;
}