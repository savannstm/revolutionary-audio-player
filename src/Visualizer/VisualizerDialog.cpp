#include "VisualizerDialog.hpp"

#include "Constants.hpp"
#include "Settings.hpp"
#include "Utils.hpp"

#include <QCheckBox>
#include <QDebug>
#include <QDialog>
#include <QFileDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QScreen>
#include <QSpinBox>

constexpr QSize VISUALIZER_DIALOG_SIZE = QSize(400, 240);

VisualizerDialog::VisualizerDialog(
    const shared_ptr<Settings>& settings_,
    QWidget* const parent
) :
    QDialog(parent, Qt::Window),
    settings(settings_->visualizer) {
    setWindowTitle(tr("Visualizer Settings"));
    setFixedSize(VISUALIZER_DIALOG_SIZE);

    auto* const layout = new QFormLayout(this);

    fpsSpin = new QSpinBox(this);
    fpsSpin->setRange(Visualizer::MIN_FPS, Visualizer::MAX_FPS);
    fpsSpin->setValue(
        settings.fps == 0 ? u16(screen()->refreshRate()) : settings.fps
    );

    meshWidthSpin = new QSpinBox(this);
    meshHeightSpin = new QSpinBox(this);
    meshWidthSpin->setRange(
        Visualizer::MIN_MESH_SIZE,
        Visualizer::MAX_MESH_SIZE
    );
    meshHeightSpin->setRange(
        Visualizer::MIN_MESH_SIZE,
        Visualizer::MAX_MESH_SIZE
    );
    meshWidthSpin->setValue(settings.meshWidth);
    meshHeightSpin->setValue(settings.meshHeight);

    useRandomPresetsCheckbox =
        new QCheckBox(tr("Load random preset with each new track"), this);
    randomPresetDirInput = new QLineEdit(settings.randomPresetDir, this);
    const QAction* const locateRandomPresetDirAction =
        randomPresetDirInput->addAction(
            QIcon::fromTheme(QIcon::ThemeIcon::EditFind),
            QLineEdit::TrailingPosition
        );

    presetButton = new QPushButton(tr("Load Preset"), this);
    applyButton = new QPushButton(tr("Apply Settings"), this);

    layout->addRow(u"FPS:"_s, fpsSpin);
    layout->addRow(tr("Mesh Width:"), meshWidthSpin);
    layout->addRow(tr("Mesh Height:"), meshHeightSpin);
    layout->addRow(useRandomPresetsCheckbox);
    layout->addRow(tr("Random Preset Directory:"), randomPresetDirInput);
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

    connect(
        locateRandomPresetDirAction,
        &QAction::triggered,
        this,
        [this] -> void {
        const QString presetDir = QFileDialog::getExistingDirectory(
            this,
            tr("Select preset directory"),
            QString()
        );

        if (presetDir.isEmpty()) {
            useRandomPresetsCheckbox->setCheckState(Qt::Unchecked);
            return;
        }

        randomPresetDirInput->setText(presetDir);
        settings.randomPresetDir = presetDir;
    }
    );

    visualizer = new Visualizer(
        tr("Visualizer").toStdString(),
        (qApp->applicationDirPath() + u"/visualizer/textures"_qssv)
            .toStdString()
    );

    applySettings();
    visualizer->start();
}

VisualizerDialog::~VisualizerDialog() {
    delete visualizer;
}

void VisualizerDialog::applySettings() {
    visualizer->setSettings(
        meshWidthSpin->value(),
        meshHeightSpin->value(),
        fpsSpin->value()
    );

    settings.fps = fpsSpin->value();
    settings.meshWidth = meshWidthSpin->value();
    settings.meshHeight = meshHeightSpin->value();
}

void VisualizerDialog::setChannels(const u8 channels) {
    if (useRandomPresetsCheckbox->isChecked()) {
        const QString randomPresetDir = randomPresetDirInput->text();

        if (!QFile::exists(randomPresetDir)) {
            QMessageBox::warning(
                this,
                tr("Random preset directory doesn't exist"),
                tr(
                    "Unable to locate random preset directory. Create it or change it."
                )
            );
            return;
        }

        QStringList milkFiles;

        std::function<void(const QDir&)> readDir =
            [&readDir, &milkFiles](const QDir& dir) -> void {
            const QFileInfoList files = dir.entryInfoList(
                { u"*.milk"_s },
                QDir::Files | QDir::Readable | QDir::NoDotAndDotDot
            );

            for (const QFileInfo& fileInfo : files) {
                milkFiles.push_back(fileInfo.fileName());
            }

            const QFileInfoList subdirs =
                dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

            for (const QFileInfo& subdir : subdirs) {
                readDir(QDir(subdir.absoluteFilePath()));
            }
        };

        readDir(QDir(randomPresetDir));

        if (milkFiles.isEmpty()) {
            QMessageBox::warning(
                this,
                tr("Couldn't find any presets in random preset directory"),
                tr(
                    "Random preset directory doesn't contain any `.milk` presets. Add some presets to it or change the directory."
                )
            );
            return;
        }

        const u16 randomPresetIndex = randint_range(0, milkFiles.size());
        const QString presetPath =
            randomPresetDir + '/' + milkFiles[randomPresetIndex];

        openPreset(presetPath);
    }

    this->channels = channels;
}

void VisualizerDialog::addSamples(const f32* const samples) {
    visualizer->submitAudioData(samples);
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

    visualizer->loadPreset(presetPath.toStdString());
    settings.presetPath = presetPath;
}