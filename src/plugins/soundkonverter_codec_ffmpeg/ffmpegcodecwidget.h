
#ifndef FFMPEGCODECWIDGET_H
#define FFMPEGCODECWIDGET_H

#include "../../core/codecwidget.h"

class QLabel;
class QSlider;
class QSpinBox;
class QCheckBox;
class QComboBox;
class KLineEdit;

class FFmpegCodecWidget : public CodecWidget
{
    Q_OBJECT
public:
    FFmpegCodecWidget();
    ~FFmpegCodecWidget();

    ConversionOptions *currentConversionOptions() override;
    bool setCurrentConversionOptions( const ConversionOptions *_options ) override;
    void setCurrentFormat( const QString& format ) override;
    QString currentProfile() override;
    bool setCurrentProfile( const QString& profile ) override;
    int currentDataRate() override;

private:
    QLabel *lBitrate;
    QSlider *sBitrate;
    QSpinBox *iBitrate;
    QComboBox *cBitrate;
    QCheckBox *cCmdArguments;
    KLineEdit *lCmdArguments;

    QString currentFormat; // holds the current output file format

private slots:
    void qualitySliderChanged( int bitrate );
    void qualitySpinBoxChanged( int bitrate );
};

#endif // FFMPEGCODECWIDGET_H