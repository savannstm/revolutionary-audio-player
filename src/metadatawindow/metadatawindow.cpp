#include "metadatawindow.hpp"

#include "enums.hpp"
#include "trackproperties.hpp"

MetadataWindow::MetadataWindow(const MetadataMap& metadata, QWidget* parent) :
    QDialog(parent) {
    setWindowTitle(tr("%1: Metadata").arg(metadata[Title]));

    treeWidget->setColumnCount(2);
    treeWidget->setHeaderLabels({ tr("Property"), tr("Value") });

    for (const auto& [idx, label] :
         views::drop(views::enumerate(trackPropertiesLabels()), 1)) {
        auto* item = new QTreeWidgetItem(treeWidget);
        item->setText(0, label);
        item->setText(1, metadata[idx]);
        treeWidget->addTopLevelItem(item);
    }

    treeWidget->resizeColumnToContents(0);
    treeWidget->resizeColumnToContents(1);

    layout->addWidget(treeWidget);
    setFixedSize(800, 600);
}
