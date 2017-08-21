class QgsPasteHandler {
%TypeHeaderCode
#include <qgsclipboard.h>
%End
  public:
  virtual ~QgsPasteHandler();
  virtual void paste(const QString& mimeData, const QByteArray& data, const QgsPoint* mapPos) = 0;
};

class QgsClipboard : public QObject
{
%TypeHeaderCode
#include <qgsclipboard.h>
%End
  public:
    QgsClipboard(QObject* parent = 0);

    // Returns whether there is any data in the clipboard
    bool isEmpty() const;

    // Queries whether the clipboard has specified format.
    bool hasFormat(const QString& format) const;

    // Sets the clipboard contents
    void setMimeData(QMimeData* mimeData);

    // Retreives the clipboard contents
    const QMimeData* mimeData();

    // Utility function for storing features in clipboard
    void setStoredFeatures(const QgsFeatureStore& featureStore);

    // Utility function for getting features from clipboard
    const QgsFeatureStore& getStoredFeatures() const;

  signals:
    void dataChanged();
};
