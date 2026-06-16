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
#ifndef PLAYLISTOPENER_H
#define PLAYLISTOPENER_H

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
class PlaylistOpener : public QDialog
{
    Q_OBJECT
public:
    PlaylistOpener( Config *_config, QWidget *parent, Qt::WindowFlags f={} );
    ~PlaylistOpener();

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

signals:
    void openFiles( const QList<QUrl>& files, ConversionOptions *conversionOptions );

};

#endif
