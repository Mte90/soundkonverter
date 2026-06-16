
#include "diropener.h"
#include "../config.h"
#include "../options.h"
#include "../codecproblems.h"

#include <QApplication>
#include <QLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDir>
#include <QCheckBox>
#include <QPushButton>
#include <QListWidget>
#include <QLineEdit>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QFileDialog>
#include <klocalizedstring.h>


DirOpener::DirOpener( Config *_config, Mode _mode, QWidget *parent, Qt::WindowFlags f )
    : QDialog(parent, f ),
    dialogAborted( false ),
    config( _config ),
    mode( _mode )
{
    setWindowTitle( i18n("Add folder") );
    setWindowIcon( QIcon::fromTheme("folder") );

    QDialogButtonBox *buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
    if( mode == Convert )
    {
        buttonBox->setStandardButtons( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
        buttonBox->button( QDialogButtonBox::Ok )->setText( i18n("Proceed") );
        buttonBox->button( QDialogButtonBox::Ok )->setIcon( QIcon("go-next") );
    }
    connect( buttonBox, SIGNAL(accepted()), this, SLOT(addClicked()) );
    connect( buttonBox, SIGNAL(rejected()), this, SLOT(cancelClicked()) );

    const int fontHeight = QFontMetrics(QApplication::font()).boundingRect("M").size().height();

    connect( this, SIGNAL(user1Clicked()), this, SLOT(proceedClicked()) );
    connect( this, SIGNAL(okClicked()), this, SLOT(addClicked()) );

    page = DirOpenPage;

    QWidget *widget = new QWidget();
    QGridLayout *mainGrid = new QGridLayout( widget );
    QGridLayout *topGrid = new QGridLayout();
    mainGrid->addLayout( topGrid, 0, 0 );
    setLayout( new QVBoxLayout( this ) );

    lSelector = new QLabel( i18n("1. Select directory"), widget );
    QFont font;
    font.setBold( true );
    lSelector->setFont( font );
    topGrid->addWidget( lSelector, 0, 0 );
    lOptions = new QLabel( i18n("2. Set conversion options"), widget );
    topGrid->addWidget( lOptions, 0, 1 );

    // draw a horizontal line
    QFrame *lineFrame = new QFrame( widget );
    lineFrame->setFrameShape( QFrame::HLine );
    lineFrame->setFrameShadow( QFrame::Sunken );
    mainGrid->addWidget( lineFrame, 1, 0 );

    if( mode == ReplayGain )
    {
        lSelector->hide();
        lOptions->hide();
        lineFrame->hide();
    }

    // Dir Opener Widget

    dirOpenerWidget = new QWidget( widget );
    mainGrid->addWidget( dirOpenerWidget, 2, 0 );

    QVBoxLayout *box = new QVBoxLayout( dirOpenerWidget );

    QHBoxLayout *directoryBox = new QHBoxLayout();
    box->addLayout( directoryBox );

    QLabel *labelFilter = new QLabel( i18n("Directory:"), dirOpenerWidget );
    directoryBox->addWidget( labelFilter );

    uDirectory = new QLineEdit( dirOpenerWidget );
    QPushButton *browseBtn = new QPushButton( tr("Browse"), dirOpenerWidget );
    connect( browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory( this, tr("Select Directory"), uDirectory->text() );
        if ( !dir.isEmpty() ) {
            uDirectory->setText( dir );
        }
    });
    directoryBox->addWidget( uDirectory );

    QLabel *labelDirectory = new QLabel( i18n("Only add selected file formats:"), dirOpenerWidget );
    box->addWidget( labelDirectory );

    QHBoxLayout *fileTypesBox = new QHBoxLayout();
    box->addLayout( fileTypesBox );

    QStringList codecList;
    fileTypes = new QListWidget( dirOpenerWidget );
    if( mode == Convert )
    {
        codecList = config->pluginLoader()->formatList( PluginLoader::Decode, PluginLoader::CompressionType(PluginLoader::InferiorQuality|PluginLoader::Lossy|PluginLoader::Lossless|PluginLoader::Hybrid) );
    }
    else if( mode == ReplayGain )
    {
        codecList = config->pluginLoader()->formatList( PluginLoader::ReplayGain, PluginLoader::CompressionType(PluginLoader::InferiorQuality|PluginLoader::Lossy|PluginLoader::Lossless|PluginLoader::Hybrid) );
    }
    for( int i = 0; i < codecList.size(); i++ )
    {
        if( codecList.at(i) == "audio cd" ) continue;
        QListWidgetItem *newItem = new QListWidgetItem( codecList.at(i), fileTypes );
        newItem->setFlags( Qt::ItemIsEnabled | Qt::ItemIsUserCheckable );
        newItem->setCheckState( Qt::Checked );
    }

    QVBoxLayout *fileTypesFormatsBox = new QVBoxLayout();
    fileTypesBox->addLayout( fileTypesFormatsBox );

    fileTypesFormatsBox->addWidget( fileTypes );
    QLabel *formatHelp = new QLabel( "<a href=\"format-help\">" + i18n("Are you missing some file formats?") + "</a>", this );
    connect( formatHelp, SIGNAL(linkActivated(const QString&)), this, SLOT(showHelp()) );
    fileTypesFormatsBox->addWidget( formatHelp );

    QVBoxLayout *fileTypesButtonsBox = new QVBoxLayout();
    fileTypesBox->addLayout( fileTypesButtonsBox );
    fileTypesButtonsBox->addStretch();

    pSelectAll = new QPushButton( QIcon("edit-select-all"), i18n("Select all"), dirOpenerWidget );
    fileTypesButtonsBox->addWidget( pSelectAll );
    connect( pSelectAll, SIGNAL(clicked()), this, SLOT(selectAllClicked()) );

    pSelectNone = new QPushButton( QIcon("application-x-zerosize"), i18n("Select none"), dirOpenerWidget );
    fileTypesButtonsBox->addWidget( pSelectNone );
    connect( pSelectNone, SIGNAL(clicked()), this, SLOT(selectNoneClicked()) );

    cRecursive = new QCheckBox( i18n("Recursive"), dirOpenerWidget );
    cRecursive->setChecked( true );
    cRecursive->setToolTip( i18n("If checked, files from subdirectories will be added, too.") );
    fileTypesButtonsBox->addWidget( cRecursive );

    fileTypesButtonsBox->addStretch();


    // Conversion Options Widget

    options = new Options( config, i18n("Select your desired output options and click on \"Ok\"."), widget );
    mainGrid->addWidget( options, 2, 0 );
    adjustSize();
    options->hide();


    QString dir = QFileDialog::getExistingDirectory( this, tr("Select Directory"), uDirectory->text() );
    if( !dir.isEmpty() )
        uDirectory->setText( dir );
    else
        dialogAborted = true;

        // Prevent the dialog from beeing too wide because of the directory history
    if( parent && width() > parent->width() )
        resize( parent->width()-fontHeight, sizeHint().height() );
    QSettings settings( "soundkonverterrc", QSettings::IniFormat );
    restoreGeometry( settings.value( "DirOpener/geometry" ).toByteArray() );
}

DirOpener::~DirOpener()
{
    QSettings settings( "soundkonverterrc", QSettings::IniFormat );
    settings.setValue( "DirOpener/geometry", saveGeometry() );
}

void DirOpener::proceedClicked()
{
    if( page == DirOpenPage )
    {
        dirOpenerWidget->hide();
        options->show();
        page = ConversionOptionsPage;
        QFont font;
        font.setBold( false );
        lSelector->setFont( font );
        font.setBold( true );
        lOptions->setFont( font );
    }
}

void DirOpener::addClicked()
{
    QStringList selectedCodecs;
    for( int i = 0; i < fileTypes->count(); i++ )
    {
        if( fileTypes->item(i)->checkState() == Qt::Checked )
            selectedCodecs += fileTypes->item(i)->text();
    }

    if( mode == Convert )
    {
        ConversionOptions *conversionOptions = options->currentConversionOptions();
        if( conversionOptions )
        {
            hide();

            emit openFiles( QUrl::fromLocalFile( uDirectory->text() ), cRecursive->checkState() == Qt::Checked, selectedCodecs, conversionOptions );
            accept();
        }
        else
        {
            QMessageBox::critical( this, tr("Error"), i18n("No conversion options selected.") );
        }
    }
    else if( mode == ReplayGain )
    {
        hide();
        emit openFiles( QUrl::fromLocalFile( uDirectory->text() ), cRecursive->checkState() == Qt::Checked, selectedCodecs );
        accept();
    }
}

void DirOpener::selectAllClicked()
{
    for( int i = 0; i < fileTypes->count(); i++ )
    {
        fileTypes->item(i)->setCheckState( Qt::Checked );
    }
}

void DirOpener::selectNoneClicked()
{
    for( int i = 0; i < fileTypes->count(); i++ )
    {
        fileTypes->item(i)->setCheckState( Qt::Unchecked );
    }
}

void DirOpener::showHelp()
{
    QList<CodecProblems::Problem> problemList;

    QMap<QString,QStringList> problems = ( mode == Convert ) ? config->pluginLoader()->decodeProblems() : config->pluginLoader()->replaygainProblems();
    for( int i=0; i<problems.count(); i++ )
    {
        CodecProblems::Problem problem;
        problem.codecName = problems.keys().at(i);
        if( problem.codecName != "wav" )
        {
            problem.solutions = problems.value(problem.codecName);
            problemList += problem;
        }
    }
    CodecProblems *problemsDialog = new CodecProblems( CodecProblems::Debug, problemList, this );
    problemsDialog->exec();
}


