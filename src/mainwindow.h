#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QAudioOutput>
#include <QCloseEvent>
#include <QMainWindow>
#include <QMediaPlayer>
#include <QPushButton>
#include <QSlider>
#include <QStandardItemModel>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QSystemTrayIcon>
#include <QTextEdit>

#include "customslider.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}  // namespace Ui
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

   protected:
    void closeEvent(QCloseEvent* event) override {
        this->hide();
        event->ignore();
    }

   private:
    Ui::MainWindow* ui;
    QMediaPlayer* player;
    QAudioOutput* audioOutput;
    QAction* exitAction;
    QAction* openFileAction;
    QAction* openFolderAction;
    QStandardItemModel* tracksModel;
    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu;
    CustomSlider* slider;
    QRect sliderKnobRect;

    [[nodiscard]] auto isOnSliderKnob(const QPoint& pos) const -> bool {
        QStyleOptionSlider opt;
        opt.initFrom(slider);
        opt.orientation = slider->orientation();
        opt.maximum = slider->maximum();
        opt.minimum = slider->minimum();
        opt.sliderPosition = slider->value();
        opt.sliderValue = slider->value();
        opt.subControls = QStyle::SC_All;

        QRect handleRect = slider->style()->subControlRect(
            QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, slider);

        return handleRect.contains(pos);
    }
};

#endif  // MAINWINDOW_H
