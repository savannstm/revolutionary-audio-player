
#include "metadatawindow.hpp"

#include "constants.hpp"

#include <QVBoxLayout>

MetadataWindow::MetadataWindow(
    const metadata_array& metadata,
    QWidget* parent
) :
    QDialog(parent) {
    setWindowTitle("Metadata");

    treeWidget.setColumnCount(2);
    treeWidget.setHeaderLabels({ "Property", "Value" });

    populateTree(metadata);

    treeWidget.resizeColumnToContents(0);
    treeWidget.resizeColumnToContents(1);

    layout.addWidget(&treeWidget);
    setLayout(&layout);

    resize(800, 600);
}

void MetadataWindow::populateTree(const metadata_array& metadata) {
    for (const auto& [property, value] : TRACK_PROPERTIES_MAP) {
        auto* item = new QTreeWidgetItem(&treeWidget);
        item->setText(0, property);
        item->setText(1, metadata[value].data());
        treeWidget.addTopLevelItem(item);
    }
}

auto MetadataWindow::propertyToLabel(TrackProperty prop) -> QString {
    switch (prop) {
        case Title:
            return "Title";
        case Artist:
            return "Artist";
        case Album:
            return "Album";
        case TrackNumber:
            return "Track Number";
        case AlbumArtist:
            return "Album Artist";
        case Genre:
            return "Genre";
        case Year:
            return "Year";
        case Duration:
            return "Duration";
        case Composer:
            return "Composer";
        case BPM:
            return "BPM";
        case Language:
            return "Language";
        case DiscNumber:
            return "Disc Number";
        case Comment:
            return "Comment";
        case Publisher:
            return "Publisher";
        case Bitrate:
            return "Bitrate";
        case SampleRate:
            return "Sample Rate";
        case Channels:
            return "Channels";
        case Format:
            return "File Format";
        case Path:
            return "Path";
        default:
            return "";
    }
}