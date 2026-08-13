
#ifndef SOUNDKONVERTER_REPLAYGAIN_AACGAIN_H
#define SOUNDKONVERTER_REPLAYGAIN_AACGAIN_H

#include "../../core/replaygainplugin.h"

#include <QUrl>
#include <QPointer>

class ConversionOptions;
class QDialog;
class QComboBox;
class QCheckBox;
class QDoubleSpinBox;


class AacGainPluginItem : public ReplayGainPluginItem
{
    Q_OBJECT
public:
    explicit AacGainPluginItem( QObject *parent );
    ~AacGainPluginItem();

    QList<QUrl> undoFileList;
};


class soundkonverter_replaygain_aacgain : public ReplayGainPlugin
{
    Q_OBJECT
public:
    /** Default Constructor */
    soundkonverter_replaygain_aacgain( QObject *parent, const QVariantList& args );

    /** Default Destructor */
    ~soundkonverter_replaygain_aacgain();

    QString name() const override;

    QList<ReplayGainPipe> codecTable() override;

    bool isConfigSupported( ActionType action, const QString& codecName ) override;
    void showConfigDialog( ActionType action, const QString& codecName, QWidget *parent ) override;
    bool hasInfo() override;
    void showInfo( QWidget *parent ) override;

    int apply( const QList<QUrl>& fileList, ApplyMode mode = Add ) override;
    float parseOutput( const QString& output ) override;

private:
    QPointer<QDialog> configDialog;
    QComboBox *configDialogTagModeComboBox;
    QCheckBox *configDialogModifyAudioStreamCheckBox;
    QDoubleSpinBox *configDialogGainAdjustmentSpinBox;

    int tagMode;
    bool modifyAudioStream;
    double gainAdjustment;

private slots:
    /** The undo process has exited */
    virtual void undoProcessExit( int exitCode, QProcess::ExitStatus exitStatus );

    void configDialogSave();
    void configDialogDefault();

};

#endif // _SOUNDKONVERTER_REPLAYGAIN_AACGAIN_H_
