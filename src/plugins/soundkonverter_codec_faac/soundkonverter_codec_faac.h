
#ifndef SOUNDKONVERTER_CODEC_FAAC_H
#define SOUNDKONVERTER_CODEC_FAAC_H

#include "../../core/codecplugin.h"

#include <QPointer>
#include <QDateTime>

class ConversionOptions;


class soundkonverter_codec_faac : public CodecPlugin
{
    Q_OBJECT
public:
    /** Default Constructor */
    soundkonverter_codec_faac( QObject *parent, const QVariantList& args );

    /** Default Destructor */
    ~soundkonverter_codec_faac();

    QString name() const override;
    int version();

    QList<ConversionPipeTrunk> codecTable() override;

    bool isConfigSupported( ActionType action, const QString& codecName ) override;
    void showConfigDialog( ActionType action, const QString& codecName, QWidget *parent ) override;
    bool hasInfo() override;
    void showInfo( QWidget *parent ) override;

    CodecWidget *newCodecWidget() override;

    int convert( const QUrl& inputFile, const QUrl& outputFile, const QString& inputCodec, const QString& outputCodec, const ConversionOptions *_conversionOptions, TagData *tags = 0, bool replayGain = false ) override;
    QStringList convertCommand( const QUrl& inputFile, const QUrl& outputFile, const QString& inputCodec, const QString& outputCodec, const ConversionOptions *_conversionOptions, TagData *tags = 0, bool replayGain = false ) override;
    float parseOutput( const QString& output ) override;

private:
    QPointer<QProcess> infoProcess;
    QString infoProcessOutputData;

    int configVersion;
    QDateTime faacLastModified;
    bool faacHasMp4Support;

private slots:
    void infoProcessOutput();
    void infoProcessExit( int exitCode, QProcess::ExitStatus exitStatus );
};

#endif // _SOUNDKONVERTER_CODEC_FAAC_H_
