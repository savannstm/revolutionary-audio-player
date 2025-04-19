#pragma once

#include "aliases.hpp"

#include <QDialog>
#include <QLabel>
#include <QResizeEvent>
#include <QVBoxLayout>

constexpr u8 MIN_SIZE = 64;

class CoverWindow : public QDialog {
    Q_OBJECT

   public:
    explicit CoverWindow(const string& coverBytes, QWidget* parent = nullptr);

   protected:
    void resizeEvent(QResizeEvent* event) override;

   private:
    QVBoxLayout layout = QVBoxLayout(this);
    QLabel image = QLabel(this);
    f64 aspectRatio = 0.0;
};
