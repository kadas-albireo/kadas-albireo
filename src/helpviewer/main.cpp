#include <iostream>
#include <qgsapplication.h>
#include <qstring.h>
#include <QTextCodec>
#include <QTranslator>
#include "qgshelpserver.h"
#include "qgshelpviewer.h"
#include "qgsapplication.h"

int main( int argc, char ** argv )
{
  QgsApplication a( argc, argv, true );
  QString context = QString::null;
  QString myTranslationCode="";

  if(argc == 2)
  {
    context = argv[1];
  }
#ifdef Q_OS_MACX
  // If we're on Mac, we have the resource library way above us...
  a.setPkgDataPath(QgsApplication::prefixPath()+"/../../../../share/qgis");
#endif

  QString i18nPath = QgsApplication::i18nPath();
  if (myTranslationCode.isEmpty())
  {
    myTranslationCode = QTextCodec::locale();
  }
#ifdef QGISDEBUG
  std::cout << "Setting translation to "
    << i18nPath.toLocal8Bit().data() << "/qgis_" << myTranslationCode.toLocal8Bit().data() << std::endl;
#endif

  /* Translation file for Qt.
   * The strings from the QMenuBar context section are used by Qt/Mac to shift
   * the About, Preferences and Quit items to the Mac Application menu.
   * These items must be translated identically in both qt_ and qgis_ files.
   */
  QTranslator qttor(0);
  if (qttor.load(QString("qt_") + myTranslationCode, i18nPath))
  {
    a.installTranslator(&qttor);
  }

  /* Translation file for QGIS.
   */
  QTranslator qgistor(0);
  if (qgistor.load(QString("qgis_") + myTranslationCode, i18nPath))
  {
    a.installTranslator(&qgistor);
  }

  QgsHelpViewer w(context);
  w.show();

  a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));

  // Create socket for client to send context requests to.
  // This allows an existing viewer to be reused rather then creating
  // an additional viewer if one is already running.
  QgsHelpContextServer *helpServer = new QgsHelpContextServer();
  // Make port number available to client
  std::cout << helpServer->port() << std::endl; 
  // Pass context request from socket to viewer widget
  QObject::connect(helpServer, SIGNAL(setContext(const QString&)),
      &w, SLOT(setContext(const QString&)));

  return a.exec();
}
