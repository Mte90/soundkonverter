
#include <QStandardPaths>
#include <QSettings>
#include <QRegularExpression>
#include <KLocalizedString>
#include "mp3replaygainglobal.h"

#include "soundkonverter_replaygain_mp3gain.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QWidget>
#include <QAbstractButton>
#include <Qt>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>


Mp3GainPluginItem::Mp3GainPluginItem( QObject *parent )
    : ReplayGainPluginItem( parent )
{}

Mp3GainPluginItem::~Mp3GainPluginItem()
{}


soundkonverter_replaygain_mp3gain::soundkonverter_replaygain_mp3gain( QObject *parent, const QVariantList& args  )
    : ReplayGainPlugin( parent )
{
    Q_UNUSED(args)

    configDialogTagModeComboBox = 0;
    configDialogModifyAudioStreamCheckBox = 0;
    configDialogGainAdjustmentSpinBox = 0;

    binaries["mp3gain"] = "";

    allCodecs += "mp3";

    QSettings conf(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/soundkonverterrc", QSettings::IniFormat);
    conf.beginGroup("Plugin-" + name());
    tagMode = conf.value( "tagMode", 0 ).toInt();
    modifyAudioStream = conf.value( "modifyAudioStream", false ).toBool();
    gainAdjustment = conf.value( "gainAdjustment", 0.0 ).toDouble();
}

soundkonverter_replaygain_mp3gain::~soundkonverter_replaygain_mp3gain()
{}

QString soundkonverter_replaygain_mp3gain::name() const
{
    return global_plugin_name;
}

QList<ReplayGainPipe> soundkonverter_replaygain_mp3gain::codecTable()
{
    QList<ReplayGainPipe> table;
    ReplayGainPipe newPipe;

    newPipe.codecName = "mp3";
    newPipe.rating = 100;
    newPipe.enabled = ( binaries["mp3gain"] != "" );
    newPipe.problemInfo = standardMessage( "replygain_codec,backend", "mp3", "mp3gain" ) + "\n" + standardMessage( "install_patented_backend", "mp3gain" );
    table.append( newPipe );

    return table;
}

bool soundkonverter_replaygain_mp3gain::isConfigSupported( ActionType action, const QString& codecName )
{
    Q_UNUSED(action)
    Q_UNUSED(codecName)

    return true;
}

void soundkonverter_replaygain_mp3gain::showConfigDialog( ActionType action, const QString& codecName, QWidget *parent )
{
    Q_UNUSED(action)
    Q_UNUSED(codecName)

    if( !configDialog )
    {
        configDialog = new QDialog( parent );
        configDialog->setWindowTitle( i18n("Configure %1",*global_plugin_name) );

        QVBoxLayout *configDialogLayout = new QVBoxLayout( configDialog.data() );

        QWidget *configDialogWidget = new QWidget( configDialog.data() );
        QVBoxLayout *configDialogBox = new QVBoxLayout( configDialogWidget );

        QHBoxLayout *configDialogBox1 = new QHBoxLayout();
        QLabel *configDialogTagModeLabel = new QLabel( i18n("Use tag format:"), configDialogWidget );
        configDialogBox1->addWidget( configDialogTagModeLabel );
        configDialogTagModeComboBox = new QComboBox( configDialogWidget );
        configDialogTagModeComboBox->addItem( "APE" );
        configDialogTagModeComboBox->addItem( "ID3v2" );
        configDialogBox1->addWidget( configDialogTagModeComboBox );
        configDialogBox->addLayout( configDialogBox1 );

        QHBoxLayout *configDialogBox3 = new QHBoxLayout();
        QLabel *configDialogGainAdjustmentLabel = new QLabel( i18n("Adjust gain:"), configDialogWidget );
        configDialogBox3->addWidget( configDialogGainAdjustmentLabel );
        configDialogGainAdjustmentSpinBox = new QDoubleSpinBox( configDialogWidget );
        configDialogGainAdjustmentSpinBox->setRange( -99, 99 );
        configDialogGainAdjustmentSpinBox->setSuffix( " " + i18nc("decibel","dB") );
        configDialogGainAdjustmentSpinBox->setToolTip( i18n("Lower or raise the suggested gain") );
        configDialogBox3->addWidget( configDialogGainAdjustmentSpinBox );
        configDialogBox->addLayout( configDialogBox3 );

        QHBoxLayout *configDialogBox2 = new QHBoxLayout();
        configDialogModifyAudioStreamCheckBox = new QCheckBox( i18n("Modify audio stream"), configDialogWidget );
        configDialogModifyAudioStreamCheckBox->setToolTip( i18n("Write gain adjustments directly into the encoded data. That way the adjustment works with all mp3 players.\nUndoing the changes is still possible since correction data will be written as well.") );
        configDialogBox2->addWidget( configDialogModifyAudioStreamCheckBox );
        configDialogBox->addLayout( configDialogBox2 );

        configDialogWidget->setLayout( configDialogBox );
        configDialogLayout->addWidget( configDialogWidget );

        QDialogButtonBox *buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults );
        configDialogLayout->addWidget( buttonBox );

        connect( buttonBox, SIGNAL( accepted() ), this, SLOT( configDialogSave() ) );
        connect( buttonBox, SIGNAL( rejected() ), configDialog.data(), SLOT( reject() ) );
        connect( buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *btn) {
            if( btn->text().contains("Defaults") )
                configDialogDefault();
        } );
    }
    configDialogTagModeComboBox->setCurrentIndex( tagMode );
    configDialogModifyAudioStreamCheckBox->setChecked( modifyAudioStream );
    configDialogGainAdjustmentSpinBox->setValue( gainAdjustment );
    configDialog->show();
}

void soundkonverter_replaygain_mp3gain::configDialogSave()
{
    if( configDialog )
    {
        tagMode = configDialogTagModeComboBox->currentIndex();
        modifyAudioStream = configDialogModifyAudioStreamCheckBox->isChecked();
        gainAdjustment = configDialogGainAdjustmentSpinBox->value();

        QSettings conf(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/soundkonverterrc", QSettings::IniFormat);
        conf.beginGroup("Plugin-" + name());
        conf.setValue( "tagMode", tagMode );
        conf.setValue( "modifyAudioStream", modifyAudioStream );
        conf.setValue( "gainAdjustment", gainAdjustment );

        configDialog->deleteLater();
    }
}

void soundkonverter_replaygain_mp3gain::configDialogDefault()
{
    if( configDialog )
    {
        configDialogTagModeComboBox->setCurrentIndex( 0 );
        configDialogModifyAudioStreamCheckBox->setChecked( false );
        configDialogGainAdjustmentSpinBox->setValue( 0.0 );
    }
}

bool soundkonverter_replaygain_mp3gain::hasInfo()
{
    return false;
}

void soundkonverter_replaygain_mp3gain::showInfo( QWidget *parent )
{
    Q_UNUSED(parent)
}

int soundkonverter_replaygain_mp3gain::apply( const QList<QUrl>& fileList, ReplayGainPlugin::ApplyMode mode )
{
    if( fileList.count() <= 0 )
        return BackendPlugin::UnknownError;

    Mp3GainPluginItem *newItem = new Mp3GainPluginItem( this );
    newItem->id = lastId++;
    newItem->process = new QProcess( newItem );
    newItem->process->setProcessChannelMode( QProcess::MergedChannels );
    connect( newItem->process, SIGNAL(readyRead()), this, SLOT(processOutput()) );

    QStringList command;
    command += binaries["mp3gain"];
    if( mode == ReplayGainPlugin::Add )
    {
        command += "-k";
        if( modifyAudioStream )
        {
            command += "-a";
        }
        connect( newItem->process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processExit(int,QProcess::ExitStatus)) );
    }
    else if( mode == ReplayGainPlugin::Force )
    {
        command += "-k";
        if( modifyAudioStream )
        {
            command += "-a";
        }
        command += "-s";
        command += "r";
        connect( newItem->process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processExit(int,QProcess::ExitStatus)) );
    }
    else
    {
        command += "-u";
        connect( newItem->process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(undoProcessExit(int,QProcess::ExitStatus)) );
        newItem->undoFileList = fileList;
    }
    if( gainAdjustment != 0 )
    {
        command += "-d";
        command += QString::number(gainAdjustment);
    }
    if( mode == ReplayGainPlugin::Add || mode == ReplayGainPlugin::Force )
    {
        if( tagMode == 0 )
        {
            // APE tags
            command += "-s";
            command += "a";
        }
        else
        {
            // ID3v2 tags
            command += "-s";
            command += "i";
        }
    }
    for(const QUrl& file : fileList)
    {
        command += "\"" + escapeUrl(file) + "\"";
    }

    newItem->process->startCommand(command.join(" "));

    logCommand( newItem->id, command.join(" ") );

    backendItems.append( newItem );
    return newItem->id;
}

void soundkonverter_replaygain_mp3gain::undoProcessExit( int exitCode, QProcess::ExitStatus exitStatus )
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    Mp3GainPluginItem *item = 0;

    for( int i=0; i<backendItems.size(); i++ )
    {
        if( backendItems.at(i)->process == QObject::sender() )
        {
            item = qobject_cast<Mp3GainPluginItem*>(backendItems.at(i));
            break;
        }
    }

    if( !item )
        return;

    if( item->undoFileList.count() <= 0 )
        return;

    if( item->process )
        item->process->deleteLater();

    item->process = new QProcess( item );
    item->process->setProcessChannelMode( QProcess::MergedChannels );
    connect( item->process, SIGNAL(readyRead()), this, SLOT(processOutput()) );
    connect( item->process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processExit(int,QProcess::ExitStatus)) );

    QStringList command;
    command += binaries["mp3gain"];
    // APE tags
    command += "-s";
    command += "a";
    // ID3v2 tags
    command += "-s";
    command += "i";
    // delete tags
    command += "-s";
    command += "d";
    for(const QUrl& file : item->undoFileList)
    {
        command += "\"" + escapeUrl(file) + "\"";
    }

    item->process->startCommand(command.join(" "));

    logCommand( item->id, command.join(" ") );
}

float soundkonverter_replaygain_mp3gain::parseOutput( const QString& output )
{
    //  9% of 45218064 bytes analyzed
    // [1/10] 32% of 13066690 bytes analyzed

    float progress = -1.0f;

    QRegularExpression reg1("\\[(\\d+)/(\\d+)\\] (\\d+)%");
    QRegularExpression reg2("(\\d+)%");
    if( reg1.match(output).hasMatch() )
    {
        float fraction = 1.0f/reg1.match(output).captured(2).toInt();
        progress = 100*(reg1.match(output).captured(1).toInt()-1)*fraction + reg1.match(output).captured(3).toInt()*fraction;
    }
    else if( reg2.match(output).hasMatch() )
    {
        progress = reg2.match(output).captured(1).toInt();
    }

    // Applying mp3 gain change of -6 to /home/user/file.mp3...
    // Undoing mp3gain changes (6,6) to /home/user/file.mp3...
    // Deleting tag info of /home/user/file.mp3...
    QRegularExpression reg3("[Applying mp3 gain change|Undoing mp3gain changes|Deleting tag info]");
    if( progress == -1 && reg3.match(output).hasMatch() )
    {
        progress = 0.0f;
    }

    return progress;
}

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(soundkonverter_replaygain_mp3gain, "replaygain_mp3gain.json")

#include "soundkonverter_replaygain_mp3gain.moc"
