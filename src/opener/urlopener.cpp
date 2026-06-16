//
// C++ Implementation: opener
//
// Description:
//
//
// Author: Daniel Faust <hessijames@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "urlopener.h"
#include "../options.h"
#include "../config.h"

#include <QApplication>
#include <QLocale>
#include <QPushButton>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QSettings>

#include <QLineEdit>
#include <QDir>
#include <QIcon>
#include <klocalizedstring.h>


// TODO enable proceed button only if at least one file got selected // copy'n'paste error ???

// TODO message box if url can't be added -> maybe in file list

UrlOpener::UrlOpener( Config *_config, QWidget *parent, Qt::WindowFlags f )
    : QDialog( parent, f ),
    config( _config )
{
    setWindowTitle( i18n("Add url") );
    setWindowIcon( QIcon::fromTheme("network-workgroup") );
    //

    page = FileOpenPage;

    const int fontHeight = QFontMetrics(QApplication::font()).boundingRect("M").size().height();

    QWidget *widget = new QWidget();
    setLayout( new QVBoxLayout( this ) ); // widget );

    QGridLayout *mainGrid = new QGridLayout( widget );
    QGridLayout *topGrid = new QGridLayout( widget );
    mainGrid->addLayout( topGrid, 0, 0 );

    lSelector = new QLabel( i18n("1. Enter url"), widget );
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

    QVBoxLayout *urlBox = new QVBoxLayout();
    mainGrid->addLayout( urlBox, 2, 0 );
    urlBox->addSpacing( 6*fontHeight );
    urlRequester = new QLineEdit( widget );
    // QLineEdit does not have setMode
    urlBox->addWidget( urlRequester );
    urlBox->addStretch();

    options = new Options( config, i18n("Select your desired output options and click on \"Ok\"."), widget );
    mainGrid->addWidget( options, 2, 0 );
    adjustSize();
    options->hide();


    // add a horizontal box layout for the control elements
    QHBoxLayout *controlBox = new QHBoxLayout();
    mainGrid->addLayout( controlBox, 5, 0 );
    controlBox->addStretch();

    pProceed = new QPushButton( QIcon::fromTheme("go-next"), i18n("Proceed"), widget );
    controlBox->addWidget( pProceed );
    connect( pProceed, SIGNAL(clicked()), this, SLOT(proceedClickedSlot()) );
    pAdd = new QPushButton( QIcon::fromTheme("dialog-ok"), i18n("Ok"), widget );
    controlBox->addWidget( pAdd );
    pAdd->hide();
    connect( pAdd, SIGNAL(clicked()), this, SLOT(okClickedSlot()) );
    pCancel = new QPushButton( QIcon::fromTheme("dialog-cancel"), i18n("Cancel"), widget );
    controlBox->addWidget( pCancel );
    connect( pCancel, SIGNAL(clicked()), this, SLOT(reject()) );


        // Prevent the dialog from beeing too wide because of the directory history
    if( parent && width() > parent->width() )
        resize( QSize(parent->width()-fontHeight,sizeHint().height()) );
QSettings settings("soundkonverterrc", QSettings::IniFormat);
// Group: UrlOpener
settings.beginGroup("UrlOpener"); resize(settings.value("size", QSize(600, 400)).toSize()); settings.endGroup();}

UrlOpener::~UrlOpener()
{
    QSettings settings("soundkonverterrc", QSettings::IniFormat);
    // Group: UrlOpener
    settings.beginGroup("UrlOpener"); settings.setValue("size", size()); settings.endGroup();
}

void UrlOpener::proceedClickedSlot()
{
    if( page == FileOpenPage )
    {
        if( urlRequester->text().isEmpty() )
        {
            QMessageBox::information( this, tr("Information"), i18n("The Url you entered is invalid. Please try again.") );
            return;
        }

        urls += QUrl::fromUserInput( urlRequester->text() );

        urlRequester->hide();
        options->show();
        page = ConversionOptionsPage;
        QFont font;
        font.setBold( false );
        lSelector->setFont( font );
        font.setBold( true );
        lOptions->setFont( font );
        pProceed->hide();
        pAdd->show();
    }
}

void UrlOpener::okClickedSlot()
{
    if( page == ConversionOptionsPage )
    {
        ConversionOptions *conversionOptions = options->currentConversionOptions();
        if( conversionOptions )
        {
            options->accepted();
            emit openFiles( urls, conversionOptions );
            accept();
        }
        else
        {
            QMessageBox::critical( this, tr("Error"), i18n("No conversion options selected.") );
        }
    }
}
