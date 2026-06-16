#ifndef SOUNDKONVERTERAPP_H
#define SOUNDKONVERTERAPP_H

#include <QApplication>
#include <QCommandLineParser>

class soundKonverter;

class soundKonverterApp : public QApplication
{
    Q_OBJECT
public:
    /** Constructor */
    soundKonverterApp(int &argc, char **argv);

    /** Destructor */
    ~soundKonverterApp();

    /** This function is called, when a new instance of soundKonverter should be created */
    virtual int newInstance(QCommandLineParser &parser);

    /** Add soundKonverter-specific CLI options to parser */
    static void addCmdLineOptions(QCommandLineParser &parser);

private:
    soundKonverter *mainWindow;
};

#endif // SOUNDKONVERTERAPP_H
