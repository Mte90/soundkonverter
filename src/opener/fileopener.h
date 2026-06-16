//
// C++ Interface: opener
//
// Description:
//
//
// Author: Daniel Faust <hessijames@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef FILEOPENER_H
#define FILEOPENER_H

#include <QDialog>

#include <QUrl>

class Config;
class Options;
class QLabel;
class ConversionOptions;
class QDialog;
class QPushButton;
class QFileDialog;

/** @author Daniel Faust <hessijames@gmail.com> */
class FileOpener : public QDialog
{
    Q_OBJECT
public:
    FileOpener( Config *_config, QWidget *parent, Qt::WindowFlags f={} );
    ~FileOpener();

    /** true if the file dialog was aborted (don't execute the dialog) */
    bool dialogAborted;

private:
    Config *config;

    QFileDialog *fileDialog;
    Options *options;
    QList<QUrl> urls;
    QPushButton *pAdd;
    QPushButton *pCancel;
    QLabel *formatHelp;

private slots:
    void fileDialogAccepted();
    void okClickedSlot();
    void showHelp();

signals:
    void openFiles( const QList<QUrl>& files, ConversionOptions *conversionOptions );

};

#endif
