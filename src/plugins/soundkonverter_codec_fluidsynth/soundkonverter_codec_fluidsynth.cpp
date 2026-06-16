
#include <QStandardPaths>
#include <QSettings>
#include <QRegularExpression>
#include <KLocalizedString>
#include "fluidsynthcodecglobal.h"

#include "soundkonverter_codec_fluidsynth.h"
#include "../../core/conversionoptions.h"
#include "fluidsynthcodecwidget.h"

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>


soundkonverter_codec_fluidsynth::soundkonverter_codec_fluidsynth( QObject *parent, const QVariantList& args  )
    : CodecPlugin( parent )
{
    Q_UNUSED(args)

    configDialogSoundFontLineEdit = 0;

    binaries["fluidsynth"] = "";

    allCodecs += "midi";
    allCodecs += "mod";
    allCodecs += "wav";

    QSettings conf(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/soundkonverterrc", QSettings::IniFormat);
    conf.beginGroup("Plugin-" + name());
    soundFontFile = conf.value( "soundFontFile", QUrl() ).toUrl();
}

soundkonverter_codec_fluidsynth::~soundkonverter_codec_fluidsynth()
{}

QString soundkonverter_codec_fluidsynth::name() const
{
    return global_plugin_name;
}

QList<ConversionPipeTrunk> soundkonverter_codec_fluidsynth::codecTable()
{
    QList<ConversionPipeTrunk> table;
    ConversionPipeTrunk newTrunk;

    newTrunk.codecFrom = "midi";
    newTrunk.codecTo = "wav";
    newTrunk.rating = 90;
    newTrunk.enabled = ( binaries["fluidsynth"] != "" );
    newTrunk.problemInfo = standardMessage( "decode_codec,backend", "midi", "fluidsynth" ) + "\n" + standardMessage( "install_opensource_backend", "fluidsynth" );
    newTrunk.data.hasInternalReplayGain = false;
    table.append( newTrunk );

    newTrunk.codecFrom = "mod";
    newTrunk.codecTo = "wav";
    newTrunk.rating = 90;
    newTrunk.enabled = ( binaries["fluidsynth"] != "" );
    newTrunk.problemInfo = standardMessage( "decode_codec,backend", "mod", "fluidsynth" ) + "\n" + standardMessage( "install_opensource_backend", "fluidsynth" );
    newTrunk.data.hasInternalReplayGain = false;
    table.append( newTrunk );

    return table;
}

bool soundkonverter_codec_fluidsynth::isConfigSupported( ActionType action, const QString& codecName )
{
    Q_UNUSED(action)
    Q_UNUSED(codecName)

    return true;
}

void soundkonverter_codec_fluidsynth::showConfigDialog( ActionType action, const QString& codecName, QWidget *parent )
{
    Q_UNUSED(action)
    Q_UNUSED(codecName)

    if( !configDialog )
    {
        const int fontHeight = QFontMetrics(QApplication::font()).boundingRect("M").size().height();

        configDialog = new QDialog( parent );
        configDialog->setWindowTitle( i18n("Configure %1",*global_plugin_name) );

        QVBoxLayout *configDialogLayout = new QVBoxLayout( configDialog );

        QWidget *configDialogWidget = new QWidget( configDialog );
        QHBoxLayout *configDialogBox = new QHBoxLayout( configDialogWidget );
        QLabel *configDialogSoundFontLabel = new QLabel( i18n("Use SoundFont file:"), configDialogWidget );
        configDialogSoundFontLabel->setToolTip( i18n("In order to convert the midi data to a wave form you need a SoundFont which maps the midi data to sound effects.\nHave a look at %1 in order to get SoundFont files.",QString("http://sourceforge.net/apps/trac/fluidsynth/wiki/SoundFont")) );
        configDialogBox->addWidget( configDialogSoundFontLabel );

        configDialogSoundFontLineEdit = new QLineEdit( configDialogWidget );
        configDialogSoundFontLineEdit->setMinimumWidth( 30*fontHeight );
        configDialogSoundFontLineEdit->setReadOnly( true );
        configDialogBox->addWidget( configDialogSoundFontLineEdit );

        QPushButton *browseButton = new QPushButton( i18n("Browse..."), configDialogWidget );
        configDialogBox->addWidget( browseButton );

        connect( browseButton, &QPushButton::clicked, this, [this]() {
            QString fileName = QFileDialog::getOpenFileName( configDialog, i18n("Select SoundFont File"), QString(), i18n("SoundFont Files (*.sf2 *.sf3)") );
            if( !fileName.isEmpty() )
                configDialogSoundFontLineEdit->setText( fileName );
        } );

        configDialogWidget->setLayout( configDialogBox );
        configDialogLayout->addWidget( configDialogWidget );

        QDialogButtonBox *buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
        configDialogLayout->addWidget( buttonBox );

        connect( buttonBox, SIGNAL( accepted() ), this, SLOT( configDialogSave() ) );
        connect( buttonBox, SIGNAL( rejected() ), configDialog, SLOT( reject() ) );
    }
    configDialogSoundFontLineEdit->setText( soundFontFile.toLocalFile() );
    configDialog->show();
}

void soundkonverter_codec_fluidsynth::configDialogSave()
{
    if( configDialog )
    {
        soundFontFile = QUrl::fromLocalFile( configDialogSoundFontLineEdit->text() );

        QSettings conf(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/soundkonverterrc", QSettings::IniFormat);
        conf.beginGroup("Plugin-" + name());
        conf.setValue( "soundFontFile", soundFontFile );

        configDialog->deleteLater();
        configDialog = 0;
    }
}

bool soundkonverter_codec_fluidsynth::hasInfo()
{
    return false;
}

void soundkonverter_codec_fluidsynth::showInfo( QWidget *parent )
{
    Q_UNUSED(parent)
}

CodecWidget *soundkonverter_codec_fluidsynth::newCodecWidget()
{
    FluidsynthCodecWidget *widget = new FluidsynthCodecWidget();
    return qobject_cast<CodecWidget*>(widget);
}

int soundkonverter_codec_fluidsynth::convert( const QUrl& inputFile, const QUrl& outputFile, const QString& inputCodec, const QString& outputCodec, const ConversionOptions *_conversionOptions, TagData *tags, bool replayGain )
{
    if( soundFontFile.isEmpty() )
    {
        emit log( 1000, i18n("FluidSynth is not configured, yet. You need to set a SoundFont file.") );
        return BackendPlugin::BackendNeedsConfiguration;
    }

    const QStringList command = convertCommand( inputFile, outputFile, inputCodec, outputCodec, _conversionOptions, tags, replayGain );
    if( command.isEmpty() )
    {
        return BackendPlugin::UnknownError;
    }

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

QStringList soundkonverter_codec_fluidsynth::convertCommand( const QUrl& inputFile, const QUrl& outputFile, const QString& inputCodec, const QString& outputCodec, const ConversionOptions *_conversionOptions, TagData *tags, bool replayGain )
{
    Q_UNUSED(inputCodec)
    Q_UNUSED(_conversionOptions)
    Q_UNUSED(tags)
    Q_UNUSED(replayGain)

    if( soundFontFile.isEmpty() )
        return QStringList();

    if( outputFile.isEmpty() )
        return QStringList();

    QStringList command;

    if( outputCodec == "wav" )
    {
        command += binaries["fluidsynth"];
        command += "-l";
        command += "--fast-render";
        command += "\"" + escapeUrl(outputFile) + "\"";
        command += "\"" + escapeUrl(soundFontFile) + "\"";
        command += "\"" + escapeUrl(inputFile) + "\"";
    }

    return command;
}

float soundkonverter_codec_fluidsynth::parseOutput( const QString& output )
{
    Q_UNUSED(output)

    // no output

    return -1;
}

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(soundkonverter_codec_fluidsynth, "codec_fluidsynth.json")

#include "soundkonverter_codec_fluidsynth.moc"
