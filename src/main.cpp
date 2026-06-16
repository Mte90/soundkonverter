#include "soundkonverterapp.h"
#include "soundkonverter.h"
#include "global.h"

#include <QApplication>
#include <QCommandLineParser>
#include <KLocalizedString>

static const char description[] =
    "soundKonverter is a frontend to various audio converters, "
    "Replay Gain tools and CD rippers.\n\n"
    "Please file bug reports at https://github.com/dfaust/soundkonverter/issues";

static const char version[] = SOUNDKONVERTER_VERSION_STRING;

int main(int argc, char **argv)
{
    KLocalizedString::setApplicationDomain("soundkonverter");

    soundKonverterApp app(argc, argv);
    QApplication::setApplicationName("soundkonverter");
    QApplication::setApplicationVersion(version);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", description));
    parser.addVersionOption();
    parser.addHelpOption();

    QCommandLineOption replayGainOption(QStringList() << "replaygain", QCoreApplication::translate("main", "Open the Replay Gain tool and add all given files"), "replaygain");
    parser.addOption(replayGainOption);

    QCommandLineOption ripOption(QStringList() << "rip", QCoreApplication::translate("main", "List all tracks on the cd drive <device>, 'auto' will search for a cd"), "rip");
    parser.addOption(ripOption);

    QCommandLineOption profileOption(QStringList() << "profile", QCoreApplication::translate("main", "Add all files using the given profile"), "profile");
    parser.addOption(profileOption);

    QCommandLineOption formatOption(QStringList() << "format", QCoreApplication::translate("main", "Add all files using the given format"), "format");
    parser.addOption(formatOption);

    QCommandLineOption outputOption(QStringList() << "output", QCoreApplication::translate("main", "Output all files to <directory>"), "output");
    parser.addOption(outputOption);

    QCommandLineOption invisibleOption(QStringList() << "invisible", QCoreApplication::translate("main", "Start soundKonverter invisible"));
    parser.addOption(invisibleOption);

    QCommandLineOption commandOption(QStringList() << "command", QCoreApplication::translate("main", "Execute <command> after each file has been converted (%i=input file, %o=output file)"), "command");
    parser.addOption(commandOption);

    QCommandLineOption fileListOption(QStringList() << "file-list", QCoreApplication::translate("main", "Load the file list at <path> after starting soundKonverter"), "file-list");
    parser.addOption(fileListOption);

    parser.addPositionalArgument("files", QCoreApplication::translate("main", "Audio file(s) to append to the file list"));

    soundKonverterApp::addCmdLineOptions(parser);

    parser.process(app);

    app.newInstance(parser);

    return app.exec();
}
