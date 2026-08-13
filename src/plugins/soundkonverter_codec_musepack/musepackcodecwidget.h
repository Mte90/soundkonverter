
#ifndef MUSEPACKCODECWIDGET_H
#define MUSEPACKCODECWIDGET_H

#include "../../core/codecwidget.h"

class QComboBox;
class KLineEdit;
class QSlider;
class QDoubleSpinBox;
class QGroupBox;
class QCheckBox;

class MusePackCodecWidget : public CodecWidget
{
    Q_OBJECT
public:
    MusePackCodecWidget();
    ~MusePackCodecWidget();

    ConversionOptions *currentConversionOptions() override;
    bool setCurrentConversionOptions( const ConversionOptions *_options ) override;
    void setCurrentFormat( const QString& format ) override;
    QString currentProfile() override;
    bool setCurrentProfile( const QString& profile ) override;
    int currentDataRate() override;

private:
    // preset selection
    QComboBox *cPreset;
    // user defined options
    QGroupBox *userdefinedBox;
    QSlider *sQuality;
    QDoubleSpinBox *dQuality;
    QCheckBox *cCmdArguments;
    KLineEdit *lCmdArguments;

    QString currentFormat; // holds the current output file format

    int bitrateForQuality( double quality );
    double qualityForBitrate( int bitrate );

private slots:
    // presets
    void presetChanged( const QString& preset );
    // user defined options
    void qualitySliderChanged( int quality );
    void qualitySpinBoxChanged( double quality );
};

#endif // MUSEPACKCODECWIDGET_H