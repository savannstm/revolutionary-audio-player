#pragma once

#include "FWD.hpp"

#include <QFrame>

class InputPopup : public QFrame {
    Q_OBJECT

   public:
    explicit InputPopup(QWidget* parent = nullptr);
    explicit InputPopup(const QPoint& pos, QWidget* parent = nullptr);
    explicit InputPopup(
        const QString& text,
        const QPoint& pos,
        QWidget* parent = nullptr
    );

    [[nodiscard]] auto inputWidget() const -> QLineEdit* { return lineEdit; }

    void setValidator(const QValidator* validator);

   signals:
    void finished(const QString& text);

   protected:
    void hideEvent(QHideEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

   private:
    QLineEdit* const lineEdit;
};
