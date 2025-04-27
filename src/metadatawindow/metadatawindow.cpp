#include "metadatawindow.hpp"

#include "trackproperties.hpp"

MetadataWindow::MetadataWindow(
    const QMap<u8, QString>& metadata,
    QWidget* parent
) :
    QDialog(parent) {
    setWindowTitle(u"%1: Metadata"_s.arg(metadata[Title]));

    treeWidget.setColumnCount(2);
    treeWidget.setHeaderLabels({ "Property", "Value" });

    for (const auto& [idx, label] : views::enumerate(trackPropertiesLabels())) {
        auto* item = new QTreeWidgetItem(&treeWidget);
        item->setText(0, label);
        item->setText(1, metadata[idx]);
        treeWidget.addTopLevelItem(item);
    }

    treeWidget.resizeColumnToContents(0);
    treeWidget.resizeColumnToContents(1);

    layout.addWidget(&treeWidget);
    setFixedSize(800, 600);
}
