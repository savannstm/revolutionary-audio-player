#pragma once

#include "ui_HelpWindow.h"

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
    using QDialog::QDialog;
    ~HelpWindow() override;

   private:
    auto setupUi() -> Ui::HelpWindow*;
    Ui::HelpWindow* ui = setupUi();
};