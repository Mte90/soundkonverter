
#ifndef SOUNDKONVERTER_CODEC_MUSEPACK_H
#define SOUNDKONVERTER_CODEC_MUSEPACK_H

#include "../../core/codecplugin.h"

class ConversionOptions;


class soundkonverter_codec_musepack : public CodecPlugin
{
    Q_OBJECT
public:
    /** Default Constructor */
    soundkonverter_codec_musepack( QObject *parent, const QVariantList& args );

    /** Default Destructor */
    ~soundkonverter_codec_musepack();

    QString name() const override;

    /** search for the backend binaries in the given directories */
    void scanForBackends( const QStringList& directoryList = QStringList() ) override;

    QList<ConversionPipeTrunk> codecTable() override;

    bool isConfigSupported( ActionType action, const QString& codecName ) override;
    void showConfigDialog( ActionType action, const QString& codecName, QWidget *parent ) override;
    bool hasInfo() override;
    void showInfo( QWidget *parent ) override;
    CodecWidget *newCodecWidget() override;

    int convert( const QUrl& inputFile, const QUrl& outputFile, const QString& inputCodec, const QString& outputCodec, const ConversionOptions *_conversionOptions, TagData *tags = 0, bool replayGain = false ) override;
    QStringList convertCommand( const QUrl& inputFile, const QUrl& outputFile, const QString& inputCodec, const QString& outputCodec, const ConversionOptions *_conversionOptions, TagData *tags = 0, bool replayGain = false ) override;
    float parseOutput( const QString& output ) override;

    ConversionOptions *conversionOptionsFromXml( QDomElement conversionOptions, QList<QDomElement> *filterOptionsElements = 0 ) override;
};

#endif // SOUNDKONVERTER_CODEC_MUSEPACK_H
