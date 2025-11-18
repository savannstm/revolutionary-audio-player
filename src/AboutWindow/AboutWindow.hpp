#pragma once

#include "MainWindow.hpp"
#include "ui_AboutWindow.h"

#include <QDialog>

QT_BEGIN_NAMESPACE

namespace Ui {
    class AboutWindow;
}  // namespace Ui

QT_END_NAMESPACE

class AboutWindow : public QDialog {
    Q_OBJECT

   public:
    explicit AboutWindow(MainWindow* parent = nullptr);
    ~AboutWindow() override;

   protected:
    void changeEvent(QEvent* const event) override {
        if (event->type() == QEvent::LanguageChange) {
            ui->retranslateUi(this);
        }

        QDialog::changeEvent(event);
    }

   private:
    auto setupUi() -> Ui::AboutWindow*;
    Ui::AboutWindow* ui = setupUi();
};
