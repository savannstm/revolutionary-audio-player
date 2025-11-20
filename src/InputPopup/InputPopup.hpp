#pragma once

#include <QFrame>
#include <QLineEdit>

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

    void setValidator(const QValidator* const validator) {
        lineEdit->setValidator(validator);
    }

   signals:
    void finished(const QString& text);

   protected:
    void hideEvent(QHideEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

   private:
    QLineEdit* const lineEdit;
};
