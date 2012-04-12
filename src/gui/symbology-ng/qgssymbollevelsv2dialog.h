#ifndef QGSSYMBOLLEVELSV2DIALOG_H
#define QGSSYMBOLLEVELSV2DIALOG_H

#include <QDialog>
#include <QList>

#include "qgsrendererv2.h"

#include "ui_qgssymbollevelsv2dialogbase.h"


class GUI_EXPORT QgsSymbolLevelsV2Dialog : public QDialog, private Ui::QgsSymbolLevelsV2DialogBase
{
    Q_OBJECT
  public:
    QgsSymbolLevelsV2Dialog( QgsLegendSymbolList list, bool usingSymbolLevels, QWidget* parent = NULL );

    bool usingLevels() const;

    // used by rule-based renderer (to hide checkbox to enable/disable ordering)
    void setForceOrderingEnabled( bool enabled );

  public slots:
    void updateUi();

    void renderingPassChanged( int row, int column );

  protected:
    void populateTable();
    void setDefaultLevels();

  protected:
    //! maximal number of layers from all symbols
    int mMaxLayers;
    QgsLegendSymbolList mList;
    //! whether symbol layers always should be used (default false)
    bool mForceOrderingEnabled;
};

#endif // QGSSYMBOLLEVELSV2DIALOG_H
