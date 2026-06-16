#include "soundkonverterapp.h"
#include "soundkonverter.h"

#include <QStandardPaths>
#include <KLocalizedString>
#include <QFile>
#include <QCommandLineOption>

soundKonverterApp::soundKonverterApp(int &argc, char **argv)
    : QApplication(argc, argv)
{
    mainWindow = new soundKonverter();
    setActiveWindow(mainWindow);
}

soundKonverterApp::~soundKonverterApp()
{}

void soundKonverterApp::addCmdLineOptions(QCommandLineParser &parser)
{
    QStringList autostartNames;
    autostartNames << QStringLiteral("autostart");
    QCommandLineOption autostartOption(autostartNames, i18n("Start the conversion immediately (enabled when using '--invisible')"), "autostart");
    parser.addOption(autostartOption);

    QStringList autocloseNames;
    autocloseNames << QStringLiteral("autoclose");
    QCommandLineOption autocloseOption(autocloseNames, i18n("Close soundKonverter after all files are converted (enabled when using '--invisible')"), "autoclose");
    parser.addOption(autocloseOption);
}

int soundKonverterApp::newInstance(QCommandLineParser &parser)
{
    static bool first = true;
    bool visible = true;
    bool autoclose = false;
    bool autostart = false;
    bool activateMainWindow = true;

    if ((first || !mainWindow->isVisible()) && parser.isSet("replaygain") && parser.positionalArguments().count() > 0)
        visible = false;

    autoclose = parser.isSet("autoclose");
    autostart = parser.isSet("autostart");

    const QString profile = parser.value("profile");
    const QString format = parser.value("format");
    const QString directory = parser.value("output");
    const QString notifyCommand = parser.value("command");
    const QString fileListPath = parser.value("file-list");

    if (parser.isSet("invisible"))
    {
        autoclose = true;
        autostart = true;
        visible = false;
        mainWindow->showSystemTray();
    }

    if (first && fileListPath.isEmpty() && QFile::exists(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/soundkonverter/filelist_autosave.xml"))
    {
        if (!visible)
        {
            visible = true;
            autoclose = false;
            autostart = false;
            mainWindow->show();
        }
        mainWindow->show();
        mainWindow->loadAutosaveFileList();
    }
    else if (!fileListPath.isEmpty() && QFile::exists(fileListPath))
    {
        mainWindow->loadFileList(fileListPath);
    }

    const QString device = parser.value("rip");
    if (!device.isEmpty())
    {
        const bool success = mainWindow->ripCd(device, profile, format, directory, notifyCommand);
        if (!success && first)
        {
            quit();
            return 0;
        }
    }

    if (visible)
        mainWindow->show();

    mainWindow->setAutoClose(autoclose);

    if (parser.isSet("replaygain"))
    {
        QStringList urls = parser.positionalArguments();
        if (!urls.isEmpty())
        {
            mainWindow->addReplayGainFiles(urls);
            activateMainWindow = false;
        }
    }
    else
    {
        QStringList urls = parser.positionalArguments();
        if (!urls.isEmpty())
            mainWindow->addConvertFiles(urls, profile, format, directory, notifyCommand);
    }

    first = false;

    if (activateMainWindow)
        mainWindow->activateWindow();

    if (autostart)
        mainWindow->startConversion();

    return 0;
}
