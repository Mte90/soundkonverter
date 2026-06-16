/*
 * soundkonverter.cpp
 *
 * Copyright (C) 2007 Daniel Faust <hessijames@gmail.com>
 */
#include "soundkonverter.h"
#include "soundkonverterview.h"
#include "global.h"
#include "config.h"
#include "configdialog/configdialog.h"
#include "logger.h"
#include "logviewer.h"
#include "replaygainscanner/replaygainscanner.h"
#include "aboutplugins.h"

#include <taglib.h>

#include <QActionGroup>
#include <QAction>
#include <KActionMenu>
#include <QMenu>
#include <KLocalizedString>
#include <QToolBar>
#include <QIcon>
#include <QStandardPaths>
#include <QMenu>
#include <QMessageBox>
#include <QDir>
#include <QApplication>
#include <QSystemTrayIcon>
#include <QSharedPointer>

soundKonverter::soundKonverter()
    : KXmlGuiWindow(),
      cdManager( 0 ),
      logViewer( 0 ),
      systemTray( 0 ),
      autoclose( false )
{
    // accept dnd
    setAcceptDrops(true);

    const int fontHeight = QFontMetrics(QApplication::font()).boundingRect("M").size().height();

    logger = new Logger( this );
    logger->log( 1000, i18n("This is soundKonverter %1",*SOUNDKONVERTER_VERSION_STRING) );

    logger->log( 1000, "\n" + i18n("Compiled with TagLib %1.%2.%3",TAGLIB_MAJOR_VERSION,TAGLIB_MINOR_VERSION,TAGLIB_PATCH_VERSION) );

    config = new Config( logger, this );
    config->load();

    m_view = new soundKonverterView( logger, config, cdManager, this );
    connect( m_view, SIGNAL(signalConversionStarted()), this, SLOT(conversionStarted()) );
    connect( m_view, SIGNAL(signalConversionStopped(bool)), this, SLOT(conversionStopped(bool)) );
    connect( m_view, SIGNAL(progressChanged(const QString&)), this, SLOT(progressChanged(const QString&)) );
    connect( m_view, SIGNAL(showLog(int)), this, SLOT(showLogViewer(int)) );

    // tell the KXmlGuiWindow that this is indeed the main widget
    setCentralWidget( m_view );

    // then, setup our actions
    setupActions();

    // a call to KXmlGuiWindow::setupGUI() populates the GUI
    // with actions, using KXMLGUI.
    // It also applies the saved mainwindow settings, if any, and ask the
    // mainwindow to automatically save settings if changed: window size,
    // toolbar position, icon size, etc.
    setupGUI( QSize(70*fontHeight,45*fontHeight), ToolBar | Keys | Save | Create );
}

soundKonverter::~soundKonverter()
{
    if( logViewer )
        delete logViewer;

    if( replayGainScanner )
        delete replayGainScanner.data();

    if( systemTray )
        delete systemTray;
}

void soundKonverter::saveProperties( KConfigGroup& configGroup )
{
    Q_UNUSED(configGroup)

    m_view->killConversion();

    m_view->saveFileList( false );
}

void soundKonverter::showSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;
        
    systemTray = new QSystemTrayIcon( this );
    systemTray->setIcon(QIcon::fromTheme("soundkonverter"));
    systemTray->show();
}

void soundKonverter::addConvertFiles( const QStringList& urls, const QString& profile, const QString& format, const QString& directory, const QString& notifyCommand )
{
    QList<QUrl> urlList;
    for( const QString& url : urls )
        urlList.append(QUrl::fromUserInput(url));
    m_view->addConvertFiles( urlList, profile, format, directory, notifyCommand );
}

void soundKonverter::addReplayGainFiles( const QStringList& urls )
{
    showReplayGainScanner();
    QList<QUrl> urlList;
    for( const QString& url : urls )
        urlList.append(QUrl::fromUserInput(url));
    replayGainScanner.data()->addFiles( urlList );
}

bool soundKonverter::ripCd( const QString& device, const QString& profile, const QString& format, const QString& directory, const QString& notifyCommand )
{
    return m_view->showCdDialog( device != "auto" ? device : "", profile, format, directory, notifyCommand );
}

void soundKonverter::setupActions()
{
    // Quit action
    QAction *quitAction = new QAction(this);
    quitAction->setText(i18n("Quit"));
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
    actionCollection()->addAction("quit", quitAction);

    // Preferences action
    QAction *prefAction = new QAction(this);
    prefAction->setText(i18n("Preferences..."));
    connect(prefAction, &QAction::triggered, this, &soundKonverter::showConfigDialog);
    actionCollection()->addAction("preferences", prefAction);

    actionCollection()->addAction("stop_menu", m_view->stopMenu());

    QAction *logviewer = actionCollection()->addAction("logviewer");
    logviewer->setText(i18n("View logs..."));
    QIcon logviewerIcon = QIcon::fromTheme("view-list-text");
    logviewer->setIcon(logviewerIcon);
    connect( logviewer, SIGNAL(triggered()), this, SLOT(showLogViewer()) );

    QAction *replaygainscanner = actionCollection()->addAction("replaygainscanner");
    replaygainscanner->setText(i18n("Replay Gain tool..."));
    QIcon replaygainIcon = QIcon::fromTheme("soundkonverter-replaygain");
    replaygainscanner->setIcon(replaygainIcon);
    connect( replaygainscanner, SIGNAL(triggered()), this, SLOT(showReplayGainScanner()) );

    QAction *aboutplugins = actionCollection()->addAction("aboutplugins");
    aboutplugins->setText(i18n("About plugins..."));
    QIcon aboutpluginsIcon = QIcon::fromTheme("preferences-plugin");
    aboutplugins->setIcon(aboutpluginsIcon);
    connect( aboutplugins, SIGNAL(triggered()), this, SLOT(showAboutPlugins()) );

    QAction *add_files = actionCollection()->addAction("add_files");
    add_files->setText(i18n("Add files..."));
    QIcon addFilesIcon = QIcon::fromTheme("audio-x-generic");
    add_files->setIcon(addFilesIcon);
    connect( add_files, SIGNAL(triggered()), m_view, SLOT(showFileDialog()) );

    QAction *add_folder = actionCollection()->addAction("add_folder");
    add_folder->setText(i18n("Add folder..."));
    QIcon addFolderIcon = QIcon::fromTheme("folder");
    add_folder->setIcon(addFolderIcon);
    connect( add_folder, SIGNAL(triggered()), m_view, SLOT(showDirDialog()) );

    QAction *add_audiocd = actionCollection()->addAction("add_audiocd");
    add_audiocd->setText(i18n("Add CD tracks..."));
    QIcon addAudioCdIcon = QIcon::fromTheme("media-optical-audio");
    add_audiocd->setIcon(addAudioCdIcon);
    connect( add_audiocd, SIGNAL(triggered()), m_view, SLOT(showCdDialog()) );

    QAction *add_url = actionCollection()->addAction("add_url");
    add_url->setText(i18n("Add url..."));
    QIcon addUrlIcon = QIcon::fromTheme("network-workgroup");
    add_url->setIcon(addUrlIcon);
    connect( add_url, SIGNAL(triggered()), m_view, SLOT(showUrlDialog()) );

    QAction *add_playlist = actionCollection()->addAction("add_playlist");
    add_playlist->setText(i18n("Add playlist..."));
    QIcon addPlaylistIcon = QIcon::fromTheme("view-media-playlist");
    add_playlist->setIcon(addPlaylistIcon);
    connect( add_playlist, SIGNAL(triggered()), m_view, SLOT(showPlaylistDialog()) );

    QAction *load = actionCollection()->addAction("load");
    load->setText(i18n("Load file list"));
    QIcon loadIcon = QIcon::fromTheme("document-open");
    load->setIcon(loadIcon);
    connect( load, SIGNAL(triggered()), m_view, SLOT(loadFileList()) );

    QAction *save = actionCollection()->addAction("save");
    save->setText(i18n("Save file list"));
    QIcon saveIcon = QIcon::fromTheme("document-save");
    save->setIcon(saveIcon);
    connect( save, SIGNAL(triggered()), m_view, SLOT(saveFileList()) );

    QAction *startAct = m_view->start();
    actionCollection()->addAction("start", startAct);
}

void soundKonverter::showConfigDialog()
{
    ConfigDialog *dialog = new ConfigDialog( config, this/*, ConfigDialog::Page(configStartPage)*/ );
    connect( dialog, SIGNAL(updateFileList()), m_view, SLOT(updateFileList()) );

    dialog->resize( size() );
    dialog->exec();

    delete dialog;
}

void soundKonverter::showLogViewer( const int logId )
{
    if( !logViewer )
        logViewer = new LogViewer( logger, 0 );

    if( logId )
        logViewer->showLog( logId );

    logViewer->show();
    logViewer->raise();
}

void soundKonverter::showReplayGainScanner()
{
    if( !replayGainScanner )
    {
        replayGainScanner.reset(new ReplayGainScanner( config, logger, !isVisible(), 0 ));
        connect( replayGainScanner.data(), SIGNAL(finished()), this, SLOT(replayGainScannerClosed()) );
        connect( replayGainScanner.data(), SIGNAL(showMainWindow()), this, SLOT(showMainWindow()) );
    }

    replayGainScanner.data()->setAttribute( Qt::WA_DeleteOnClose );

    replayGainScanner.data()->show();
    replayGainScanner.data()->raise();
    replayGainScanner.data()->activateWindow();
}

void soundKonverter::replayGainScannerClosed()
{
    if( !isVisible() )
        QApplication::quit();
}

void soundKonverter::showMainWindow()
{
    show();
}

void soundKonverter::showAboutPlugins()
{
    AboutPlugins *dialog = new AboutPlugins( config, this );
    dialog->exec();
    dialog->deleteLater();
}

void soundKonverter::startConversion()
{
    m_view->startConversion();
}

void soundKonverter::loadAutosaveFileList()
{
    m_view->loadAutosaveFileList();
}

void soundKonverter::loadFileList(const QString& fileListPath)
{
    m_view->loadFileList(fileListPath);
}

void soundKonverter::startupChecks()
{
    // check if codec plugins could be loaded
    if( config->pluginLoader()->getAllCodecPlugins().count() == 0 )
    {
        QMessageBox::critical(this, i18n("Error"), i18n("No codec plugins could be loaded. Without codec plugins soundKonverter can't work.\nThis problem can have two causes:\n1. You just installed soundKonverter and the system configuration cache is not up-to-date, yet.\nIn this case, run kbuildsycoca5 and restart soundKonverter to fix the problem.\n2. Your installation is broken.\nIn this case try reinstalling soundKonverter."));
    }

    // remove old KDE4 action menus created by soundKonverter 0.3 - don't change the paths, it's what soundKonverter 0.3 used
    if( config->data.app.configVersion < 1001 )
    {
        if( QFile::exists(QDir::homePath()+"/.kde4/share/kde4/services/ServiceMenus/convert_with_soundkonverter.desktop") )
        {
            QFile::remove(QDir::homePath()+"/.kde4/share/kde4/services/ServiceMenus/convert_with_soundkonverter.desktop");
            logger->log( 1000, i18n("Removing old file: %1",QDir::homePath()+"/.kde4/share/kde4/services/ServiceMenus/convert_with_soundkonverter.desktop") );
        }
        if( QFile::exists(QDir::homePath()+"/.kde4/share/kde4/services/ServiceMenus/add_replaygain_with_soundkonverter.desktop") )
        {
            QFile::remove(QDir::homePath()+"/.kde4/share/kde4/services/ServiceMenus/add_replaygain_with_soundkonverter.desktop");
            logger->log( 1000, i18n("Removing old file: %1",QDir::homePath()+"/.kde4/share/kde4/services/ServiceMenus/add_replaygain_with_soundkonverter.desktop") );
        }
    }

    // clean up log directory
    QDir dir( QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/log/" );
    dir.setFilter( QDir::Files | QDir::Writable );

    QStringList list = dir.entryList();

    for( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
    {
        if( *it != "1000.log" && (*it).endsWith(".log") )
        {
            QFile::remove( dir.absolutePath() + "/" + (*it) );
            logger->log( 1000, i18n("Removing old file: %1",dir.absolutePath()+"/"+(*it)) );
        }
    }

    // check if new backends got installed and the backend settings can be optimized
    QList<CodecOptimizations::Optimization> optimizationList = config->getOptimizations();
    if( !optimizationList.isEmpty() )
    {
        CodecOptimizations *optimizationsDialog = new CodecOptimizations( optimizationList, this );
        connect( optimizationsDialog, SIGNAL(solutions(const QList<CodecOptimizations::Optimization>&)), config, SLOT(doOptimizations(const QList<CodecOptimizations::Optimization>&)) );
        optimizationsDialog->open();
    }
}

void soundKonverter::conversionStarted()
{
    if( systemTray )
    {
        systemTray->setToolTip(i18n("Converting") + ": 0%");
    }
}

void soundKonverter::conversionStopped( bool failed )
{
    if( autoclose && !failed /*&& !m_view->isVisible()*/ )
        QApplication::quit(); // close app on conversion stop unless the conversion was stopped by the user or the window is shown

    if( systemTray )
    {
        systemTray->setToolTip(i18n("Finished"));
    }
}

void soundKonverter::progressChanged( const QString& progress )
{
    setWindowTitle( progress + " - soundKonverter" );

    if( systemTray )
    {
        systemTray->setToolTip("soundkonverter");
    }
}


#include "soundkonverter.moc"
