
#include <QStandardPaths>
#include <QSettings>
#include <QRegularExpression>
#include <KLocalizedString>
#include "cdparanoiaripperglobal.h"

#include "soundkonverter_ripper_cdparanoia.h"

#include <QWidget>
#include <QLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLocale>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QAbstractButton>
#include <Qt>


soundkonverter_ripper_cdparanoia::soundkonverter_ripper_cdparanoia( QObject *parent, const QVariantList& args  )
    : RipperPlugin( parent )
{
    Q_UNUSED(args)

    configDialogForceReadSpeedCheckBox = 0;
    configDialogForceReadSpeedSpinBox = 0;
    configDialogForceEndiannessComboBox = 0;
    configDialogMaximumRetriesSpinBox = 0;
    configDialogEnableParanoiaCheckBox = 0;
    configDialogEnableExtraParanoiaCheckBox = 0;

    binaries["cdparanoia"] = "";

    QSettings conf(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/soundkonverterrc", QSettings::IniFormat);
    conf.beginGroup("Plugin-" + name());
    forceReadSpeed = conf.value( "forceReadSpeed", 0 ).toInt();
    forceEndianness = conf.value( "forceEndianness", 0 ).toInt();
    maximumRetries = conf.value( "maximumRetries", 20 ).toInt();
    enableParanoia = conf.value( "enableParanoia", true ).toBool();
    enableExtraParanoia = conf.value( "enableExtraParanoia", true ).toBool();
}

soundkonverter_ripper_cdparanoia::~soundkonverter_ripper_cdparanoia()
{}

QString soundkonverter_ripper_cdparanoia::name() const
{
    return global_plugin_name;
}

QList<ConversionPipeTrunk> soundkonverter_ripper_cdparanoia::codecTable()
{
    QList<ConversionPipeTrunk> table;
    ConversionPipeTrunk newTrunk;

    newTrunk.codecFrom = "audio cd";
    newTrunk.codecTo = "wav";
    newTrunk.rating = 100;
    newTrunk.enabled = ( binaries["cdparanoia"] != "" );
    newTrunk.data.canRipEntireCd = true;
    newTrunk.problemInfo = i18n( "In order to rip audio cds per track or to a single file, you need to install 'cdparanoia'.\n'cdparanoia' is usually shipped with your distribution, the package name can vary." );
    table.append( newTrunk );

    return table;
}

bool soundkonverter_ripper_cdparanoia::isConfigSupported( ActionType action, const QString& codecName )
{
    Q_UNUSED(action)
    Q_UNUSED(codecName)

    return true;
}

void soundkonverter_ripper_cdparanoia::showConfigDialog( ActionType action, const QString& codecName, QWidget *parent )
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

        QHBoxLayout *configDialogBox0 = new QHBoxLayout();
        configDialogForceReadSpeedCheckBox = new QCheckBox( i18n("Force read speed:"), configDialogWidget );
        configDialogBox0->addWidget( configDialogForceReadSpeedCheckBox );
        configDialogForceReadSpeedSpinBox = new QSpinBox( configDialogWidget );
        configDialogForceReadSpeedSpinBox->setRange(1, 64);
        configDialogForceReadSpeedSpinBox->setSuffix(" x");
        configDialogBox0->addWidget( configDialogForceReadSpeedSpinBox );
        configDialogBox->addLayout( configDialogBox0 );
        connect( configDialogForceReadSpeedCheckBox, SIGNAL( stateChanged(int) ), this, SLOT( configDialogForceReadSpeedChanged(int) ) );

        QHBoxLayout *configDialogBox1 = new QHBoxLayout();
        QLabel *configDialogForceEndiannessLabel = new QLabel( i18nc("Byte-Order", "Endianness:"), configDialogWidget );
        configDialogBox1->addWidget( configDialogForceEndiannessLabel );
        configDialogForceEndiannessComboBox = new QComboBox( configDialogWidget );
        configDialogForceEndiannessComboBox->addItem( "Auto" );
        configDialogForceEndiannessComboBox->addItem( "Little endian" );
        configDialogForceEndiannessComboBox->addItem( "Big endian" );
        configDialogBox1->addWidget( configDialogForceEndiannessComboBox );
        configDialogBox->addLayout( configDialogBox1 );

        QHBoxLayout *configDialogBox2 = new QHBoxLayout();
        QLabel *configDialogMaximumRetriesLabel = new QLabel( i18n("Maximum read retries:"), configDialogWidget );
        configDialogBox2->addWidget( configDialogMaximumRetriesLabel );
        configDialogMaximumRetriesSpinBox = new QSpinBox( configDialogWidget );
        configDialogMaximumRetriesSpinBox->setRange(0, 100);
        configDialogBox2->addWidget( configDialogMaximumRetriesSpinBox );
        configDialogBox->addLayout( configDialogBox2 );

        QHBoxLayout *configDialogBox3 = new QHBoxLayout( configDialogWidget );
        configDialogEnableParanoiaCheckBox = new QCheckBox( i18n("Enable paranoia"), configDialogWidget );
        configDialogBox3->addWidget( configDialogEnableParanoiaCheckBox );
        configDialogBox->addLayout( configDialogBox3 );

        QHBoxLayout *configDialogBox4 = new QHBoxLayout( configDialogWidget );
        configDialogEnableExtraParanoiaCheckBox = new QCheckBox( i18n("Enable extra paranoia"), configDialogWidget );
        configDialogBox4->addWidget( configDialogEnableExtraParanoiaCheckBox );
        configDialogBox->addLayout( configDialogBox4 );

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
    configDialogForceReadSpeedCheckBox->setChecked( forceReadSpeed > 0 );
    configDialogForceReadSpeedSpinBox->setValue( forceReadSpeed );
    configDialogForceEndiannessComboBox->setCurrentIndex( forceEndianness );
    configDialogMaximumRetriesSpinBox->setValue( maximumRetries );
    configDialogEnableParanoiaCheckBox->setChecked( enableParanoia );
    configDialogEnableExtraParanoiaCheckBox->setChecked( enableExtraParanoia );

    configDialogForceReadSpeedChanged( configDialogForceReadSpeedCheckBox->checkState() );

    configDialog->show();
}

void soundkonverter_ripper_cdparanoia::configDialogForceReadSpeedChanged( int state )
{
    if( configDialog )
    {
        configDialogForceReadSpeedSpinBox->setEnabled( state == Qt::Checked );
    }
}

void soundkonverter_ripper_cdparanoia::configDialogSave()
{
    if( configDialog )
    {
        forceReadSpeed = configDialogForceReadSpeedCheckBox->isChecked() ? configDialogForceReadSpeedSpinBox->value() : 0;
        forceEndianness = configDialogForceEndiannessComboBox->currentIndex();
        maximumRetries = configDialogMaximumRetriesSpinBox->value();
        enableParanoia = configDialogEnableParanoiaCheckBox->isChecked();
        enableExtraParanoia = configDialogEnableExtraParanoiaCheckBox->isChecked();

        QSettings conf(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/soundkonverterrc", QSettings::IniFormat);
        conf.beginGroup("Plugin-" + name());
        conf.setValue( "forceReadSpeed", forceReadSpeed );
        conf.setValue( "forceEndianness", forceEndianness );
        conf.setValue( "maximumRetries", maximumRetries );
        conf.setValue( "enableParanoia", enableParanoia );
        conf.setValue( "enableExtraParanoia", enableExtraParanoia );

        configDialog->deleteLater();
    }
}

void soundkonverter_ripper_cdparanoia::configDialogDefault()
{
    if( configDialog )
    {
        configDialogForceReadSpeedCheckBox->setChecked( false );
        configDialogForceReadSpeedSpinBox->setValue( 1 );
        configDialogForceEndiannessComboBox->setCurrentIndex( 0 );
        configDialogMaximumRetriesSpinBox->setValue( 20 );
        configDialogEnableParanoiaCheckBox->setChecked( true );
        configDialogEnableExtraParanoiaCheckBox->setChecked( true );
    }
}

bool soundkonverter_ripper_cdparanoia::hasInfo()
{
    return false;
}

void soundkonverter_ripper_cdparanoia::showInfo( QWidget *parent )
{
    Q_UNUSED(parent)
}

int soundkonverter_ripper_cdparanoia::rip( const QString& device, int track, int tracks, const QUrl& outputFile )
{
    QStringList command;

    command += binaries["cdparanoia"];
    command += "--stderr-progress";
    command += "--force-cdrom-device";
    command += device;
    if( forceReadSpeed > 0 )
    {
        command += "--force-read-speed";
        command += QString::number(forceReadSpeed);
    }
    if( forceEndianness == 1 )
    {
        command += "--force-cdrom-little-endian";
    }
    else if( forceEndianness == 2 )
    {
        command += "--force-cdrom-big-endian";
    }
    command += "--never-skip=" + QString::number(maximumRetries);
    if( !enableExtraParanoia )
    {
        if( !enableParanoia )
        {
            command += "--disable-paranoia";
        }
        else
        {
            command += "--disable-extra-paranoia";
        }
    }
    else if( !enableParanoia )
    {
        command += "--disable-paranoia";
    }
    if( track > 0 )
    {
        command += QString::number(track);
    }
    else
    {
        command += "1-" + QString::number(tracks);
    }
    command += "\"" + outputFile.toLocalFile() + "\"";

    RipperPluginItem *newItem = new RipperPluginItem( this );
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

QStringList soundkonverter_ripper_cdparanoia::ripCommand( const QString& device, int track, int tracks, const QUrl& outputFile )
{
    Q_UNUSED(device)
    Q_UNUSED(track)
    Q_UNUSED(tracks)
    Q_UNUSED(outputFile)

    return QStringList();
}

float soundkonverter_ripper_cdparanoia::parseOutput( const QString& output, int *fromSector, int *toSector )
{
    // Ripping from sector       0 (track  1 [0:00.00])
    //           to sector   16361 (track  1 [3:38.11])

    // ##: -2 [wrote] @ 19242887\n

    if( output.contains("sector") )
    {
        if( fromSector && output.contains("from sector") )
        {
            QString data = output;
            data.remove( 0, data.indexOf("from sector") + 11 );
            data = data.left( data.indexOf("(") );
            data = data.simplified();
            *fromSector = data.toInt();
        }
        if( toSector && output.contains("to sector") )
        {
            QString data = output;
            data.remove( 0, data.indexOf("to sector") + 9 );
            data = data.left( data.indexOf("(") );
            data = data.simplified();
            *toSector = data.toInt();
        }
        return -1;
    }

    if( output == "" || !output.contains("@") ) return -1;
    if( !output.contains("[wrote] @") ) return 0;

    QString data = output;
    data.remove( 0, data.indexOf("[wrote] @") + 9 );
    data = data.left( data.indexOf("\n") );
    data = data.simplified();
    return data.toFloat() / 1176;
}

float soundkonverter_ripper_cdparanoia::parseOutput( const QString& output )
{
    return parseOutput( output, 0, 0 );
}

void soundkonverter_ripper_cdparanoia::processOutput()
{
    for( int i=0; i<backendItems.size(); i++ )
    {
        if( backendItems.at(i)->process == QObject::sender() )
        {
            QString output = backendItems.at(i)->process->readAllStandardOutput().data();
            RipperPluginItem *pluginItem = qobject_cast<RipperPluginItem*>(backendItems.at(i));

            float progress = parseOutput( output, &pluginItem->data.fromSector, &pluginItem->data.toSector );

            if( progress == -1 && !output.simplified().isEmpty() )
                logOutput( backendItems.at(i)->id, output );

            progress = (progress - (float)pluginItem->data.fromSector) * 100 / (float)(pluginItem->data.toSector - pluginItem->data.fromSector);

            if( progress > backendItems.at(i)->progress )
                backendItems.at(i)->progress = progress;

            return;
        }
    }
}

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(soundkonverter_ripper_cdparanoia, "ripper_cdparanoia.json")

#include "soundkonverter_ripper_cdparanoia.moc"
