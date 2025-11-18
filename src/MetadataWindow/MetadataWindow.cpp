#include "MetadataWindow.hpp"

#include "TrackProperties.hpp"

constexpr QSize METADATA_WINDOW_SIZE = QSize(800, 600);

MetadataWindow::MetadataWindow(
    const HashMap<TrackProperty, QString>& metadata,
    QWidget* const parent
) :
    QDialog(parent) {
    setWindowTitle(metadata[TrackProperty::Title] + tr(": Metadata"));

    treeWidget->setColumnCount(2);
    treeWidget->setHeaderLabels({ tr("Property"), tr("Value") });

    for (const auto& [idx, label] :
         views::drop(views::enumerate(trackPropertiesLabels()), 1)) {
        auto* const item = new QTreeWidgetItem(treeWidget);
        item->setText(0, label);
        item->setText(1, metadata[TrackProperty(idx)]);
        treeWidget->addTopLevelItem(item);
    }

    treeWidget->resizeColumnToContents(0);
    treeWidget->resizeColumnToContents(1);

    layout->addWidget(treeWidget);
    setFixedSize(METADATA_WINDOW_SIZE);
}
