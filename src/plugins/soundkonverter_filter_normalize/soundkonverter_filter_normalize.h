
#ifndef SOUNDKONVERTER_FILTER_NORMALIZE_H
#define SOUNDKONVERTER_FILTER_NORMALIZE_H

#include "../../core/filterplugin.h"

class FilterOptions;


class soundkonverter_filter_normalize : public FilterPlugin
{
    Q_OBJECT
public:
    /** Default Constructor */
    soundkonverter_filter_normalize( QObject *parent, const QVariantList& args );

    /** Default Destructor */
    ~soundkonverter_filter_normalize();

    QString name() const override;

    QList<ConversionPipeTrunk> codecTable() override;

    bool isConfigSupported( ActionType action, const QString& codecName ) override;
    void showConfigDialog( ActionType action, const QString& codecName, QWidget *parent ) override;
    bool hasInfo() override;
    void showInfo( QWidget *parent ) override;

    CodecWidget *newCodecWidget() override;
    FilterWidget *newFilterWidget() override;

    int convert( const QUrl& inputFile, const QUrl& outputFile, const QString& inputCodec, const QString& outputCodec, const ConversionOptions *_conversionOptions, TagData *tags = 0, bool replayGain = false ) override;
    QStringList convertCommand( const QUrl& inputFile, const QUrl& outputFile, const QString& inputCodec, const QString& outputCodec, const ConversionOptions *_conversionOptions, TagData *tags = 0, bool replayGain = false ) override;
    float parseOutput( const QString& output ) override;

    FilterOptions *filterOptionsFromXml( QDomElement filterOptions ) override;
};

#endif // SOUNDKONVERTER_FILTER_NORMALIZE_H
