#include "qgsapplication.h"
//#include "examp2.h"
#include "mymainwindow.h"
#include <QLocale>
#include <QTranslator>
#include <QLibraryInfo>
#include <qsplashscreen.h>
#include <QtTest/qtest.h>

int main(int argc, char *argv[])
{
    QgsApplication a(argc,argv,true);
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForLocale(QTextCodec::codecForLocale());
    QTextCodec::setCodecForCStrings(QTextCodec::codecForLocale());

    QSplashScreen *splash = new QSplashScreen;
    splash->setPixmap(QPixmap(":/Resources/BFMapMaker.bmp"));
    splash->show();
    Qt::Alignment topRight = Qt::AlignRight | Qt::AlignTop;
    splash->showMessage(QObject::tr("正在加载主界面..."),
                        topRight,
                        Qt::gray);
    QTest::qSleep(3000);
    delete splash;
    QString i18nPath = QgsApplication::i18nPath();
    QString myTranslationCode = QLocale::system().name();
    QTranslator qgistor( 0 );
    QTranslator qttor( 0 );
    if ( myTranslationCode != "C" )
    {
        if ( qgistor.load( QString( "qgis_" ) + myTranslationCode, i18nPath ) )
        {
            a.installTranslator( &qgistor );
        }
        else
        {
            qWarning( "loading of qgis translation failed [%s]", QString( "%1/qgis_%2" ).arg( i18nPath ).arg( myTranslationCode ).toLocal8Bit().constData() );
        }

        /* Translation file for Qt.
         * The strings from the QMenuBar context section are used by Qt/Mac to shift
         * the About, Preferences and Quit items to the Mac Application menu.
         * These items must be translated identically in both qt_ and qgis_ files.
         */
        if ( qttor.load( QString( "qt_" ) + myTranslationCode, QLibraryInfo::location( QLibraryInfo::TranslationsPath ) ) )
        {
            a.installTranslator( &qttor );
        }
        else
        {
            qWarning( "loading of qt translation failed [%s]", QString( "%1/qt_%2" ).arg( QLibraryInfo::location( QLibraryInfo::TranslationsPath ) ).arg( myTranslationCode ).toLocal8Bit().constData() );
        }
    }

    QStringList strList;
    strList.append("..\\symbol");
    a.setDefaultSvgPaths(strList);
    myMainWindow *w = new myMainWindow;
    w->showMaximized();

    return a.exec();
}
