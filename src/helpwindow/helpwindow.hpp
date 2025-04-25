#pragma once

#include "ui_helpwindow.h"

#include <QDialog>

QT_BEGIN_NAMESPACE

namespace Ui {
    class HelpWindow;
}  // namespace Ui

QT_END_NAMESPACE

// TODO: Implement help window

class HelpWindow : public QDialog {
    Q_OBJECT

   public:
    explicit HelpWindow(QWidget* parent);
    ~HelpWindow() override;

   private:
    auto setupUi() -> Ui::HelpWindow*;
    Ui::HelpWindow* ui = setupUi();
};