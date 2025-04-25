#include "metadatawindow.hpp"

#include "trackpropertiesmap.hpp"

MetadataWindow::MetadataWindow(
    const metadata_array& metadata,
    QWidget* parent
) :
    QDialog(parent) {
    setWindowTitle(u"%1: Metadata"_s.arg(metadata[TrackProperty::Title]));

    treeWidget.setColumnCount(2);
    treeWidget.setHeaderLabels({ "Property", "Value" });

    for (const auto& [label, property] : TRACK_PROPERTIES_MAP) {
        auto* item = new QTreeWidgetItem(&treeWidget);
        item->setText(0, label);
        item->setText(1, metadata[property]);
        treeWidget.addTopLevelItem(item);
    }

    treeWidget.resizeColumnToContents(0);
    treeWidget.resizeColumnToContents(1);

    layout.addWidget(&treeWidget);
    setLayout(&layout);
}
