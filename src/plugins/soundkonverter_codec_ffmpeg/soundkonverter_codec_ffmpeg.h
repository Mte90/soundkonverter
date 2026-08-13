
#ifndef SOUNDKONVERTER_CODEC_FFMPEG_H
#define SOUNDKONVERTER_CODEC_FFMPEG_H

#include "../../core/codecplugin.h"

#include <QPointer>
#include <QDateTime>

class ConversionOptions;
class QDialog;
class QCheckBox;


class soundkonverter_codec_ffmpeg : public CodecPlugin
{
    Q_OBJECT
public:
    struct FFmpegEncoderData
    {
        QString name;
        bool experimental = false;
    };

    struct CodecData
    {
        QString codecName;
        QList<FFmpegEncoderData> ffmpegEnoderList;
        FFmpegEncoderData currentFFmpegEncoder;
    };

    /** Default Constructor */
    soundkonverter_codec_ffmpeg( QObject *parent, const QVariantList& args );

    /** Default Destructor */
    ~soundkonverter_codec_ffmpeg();

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
    float parseOutput( const QString& output, int *length );
    float parseOutput( const QString& output ) override;

private:
    QList<CodecData> codecList;
    QPointer<QProcess> infoProcess;
    QString infoProcessOutputData;

    QPointer<QDialog> configDialog;
    QCheckBox *configDialogExperimantalCodecsEnabledCheckBox;

    int configVersion;
    bool experimentalCodecsEnabled;
    int ffmpegVersionMajor;
    int ffmpegVersionMinor;
    QDateTime ffmpegLastModified;
    QSet<QString> ffmpegCodecList;

private slots:
    /** Get the process' output */
    void processOutput() override;

    void configDialogSave();
    void configDialogDefault();

    void infoProcessOutput();
    void infoProcessExit( int exitCode, QProcess::ExitStatus exitStatus );
};

#endif // _SOUNDKONVERTER_CODEC_FFMPEG_H_
