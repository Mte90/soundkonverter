
#ifndef SOUNDKONVERTER_RIPPER_ICEDAX_H
#define SOUNDKONVERTER_RIPPER_ICEDAX_H

#include "../../core/ripperplugin.h"

#include <QUrl>
#include <QProcess>
#include <QList>


class soundkonverter_ripper_icedax : public RipperPlugin
{
    Q_OBJECT
public:
    /** Default Constructor */
    soundkonverter_ripper_icedax( QObject *parent, const QVariantList& args );

    /** Default Destructor */
    ~soundkonverter_ripper_icedax();

    QString name() const override;

    QList<ConversionPipeTrunk> codecTable() override;

    bool isConfigSupported( ActionType action, const QString& codecName ) override;
    void showConfigDialog( ActionType action, const QString& codecName, QWidget *parent ) override;
    bool hasInfo() override;
    void showInfo( QWidget *parent ) override;

    int rip( const QString& device, int track, int tracks, const QUrl& outputFile ) override;
    QStringList ripCommand( const QString& device, int track, int tracks, const QUrl& outputFile ) override;
    float parseOutput( const QString& output, RipperPluginItem *ripperItem );
    float parseOutput( const QString& output ) override;

private slots:
    /** Get the process' output */
    void processOutput() override;
};

#endif // SOUNDKONVERTER_RIPPER_ICEDAX_H
