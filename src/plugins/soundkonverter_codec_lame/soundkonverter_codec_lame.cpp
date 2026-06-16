
#include <QStandardPaths>
#include <QSettings>
#include <QRegularExpression>
#include <KLocalizedString>
#include "lamecodecglobal.h"

#include "soundkonverter_codec_lame.h"
#include "lameconversionoptions.h"
#include "lamecodecwidget.h"


#include <QWidget>
#include <QLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLocale>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QSlider>

soundkonverter_codec_lame::soundkonverter_codec_lame( QObject *parent, const QVariantList& args  )
    : CodecPlugin( parent )
{
    Q_UNUSED(args)

    configDialogStereoModeComboBox = 0;

    binaries["lame"] = "";

    allCodecs += "mp3";
    allCodecs += "mp2";
    allCodecs += "wav";

    QSettings conf(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/soundkonverterrc", QSettings::IniFormat);
    conf.beginGroup("Plugin-" + name());
    configVersion = conf.value( "configVersion", 0 ).toInt();
    stereoMode = conf.value( "stereoMode", "automatic" ).toString();
}

soundkonverter_codec_lame::~soundkonverter_codec_lame()
{}

QString soundkonverter_codec_lame::name() const
{
    return global_plugin_name;
}

QList<ConversionPipeTrunk> soundkonverter_codec_lame::codecTable()
{
    QList<ConversionPipeTrunk> table;
    ConversionPipeTrunk newTrunk;

    newTrunk.codecFrom = "wav";
    newTrunk.codecTo = "mp3";
    newTrunk.rating = 100;
    newTrunk.enabled = ( binaries["lame"] != "" );
    newTrunk.problemInfo = standardMessage( "encode_codec,backend", "mp3", "lame" ) + "\n" + standardMessage( "install_patented_backend", "lame" );
    newTrunk.data.hasInternalReplayGain = false;
    table.append( newTrunk );

    newTrunk.codecFrom = "mp3";
    newTrunk.codecTo = "wav";
    newTrunk.rating = 100;
    newTrunk.enabled = ( binaries["lame"] != "" );
    newTrunk.problemInfo = standardMessage( "decode_codec,backend", "mp3", "lame" ) + "\n" + standardMessage( "install_patented_backend", "lame" );
    newTrunk.data.hasInternalReplayGain = false;
    table.append( newTrunk );

    newTrunk.codecFrom = "mp3";
    newTrunk.codecTo = "mp3";
    newTrunk.rating = 100;
    newTrunk.enabled = ( binaries["lame"] != "" );
    newTrunk.problemInfo = standardMessage( "transcode_codec,backend", "mp3", "lame" ) + "\n" + standardMessage( "install_patented_backend", "lame" );
    newTrunk.data.hasInternalReplayGain = false;
    table.append( newTrunk );

    newTrunk.codecFrom = "mp2";
    newTrunk.codecTo = "wav";
    newTrunk.rating = 70;
    newTrunk.enabled = ( binaries["lame"] != "" );
    newTrunk.problemInfo = standardMessage( "decode_codec,backend", "mp2", "lame" ) + "\n" + standardMessage( "install_patented_backend", "lame" );
    newTrunk.data.hasInternalReplayGain = false;
    table.append( newTrunk );

    newTrunk.codecFrom = "mp2";
    newTrunk.codecTo = "mp3";
    newTrunk.rating = 70;
    newTrunk.enabled = ( binaries["lame"] != "" );
    newTrunk.problemInfo = standardMessage( "transcode_codec,backend", "mp2/mp3", "lame" ) + "\n" + standardMessage( "install_patented_backend", "lame" );
    newTrunk.data.hasInternalReplayGain = false;
    table.append( newTrunk );

    return table;
}

bool soundkonverter_codec_lame::isConfigSupported( ActionType action, const QString& codecName )
{
    Q_UNUSED(action)
    Q_UNUSED(codecName)

    return true;
}

void soundkonverter_codec_lame::showConfigDialog( ActionType action, const QString& codecName, QWidget *parent )
{
    Q_UNUSED(action)
    Q_UNUSED(codecName)

    if( !configDialog )
    {
        configDialog = new QDialog( parent );
        configDialog->setWindowTitle( i18n("Configure %1",*global_plugin_name) );

        QVBoxLayout *configDialogLayout = new QVBoxLayout( configDialog );

        QWidget *configDialogWidget = new QWidget( configDialog );
        QHBoxLayout *configDialogBox = new QHBoxLayout( configDialogWidget );
        QLabel *configDialogStereoModeLabel = new QLabel( i18n("Stereo mode:"), configDialogWidget );
        configDialogBox->addWidget( configDialogStereoModeLabel );
        configDialogStereoModeComboBox = new QComboBox( configDialogWidget );
        configDialogStereoModeComboBox->addItem( i18n("Automatic"), "automatic" );
        configDialogStereoModeComboBox->addItem( i18n("Joint Stereo"), "joint stereo" );
        configDialogStereoModeComboBox->addItem( i18n("Simple Stereo"), "simple stereo" );
        configDialogStereoModeComboBox->addItem( i18n("Forced Joint Stereo"), "forced joint stereo" );
        configDialogStereoModeComboBox->addItem( i18n("Dual Mono"), "dual mono" );
        configDialogBox->addWidget( configDialogStereoModeComboBox );
        configDialogWidget->setLayout( configDialogBox );
        configDialogLayout->addWidget( configDialogWidget );

        QDialogButtonBox *buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults );
        configDialogLayout->addWidget( buttonBox );

        connect( buttonBox, SIGNAL( accepted() ), this, SLOT( configDialogSave() ) );
        connect( buttonBox, SIGNAL( rejected() ), configDialog, SLOT( reject() ) );
        connect( buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *btn) {
            if( btn->text().contains("Defaults") )
                configDialogDefault();
        } );
    }
    configDialogStereoModeComboBox->setCurrentIndex( configDialogStereoModeComboBox->findData(stereoMode) );
    configDialog->show();
}

void soundkonverter_codec_lame::configDialogSave()
{
    if( configDialog )
    {
        stereoMode = configDialogStereoModeComboBox->itemData( configDialogStereoModeComboBox->currentIndex() ).toString();

        QSettings conf(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/soundkonverterrc", QSettings::IniFormat);
        conf.beginGroup("Plugin-" + name());
        conf.setValue( "stereoMode", stereoMode );

        configDialog->deleteLater();
    }
}

void soundkonverter_codec_lame::configDialogDefault()
{
    if( configDialog )
    {
        configDialogStereoModeComboBox->setCurrentIndex( configDialogStereoModeComboBox->findData("automatic") );
    }
}

bool soundkonverter_codec_lame::hasInfo()
{
    return true;
}

void soundkonverter_codec_lame::showInfo( QWidget *parent )
{
    QDialog *dialog = new QDialog( parent );
    dialog->setWindowTitle( i18n("About %1",*global_plugin_name) );

    QVBoxLayout *layout = new QVBoxLayout( dialog );
    QLabel *widget = new QLabel( dialog );

    widget->setText( i18n("LAME is a free high quality MP3 encoder.\nYou can get it at: http://lame.sourceforge.net") );

    layout->addWidget( widget );

    QDialogButtonBox *buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok );
    layout->addWidget( buttonBox );
    connect( buttonBox, SIGNAL( accepted() ), dialog, SLOT( accept() ) );

    dialog->setLayout( layout );
    dialog->show();
}

CodecWidget *soundkonverter_codec_lame::newCodecWidget()
{
    LameCodecWidget *widget = new LameCodecWidget();
    return qobject_cast<CodecWidget*>(widget);
}

int soundkonverter_codec_lame::convert( const QUrl& inputFile, const QUrl& outputFile, const QString& inputCodec, const QString& outputCodec, const ConversionOptions *_conversionOptions, TagData *tags, bool replayGain )
{
    QStringList command = convertCommand( inputFile, outputFile, inputCodec, outputCodec, _conversionOptions, tags, replayGain );
    if( command.isEmpty() )
        return BackendPlugin::UnknownError;

    CodecPluginItem *newItem = new CodecPluginItem( this );
    newItem->id = lastId++;
    newItem->process = new QProcess( newItem );
    newItem->process->setProcessChannelMode( QProcess::MergedChannels );
    connect( newItem->process, SIGNAL(readyRead()), this, SLOT(processOutput()) );
    connect( newItem->process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processExit(int,QProcess::ExitStatus)) );

    newItem->process->startCommand(command.join(" "));

    logCommand( newItem->id, command.join(" ") );

    backendItems.append( newItem );
    return newItem->id;
}

QStringList soundkonverter_codec_lame::convertCommand( const QUrl& inputFile, const QUrl& outputFile, const QString& inputCodec, const QString& outputCodec, const ConversionOptions *_conversionOptions, TagData *tags, bool replayGain )
{
    Q_UNUSED(inputCodec)
    Q_UNUSED(tags)
    Q_UNUSED(replayGain)

    if( !_conversionOptions )
        return QStringList();

    if( inputFile.isEmpty() )
        return QStringList();

    QStringList command;
    const ConversionOptions *conversionOptions = _conversionOptions;
    const LameConversionOptions *lameConversionOptions = 0;
    if( conversionOptions->pluginName == name() )
    {
        lameConversionOptions = dynamic_cast<const LameConversionOptions*>(conversionOptions);
    }

    if( outputCodec == "mp3" )
    {
        command += binaries["lame"];
        command += "--nohist";
        command += "--pad-id3v2";
        if( conversionOptions->pluginName == name() )
        {
            command += "-q";
            command += QString::number((int)conversionOptions->compressionLevel);
        }
//         if( conversionOptions->replaygain && replayGain )
//         {
//             command += "--replaygain-accurate";
//         }
//         else
//         {
//             command += "--noreplaygain";
//         }
        if( conversionOptions->pluginName != name() || !conversionOptions->cmdArguments.contains("replaygain") )
        {
            command += "--noreplaygain";
        }
        if( lameConversionOptions && lameConversionOptions->data.preset != LameConversionOptions::Data::UserDefined )
        {
            command += "--preset";
            if( lameConversionOptions->data.presetFast )
            {
                command += "fast";
            }
            if( lameConversionOptions->data.preset == LameConversionOptions::Data::Medium )
            {
                command += "medium";
            }
            else if( lameConversionOptions->data.preset == LameConversionOptions::Data::Standard )
            {
                command += "standard";
            }
            else if( lameConversionOptions->data.preset == LameConversionOptions::Data::Extreme )
            {
                command += "extreme";
            }
            else if( lameConversionOptions->data.preset == LameConversionOptions::Data::Insane )
            {
                command += "insane";
            }
            else if( lameConversionOptions->data.preset == LameConversionOptions::Data::SpecifyBitrate )
            {
                if( lameConversionOptions->data.presetBitrateCbr )
                {
                    command += "cbr";
                }
                command += QString::number(lameConversionOptions->data.presetBitrate);
            }
        }
        else
        {
            if( conversionOptions->qualityMode == ConversionOptions::Quality )
            {
                if( conversionOptions->pluginName != name() || !conversionOptions->cmdArguments.contains("--vbr-old") )
                {
                    command += "--vbr-new";
                }
                command += "-V";
                command += QString::number(conversionOptions->quality);
            }
            else if( conversionOptions->qualityMode == ConversionOptions::Bitrate )
            {
                if( conversionOptions->bitrateMode == ConversionOptions::Abr )
                {
                    command += "--abr";
                    command += QString::number(conversionOptions->bitrate);
                }
                else if( conversionOptions->bitrateMode == ConversionOptions::Cbr )
                {
                    command += "--cbr";
                    command += "-b";
                    command += QString::number(conversionOptions->bitrate);
                }
            }
        }
        if( stereoMode != "automatic" )
        {
            command += "-m";
            if( stereoMode == "joint stereo" )
            {
                command += "j";
            }
            else if( stereoMode == "simple stereo" )
            {
                command += "s";
            }
            else if( stereoMode == "forced joint stereo" )
            {
                command += "f";
            }
            else if( stereoMode == "dual mono" )
            {
                command += "d";
            }
        }
        if( conversionOptions->pluginName == name() )
        {
            command += conversionOptions->cmdArguments;
        }
        command += "\"" + escapeUrl(inputFile) + "\"";
        command += "\"" + escapeUrl(outputFile) + "\"";
    }
    else
    {
        command += binaries["lame"];
        command += "--decode";
        command += "\"" + escapeUrl(inputFile) + "\"";
        command += "\"" + escapeUrl(outputFile) + "\"";
    }

    return command;
}

float soundkonverter_codec_lame::parseOutput( const QString& output )
{
    // decoding
    // Frame#  1398/8202   256 kbps  L  R (...)

    // encoding
    // \r  3600/3696   (97%)|    0:05/    0:05|    0:05/    0:05|   18.190x|    0:00

    QString data = output;
    QString frame, count;

    if( output.contains("Frame#") )
    {
        data.remove( 0, data.indexOf("Frame#")+7 );
        frame = data.left( data.indexOf("/") );
        data.remove( 0, data.indexOf("/")+1 );
        count = data.left( data.indexOf(" ") );
        return frame.toFloat()/count.toFloat()*100.0f;
    }
    if( output.contains("%") )
    {
        frame = data.left( data.indexOf("/") );
        frame.remove( 0, frame.lastIndexOf(" ")+1 );
        data.remove( 0, data.indexOf("/")+1 );
        count = data.left( data.indexOf(" ") );
        return frame.toFloat()/count.toFloat()*100.0f;
    }
    /*if( output.contains("%") )
    {
        data.remove( 0, data.indexOf("(")+1 );
        data.remove( data.indexOf("%"), data.length()-data.indexOf("%") );
        return data.toFloat();
    }*/

    return -1;
}

ConversionOptions *soundkonverter_codec_lame::conversionOptionsFromXml( QDomElement conversionOptions, QList<QDomElement> *filterOptionsElements )
{
    LameConversionOptions *options = new LameConversionOptions();
    options->fromXml( conversionOptions, filterOptionsElements );
    return options;
}

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(soundkonverter_codec_lame, "codec_lame.json")

#include "soundkonverter_codec_lame.moc"
