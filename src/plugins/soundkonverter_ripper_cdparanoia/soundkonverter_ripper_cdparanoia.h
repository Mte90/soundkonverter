
#ifndef SOUNDKONVERTER_RIPPER_CDPARANOIA_H
#define SOUNDKONVERTER_RIPPER_CDPARANOIA_H

#include "../../core/ripperplugin.h"

#include <QUrl>
#include <QProcess>
#include <QList>
#include <QPointer>

class QDialog;
class QCheckBox;
class QComboBox;
class QSpinBox;


class soundkonverter_ripper_cdparanoia : public RipperPlugin
{
    Q_OBJECT
public:
    /** Default Constructor */
    soundkonverter_ripper_cdparanoia( QObject *parent, const QVariantList& args );

    /** Default Destructor */
    ~soundkonverter_ripper_cdparanoia();

    QString name() const override;

    QList<ConversionPipeTrunk> codecTable() override;

    bool isConfigSupported( ActionType action, const QString& codecName ) override;
    void showConfigDialog( ActionType action, const QString& codecName, QWidget *parent ) override;
    bool hasInfo() override;
    void showInfo( QWidget *parent ) override;

    int rip( const QString& device, int track, int tracks, const QUrl& outputFile ) override;
    QStringList ripCommand( const QString& device, int track, int tracks, const QUrl& outputFile ) override;
    float parseOutput( const QString& output, int *fromSector, int *toSector );
    float parseOutput( const QString& output ) override;

private slots:
    /** Get the process' output */
    void processOutput() override;

private:
    QPointer<QDialog> configDialog;
    QCheckBox *configDialogForceReadSpeedCheckBox;
    QSpinBox *configDialogForceReadSpeedSpinBox;
    QComboBox *configDialogForceEndiannessComboBox;
    QSpinBox *configDialogMaximumRetriesSpinBox;
    QCheckBox *configDialogEnableParanoiaCheckBox;
    QCheckBox *configDialogEnableExtraParanoiaCheckBox;

    int forceReadSpeed;
    int forceEndianness;
    int maximumRetries;
    bool enableParanoia;
    bool enableExtraParanoia;

private slots:
    void configDialogForceReadSpeedChanged( int state );
    void configDialogSave();
    void configDialogDefault();
};

#endif // SOUNDKONVERTER_RIPPER_CDPARANOIA_H
