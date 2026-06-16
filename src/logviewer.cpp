
#include "logviewer.h"
#include "logger.h"

#include <QLayout>
#include <QLabel>
#include <QApplication>

#include <QLocale>

#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#include <KLocalizedString>


LogViewer::LogViewer( Logger* _logger, QWidget *parent, Qt::WindowFlags f )
    : QDialog( parent, f ),
    logger( _logger )
{
    const int fontHeight = QFontMetrics(QApplication::font()).boundingRect("M").size().height();

    connect( logger, SIGNAL(removedProcess(int)), this, SLOT(processRemoved(int)) );
    connect( logger, SIGNAL(updateProcess(int)), this, SLOT(updateProcess(int)) );

    setWindowTitle( i18n("Log Viewer") );
    setWindowIcon( QIcon::fromTheme("view-list-text") );
    QVBoxLayout *mainLayout = new QVBoxLayout( this );

    QHBoxLayout *topBox = new QHBoxLayout();
    mainLayout->addLayout( topBox );
    QLabel *lItem = new QLabel( i18n("Log file:") );
    topBox->addWidget( lItem );
    topBox->setStretchFactor( lItem, 0 );
    cItem = new QComboBox( this );
    topBox->addWidget( cItem );
    topBox->setStretchFactor( cItem, 1 );
    connect( cItem, SIGNAL(activated(int)), this, SLOT(itemChanged()) );

    kLog = new QTextEdit( this );
    kLog->setTabStopDistance( kLog->tabStopDistance()/2 );
    mainLayout->addWidget( kLog );
    kLog->setTextInteractionFlags( Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard );

    QDialogButtonBox *buttonBox = new QDialogButtonBox( this );
    buttonBox->addButton( QDialogButtonBox::Close );
    QPushButton *saveButton = buttonBox->addButton( i18n("Save"), QDialogButtonBox::ActionRole );
    connect( saveButton, SIGNAL(clicked()), this, SLOT(save()) );
    connect( buttonBox, SIGNAL(rejected()), this, SLOT(reject()) );
    mainLayout->addWidget( buttonBox );

    refillLogs();

    resize( QSize(60*fontHeight,40*fontHeight) );
    QSettings settings("soundkonverterrc", QSettings::IniFormat);
    settings.beginGroup( "LogViewer" );
    restoreGeometry( settings.value("geometry").toByteArray() );
    settings.endGroup();}

LogViewer::~LogViewer()
{
    QSettings settings("soundkonverterrc", QSettings::IniFormat);
    settings.beginGroup( "LogViewer" );
    settings.setValue( "geometry", saveGeometry() );
    settings.endGroup();
}

void LogViewer::refillLogs()
{
    const int currentProcess = cItem->itemData(cItem->currentIndex()).toInt();

    cItem->clear();

    for(const auto& log : logger->getLogs())
    {
        const int id = log.first;
        QString name = log.second;
        // TODO make the string width dependend on the window width
        if( name.length() > 73 )
            name = name.left(35) + "..." + name.right(35);

        if( id == 1000 )
            cItem->addItem( i18n("soundKonverter application log"), QVariant(id) );
        else
            cItem->addItem( name, QVariant(id) );
    }

    if( cItem->findData(currentProcess) != -1 )
        cItem->setCurrentIndex( cItem->findData(currentProcess) );
    else
        cItem->setCurrentIndex( 0 );

    itemChanged();
}

void LogViewer::itemChanged()
{
    // HACK avoid Qt bug? changing the color of 'uncolored' text when switching the log file
    QTextCursor cursor = kLog->textCursor();
    cursor.setPosition( 0 );
    kLog->setTextCursor( cursor );

    kLog->clear();
    const LoggerItem* const item = logger->getLog( cItem->itemData(cItem->currentIndex()).toInt() );

    if( !item )
        return;

    for(const QString& line : item->data)
        kLog->append( line );

    QPalette currentPalette = kLog->palette();
    if( item->completed )
    {
        currentPalette.setColor( QPalette::Base, QApplication::palette().base().color() );
    }
    else
    {
        currentPalette.setColor( QPalette::Base, QColor(255,234,234) );
    }
    kLog->setPalette( currentPalette );
}

void LogViewer::save()
{
    const QString fileName = QFileDialog::getSaveFileName( this, i18n("Save log file"), QString(), "*.txt\n*.log" );
    if( fileName.isEmpty() )
        return;

    QFile file( fileName );
    if( file.exists() )
    {
        if( QMessageBox::question(this, i18n("File exists"), i18n("File already exists. Do you really want to overwrite it?")) == QMessageBox::No )
            return;
    }
    if( !file.open(QIODevice::WriteOnly) )
    {
        QMessageBox::critical( this, i18n("Error"), i18n("Writing to file failed.\nMaybe you haven't got write permission.") );
        return;
    }
    QTextStream textStream;
    textStream.setDevice( &file );
    textStream << kLog->toPlainText();
    file.close();
}

void LogViewer::processRemoved( int id )
{
    Q_UNUSED(id)

    refillLogs();
}

void LogViewer::updateProcess( int id )
{
    Q_UNUSED(id)

    refillLogs();
}

void LogViewer::showLog( int id )
{
    if( cItem->findData(QVariant(id)) != -1 )
        cItem->setCurrentIndex( cItem->findData(QVariant(id)) );
    else
        cItem->setCurrentIndex( 0 );

    itemChanged();
}

