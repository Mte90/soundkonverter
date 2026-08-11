
#ifndef SOUNDKONVERTER_CODEC_AFTEN_H
#define SOUNDKONVERTER_CODEC_AFTEN_H

#include "../../core/codecplugin.h"

class ConversionOptions;


class soundkonverter_codec_aften : public CodecPlugin
{
    Q_OBJECT
public:
    /** Default Constructor */
    soundkonverter_codec_aften( QObject *parent, const QVariantList& args );

    /** Default Destructor */
    ~soundkonverter_codec_aften();

    QString name() const override;

    QList<ConversionPipeTrunk> codecTable() override;

    bool isConfigSupported( ActionType action, const QString& codecName ) override;
    void showConfigDialog( ActionType action, const QString& codecName, QWidget *parent ) override;
    bool hasInfo() override;
    void showInfo( QWidget *parent ) override;

    CodecWidget *newCodecWidget() override;

    int convert( const QUrl& inputFile, const QUrl& outputFile, const QString& inputCodec, const QString& outputCodec, const ConversionOptions *_conversionOptions, TagData *tags = 0, bool replayGain = false ) override;
    QStringList convertCommand( const QUrl& inputFile, const QUrl& outputFile, const QString& inputCodec, const QString& outputCodec, const ConversionOptions *_conversionOptions, TagData *tags = 0, bool replayGain = false ) override;
    float parseOutput( const QString& output ) override;
};


#endif // _SOUNDKONVERTER_CODEC_AFTEN_H_
