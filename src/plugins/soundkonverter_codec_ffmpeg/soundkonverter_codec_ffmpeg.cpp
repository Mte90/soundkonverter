
#include <QStandardPaths>
#include <QSettings>
#include <QRegularExpression>
#include <KLocalizedString>
#include "ffmpegcodecglobal.h"

#include "soundkonverter_codec_ffmpeg.h"
#include "ffmpegcodecwidget.h"
#include "../../core/conversionoptions.h"
#include "../../metadata/tagengine.h"

#include <QMessageBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QFileInfo>


// TODO check for decoders at runtime, too

soundkonverter_codec_ffmpeg::soundkonverter_codec_ffmpeg( QObject *parent, const QVariantList& args  )
    : CodecPlugin( parent )
{
    Q_UNUSED(args)

    configDialogExperimantalCodecsEnabledCheckBox = 0;

    binaries["ffmpeg"] = "";

    QSettings conf(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/soundkonverterrc", QSettings::IniFormat);
    conf.beginGroup("Plugin-" + name());
    configVersion = conf.value("configVersion", 0).toInt();
    experimentalCodecsEnabled = conf.value("experimentalCodecsEnabled", false).toBool();
    ffmpegVersionMajor = conf.value("ffmpegVersionMajor", 0).toInt();
    ffmpegVersionMinor = conf.value("ffmpegVersionMinor", 0).toInt();
    ffmpegLastModified = conf.value("ffmpegLastModified", QDateTime()).toDateTime();
    const QStringList codecListTmp = conf.value("codecList", QStringList()).toStringList();
    ffmpegCodecList = QSet<QString>(codecListTmp.begin(), codecListTmp.end());

    // WARNING enabled codecs need to be rescanned everytime new codecs are added here -> increase plugin version

    QHash<QString, QStringList> codecs;

    codecs.insert("wav",        QStringList() << "wav");
    codecs.insert("ogg vorbis", QStringList() << "libvorbis" << "vorbis");
    codecs.insert("opus",       QStringList() << "libopus");
    codecs.insert("mp3",        QStringList() << "libmp3lame");
    codecs.insert("flac",       QStringList() << "flac");
    codecs.insert("wma",        QStringList() << "wmav2" << "wmav1");
    codecs.insert("aac",        QStringList() << "aac"); // libfaac, libvo_aacenc
    codecs.insert("m4a/aac",    QStringList() << "aac"); // libfaac, libvo_aacenc
    codecs.insert("ac3",        QStringList() << "ac3");
    codecs.insert("m4a/alac",   QStringList() << "alac");
    codecs.insert("mp2",        QStringList() << "mp2" << "libtwolame"); // mp2fixed
//     codecs.insert("amr nb",     QStringList() << "libopencore_amrnb"); // Only 8000Hz sample rate supported
    codecs.insert("wavpack",    QStringList() << "wavpack");
    codecs.insert("speex",      QStringList() << "libspeex");
    codecs.insert("tta",        QStringList() << "tta");
    codecs.insert("ra",         QStringList() << "real_144");

    for(const QString& codecName : codecs.keys())
    {
        CodecData data;
        data.codecName = codecName;

        for(const QString& encoderName : codecs.value(codecName))
        {
            FFmpegEncoderData encoderData;
            encoderData.name = encoderName;
            data.ffmpegEnoderList.append( encoderData );
        }

        codecList.append( data );
    }

    for( int i=0; i<codecList.count(); i++ )
    {
        for( int j=0; j<codecList.at(i).ffmpegEnoderList.count(); j++ )
        {
            if( ( !codecList.at(i).ffmpegEnoderList.at(j).experimental || experimentalCodecsEnabled ) && ffmpegCodecList.contains(codecList.at(i).ffmpegEnoderList.at(j).name) )
            {
                codecList[i].currentFFmpegEncoder = codecList.at(i).ffmpegEnoderList.at(j);
                break;
            }
        }
    }
}

soundkonverter_codec_ffmpeg::~soundkonverter_codec_ffmpeg()
{}

QString soundkonverter_codec_ffmpeg::name() const
{
    return global_plugin_name;
}

int soundkonverter_codec_ffmpeg::version()
{
    return global_plugin_version;
}

QList<ConversionPipeTrunk> soundkonverter_codec_ffmpeg::codecTable()
{
    QList<ConversionPipeTrunk> table;
    QStringList fromCodecs;
    QStringList toCodecs;

    /// decode
    fromCodecs += "wav";
    fromCodecs += "ogg vorbis";
    fromCodecs += "opus";
    fromCodecs += "mp3";
    fromCodecs += "flac";
    fromCodecs += "wma";
    fromCodecs += "aac";
    fromCodecs += "ac3";
    fromCodecs += "m4a/alac";
    fromCodecs += "mp2";
//     fromCodecs += "sonic";
//     fromCodecs += "sonic lossless";
    fromCodecs += "als";
    fromCodecs += "amr nb";
    fromCodecs += "amr wb";
    fromCodecs += "ape";
//     fromCodecs += "e-ac3";
    fromCodecs += "speex";
    fromCodecs += "m4a/aac";
    fromCodecs += "mp1";
    fromCodecs += "musepack";
    fromCodecs += "shorten";
//     fromCodecs += "mlp";
//     fromCodecs += "truehd";
//     fromCodecs += "truespeech";
    fromCodecs += "tta";
    fromCodecs += "wavpack";
    fromCodecs += "ra";
    fromCodecs += "sad";
    /// containers
    fromCodecs += "3gp";
    fromCodecs += "rm";
    /// video
    fromCodecs += "avi";
    fromCodecs += "mkv";
    fromCodecs += "webm";
    fromCodecs += "ogv";
    fromCodecs += "mpeg";
    fromCodecs += "mov";
    fromCodecs += "mp4";
    fromCodecs += "flv";
    fromCodecs += "wmv";
    fromCodecs += "rv";

    /// encode
    if( !binaries["ffmpeg"].isEmpty() )
    {
        QFileInfo ffmpegInfo( binaries["ffmpeg"] );
        if( ffmpegInfo.lastModified() > ffmpegLastModified || configVersion < version() )
        {
            infoProcess = new QProcess();
            infoProcess->setProcessChannelMode( QProcess::MergedChannels );
            connect( infoProcess, SIGNAL(readyRead()), this, SLOT(infoProcessOutput()) );
            connect( infoProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(infoProcessExit(int,QProcess::ExitStatus)) );

            QStringList command;
            command += binaries["ffmpeg"];
            command += "-encoders";
            infoProcess->start(command.first(), command.mid(1));

            infoProcess->waitForFinished( 3000 );
        }
    }

    for( int i=0; i<codecList.count(); i++ )
    {
        for( int j=0; j<codecList.at(i).ffmpegEnoderList.count(); j++ )
        {
            if( ( !codecList.at(i).ffmpegEnoderList.at(j).experimental || experimentalCodecsEnabled ) && ffmpegCodecList.contains(codecList.at(i).ffmpegEnoderList.at(j).name) )
            {
                codecList[i].currentFFmpegEncoder = codecList.at(i).ffmpegEnoderList.at(j);
                break;
            }
        }
        toCodecs += codecList.at(i).codecName;
    }

    for( int i=0; i<fromCodecs.count(); i++ )
    {
        for( int j=0; j<toCodecs.count(); j++ )
        {
            if( fromCodecs.at(i) == "wav" && toCodecs.at(j) == "wav" )
                continue;

            bool codecEnabled = ( toCodecs.at(j) == "wav" ); // always enabled if decoding
            QStringList ffmpegProblemInfo;
            if( !codecEnabled )
            {
                bool experimantalInfo = false;
                for( int k=0; k<codecList.count(); k++ )
                {
                    if( codecList.at(k).codecName == toCodecs.at(j) )
                    {
                        if( !codecList.at(k).currentFFmpegEncoder.name.isEmpty() ) // everything should work, lets exit
                        {
                            codecEnabled = true;
                            break;
                        }
                        for( int l=0; l<codecList.at(k).ffmpegEnoderList.count(); l++ )
                        {
                            if( codecList.at(k).ffmpegEnoderList.at(l).experimental && !experimentalCodecsEnabled && !experimantalInfo )
                            {
                                ffmpegProblemInfo.append( i18n("Enable experimental codecs in the ffmpeg configuration dialog.") );
                                experimantalInfo = true;
                            }
                            else
                            {
                                ffmpegProblemInfo.append( i18n("Compile ffmpeg with %1 support.",codecList.at(k).ffmpegEnoderList.at(l).name) );
                            }
                        }
                        break;
                    }
                }
            }
            if( fromCodecs.at(i) == "opus" && ffmpegVersionMajor < 1 )
                codecEnabled = false;

            ConversionPipeTrunk newTrunk;
            newTrunk.codecFrom = fromCodecs.at(i);
            newTrunk.codecTo = toCodecs.at(j);
            newTrunk.rating = 90;
            newTrunk.enabled = ( binaries["ffmpeg"] != "" ) && codecEnabled;
            if( binaries["ffmpeg"] == "" )
            {
                if( toCodecs.at(j) == "wav" )
                {
                    newTrunk.problemInfo = standardMessage( "decode_codec,backend", fromCodecs.at(i), "ffmpeg" ) + "\n" + standardMessage( "install_patented_backend", "ffmpeg" );
                }
                else if( fromCodecs.at(i) == "wav" )
                {
                    newTrunk.problemInfo = standardMessage( "encode_codec,backend", toCodecs.at(j), "ffmpeg" ) + "\n" + standardMessage( "install_patented_backend", "ffmpeg" );
                }
            }
            else
            {
                newTrunk.problemInfo = ffmpegProblemInfo.join("\n"+i18nc("like in either or","or")+"\n");
            }
            newTrunk.data.hasInternalReplayGain = false;
            table.append( newTrunk );
        }
    }

    QSet<QString> codecs;
    codecs += QSet<QString>(fromCodecs.begin(), fromCodecs.end());
    codecs += QSet<QString>(toCodecs.begin(), toCodecs.end());
    allCodecs = codecs.values();

    return table;
}


bool soundkonverter_codec_ffmpeg::isConfigSupported( ActionType action, const QString& codecName )
{
    Q_UNUSED(action)
    Q_UNUSED(codecName)

    return true;
}

void soundkonverter_codec_ffmpeg::showConfigDialog( ActionType action, const QString& codecName, QWidget *parent )
{
    Q_UNUSED(action)
    Q_UNUSED(codecName)

    if( !configDialog )
    {
        configDialog = new QDialog( parent );
        configDialog->setWindowTitle( i18n("Configure %1",*global_plugin_name) );

        QWidget *configDialogWidget = new QWidget( configDialog );
        QVBoxLayout *configDialogLayout = new QVBoxLayout( configDialog );
        QHBoxLayout *configDialogBox = new QHBoxLayout();
        configDialogExperimantalCodecsEnabledCheckBox = new QCheckBox( i18n("Enable experimental codecs"), configDialogWidget );
        configDialogBox->addWidget( configDialogExperimantalCodecsEnabledCheckBox );
        configDialogWidget->setLayout( configDialogBox );
        configDialogLayout->addWidget( configDialogWidget );

        QDialogButtonBox *buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults );
        configDialogLayout->addWidget( buttonBox );

        connect( buttonBox, SIGNAL( accepted() ), this, SLOT( configDialogSave() ) );
        connect( buttonBox, SIGNAL( rejected() ), configDialog, SLOT( reject() ) );
        connect( buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *btn) {
            if( btn->text().contains("Defaults") || btn->text().contains("Default") )
                configDialogDefault();
        } );
    }
    configDialogExperimantalCodecsEnabledCheckBox->setChecked( experimentalCodecsEnabled );
    configDialog->show();
}

void soundkonverter_codec_ffmpeg::configDialogSave()
{
    if( configDialog )
    {
        const bool old_experimentalCodecsEnabled = experimentalCodecsEnabled;
        experimentalCodecsEnabled = configDialogExperimantalCodecsEnabledCheckBox->isChecked();

        QSettings conf(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/soundkonverterrc", QSettings::IniFormat);
        conf.beginGroup("Plugin-" + name());
        conf.setValue( "experimentalCodecsEnabled", experimentalCodecsEnabled );

        if( experimentalCodecsEnabled != old_experimentalCodecsEnabled )
        {
            QMessageBox::information( configDialog, QString(), i18n("Please restart soundKonverter in order to activate the changes.") );
        }
        configDialog->deleteLater();
    }
}

void soundkonverter_codec_ffmpeg::configDialogDefault()
{
    if( configDialog )
    {
        configDialogExperimantalCodecsEnabledCheckBox->setChecked( false );
    }
}

bool soundkonverter_codec_ffmpeg::hasInfo()
{
    return false;
}

void soundkonverter_codec_ffmpeg::showInfo( QWidget *parent )
{
    Q_UNUSED(parent)
}

CodecWidget *soundkonverter_codec_ffmpeg::newCodecWidget()
{
    FFmpegCodecWidget *widget = new FFmpegCodecWidget();
    return qobject_cast<CodecWidget*>(widget);
}

int soundkonverter_codec_ffmpeg::convert( const QUrl& inputFile, const QUrl& outputFile, const QString& inputCodec, const QString& outputCodec, const ConversionOptions *_conversionOptions, TagData *tags, bool replayGain )
{
    Q_UNUSED(inputCodec)
    Q_UNUSED(tags)
    Q_UNUSED(replayGain)

    QStringList command;
    const ConversionOptions *conversionOptions = _conversionOptions;

    if( outputCodec != "wav" )
    {
        command += binaries["ffmpeg"];
        command += "-i";
        command += "\"" + escapeUrl(inputFile) + "\"";
        for( int i=0; i<codecList.count(); i++ )
        {
            if( codecList.at(i).codecName == outputCodec )
            {
                command += "-acodec";
                command += codecList.at(i).currentFFmpegEncoder.name;
                if( codecList.at(i).currentFFmpegEncoder.experimental )
                {
                    command += "-strict";
                    command += "experimental";
                }
                break;
            }
        }
        if( outputCodec != "m4a/alac" && outputCodec != "flac" )
        {
            command += "-ab";
            command += QString::number(conversionOptions->bitrate) + "k";
        }
        if( conversionOptions->pluginName == name() )
        {
            command += conversionOptions->cmdArguments;
        }
        command += "\"" + escapeUrl(outputFile) + "\"";
    }
    else
    {
        command += binaries["ffmpeg"];
        command += "-i";
        command += "\"" + escapeUrl(inputFile) + "\"";
        command += "\"" + escapeUrl(outputFile) + "\"";
    }

    CodecPluginItem *newItem = new CodecPluginItem( this );
    newItem->id = lastId++;
    newItem->process = new QProcess( newItem );
    newItem->process->setProcessChannelMode( QProcess::MergedChannels );
    connect( newItem->process, SIGNAL(readyRead()), this, SLOT(processOutput()) );
    connect( newItem->process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processExit(int,QProcess::ExitStatus)) );

    if( tags )
        newItem->data.length = tags->length;

    newItem->process->startCommand(command.join(" "));

    logCommand( newItem->id, command.join(" ") );

    backendItems.append( newItem );
    return newItem->id;
}

QStringList soundkonverter_codec_ffmpeg::convertCommand( const QUrl& inputFile, const QUrl& outputFile, const QString& inputCodec, const QString& outputCodec, const ConversionOptions *_conversionOptions, TagData *tags, bool replayGain )
{
    Q_UNUSED(inputFile)
    Q_UNUSED(outputFile)
    Q_UNUSED(inputCodec)
    Q_UNUSED(outputCodec)
    Q_UNUSED(_conversionOptions)
    Q_UNUSED(tags)
    Q_UNUSED(replayGain)

    return QStringList();
}

float soundkonverter_codec_ffmpeg::parseOutput( const QString& output, int *length )
{
    // Duration: 00:02:16.50, start: 0.000000, bitrate: 1411 kb/s
    // size=    2445kB time=00:01:58.31 bitrate= 169.3kbits/s

    QRegularExpression regLength("Duration: (\\d{2}):(\\d{2}):(\\d{2})\\.(\\d{2})");
    if( length && regLength.match(infoProcessOutputData).hasMatch() )
    {
        *length = regLength.match(infoProcessOutputData).captured(1).toInt()*3600 + regLength.match(infoProcessOutputData).captured(2).toInt()*60 + regLength.match(infoProcessOutputData).captured(3).toInt();
    }
    QRegularExpression reg1("time=(\\d{2}):(\\d{2}):(\\d{2})\\.(\\d{2})");
    QRegularExpression reg2("time=(\\d+)\\.\\d");
    if( reg1.match(infoProcessOutputData).hasMatch() )
    {
        return reg1.match(infoProcessOutputData).captured(1).toInt()*3600 + reg1.match(infoProcessOutputData).captured(2).toInt()*60 + reg1.match(infoProcessOutputData).captured(3).toInt();
    }
    else if( reg2.match(infoProcessOutputData).hasMatch() )
    {
        return reg2.match(infoProcessOutputData).captured(1).toInt();
    }

    // TODO error handling
    // Error while decoding stream #0.0

    return -1;
}

float soundkonverter_codec_ffmpeg::parseOutput( const QString& output )
{
    return parseOutput( output, 0 );
}

void soundkonverter_codec_ffmpeg::processOutput()
{
    for( int i=0; i<backendItems.size(); i++ )
    {
        if( backendItems.at(i)->process == QObject::sender() )
        {
            const QString output = backendItems.at(i)->process->readAllStandardOutput().data();

            CodecPluginItem *pluginItem = qobject_cast<CodecPluginItem*>(backendItems.at(i));

            float progress = parseOutput( output, &pluginItem->data.length );
            if( progress == -1 && !output.simplified().isEmpty() )
                logOutput( backendItems.at(i)->id, output );

            progress = progress * 100 / pluginItem->data.length;
            if( progress > backendItems.at(i)->progress )
                backendItems.at(i)->progress = progress;

            return;
        }
    }
}

void soundkonverter_codec_ffmpeg::infoProcessOutput()
{
    infoProcessOutputData.append( infoProcess->readAllStandardOutput().data() );
}

void soundkonverter_codec_ffmpeg::infoProcessExit( int exitCode, QProcess::ExitStatus exitStatus )
{
    Q_UNUSED(exitStatus)
    Q_UNUSED(exitCode)

    QRegularExpression regVersion("ffmpeg version (\\d+)\\.(\\d+) ");
    if( infoProcessOutputData.contains( regVersion ) )
    {
        ffmpegVersionMajor = regVersion.match(infoProcessOutputData).captured(1).toInt();
        ffmpegVersionMinor = regVersion.match(infoProcessOutputData).captured(2).toInt();
    }

    ffmpegCodecList.clear();

    for( int i=0; i<codecList.count(); i++ )
    {
        for( int j=0; j<codecList.at(i).ffmpegEnoderList.count(); j++ )
        {
            QRegularExpression regEncoder("[AVS][F\\.][S\\.]([X\\.])[B\\.][D\\.] "+codecList.at(i).ffmpegEnoderList.at(j).name+"\\b");
            if( infoProcessOutputData.contains( regEncoder ))
            {
                const bool experimental = regEncoder.match(infoProcessOutputData).captured(1) == "X";
                if( experimental )
                {
                    codecList[i].ffmpegEnoderList[j].experimental = true;
                }
                ffmpegCodecList += codecList.at(i).ffmpegEnoderList.at(j).name;
            }
        }
    }

    QFileInfo ffmpegInfo( binaries["ffmpeg"] );
    ffmpegLastModified = ffmpegInfo.lastModified();

    QSettings conf(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/soundkonverterrc", QSettings::IniFormat);
    conf.beginGroup("Plugin-" + name());
    conf.setValue( "configVersion", version() );
    conf.setValue( "ffmpegVersionMajor", ffmpegVersionMajor );
    conf.setValue( "ffmpegVersionMinor", ffmpegVersionMinor );
    conf.setValue( "ffmpegLastModified", ffmpegLastModified );
    conf.setValue( "codecList", ffmpegCodecList.values() );

    infoProcessOutputData.clear();
    infoProcess->deleteLater();
}

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(soundkonverter_codec_ffmpeg, "codec_ffmpeg.json")

#include "soundkonverter_codec_ffmpeg.moc"
