#include "MetadataWindow.hpp"

#include "Duration.hpp"
#include "Enums.hpp"
#include "Utils.hpp"

#include <QTreeWidget>
#include <QVBoxLayout>

MetadataWindow::MetadataWindow(
    const TrackMetadata& metadata,
    QWidget* const parent
) :
    QDialog(parent),
    treeWidget(new QTreeWidget(this)),
    layout(new QVBoxLayout(this)) {
    setWindowTitle(metadata[TrackProperty::Title] + tr(": Metadata"));

    treeWidget->setColumnCount(2);
    treeWidget->setHeaderLabels({ tr("Property"), tr("Value") });

    for (const auto prop : TRACK_PROPERTIES) {
        auto* const item = new QTreeWidgetItem(treeWidget);
        item->setText(0, trackPropertyLabel(prop));
        item->setText(
            1,
            prop == TrackProperty::Duration
                ? Duration::secondsToString(metadata[prop].toUInt())
                : metadata[prop]
        );
        treeWidget->addTopLevelItem(item);
    }

    treeWidget->resizeColumnToContents(0);
    treeWidget->resizeColumnToContents(1);

    layout->addWidget(treeWidget);
    setFixedSize(METADATA_WINDOW_SIZE);
}
