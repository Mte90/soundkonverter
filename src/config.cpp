
#include "config.h"
#include "logger.h"
#include "global.h"

#include <QRegularExpression>
#include <QElapsedTimer>
#include <QSettings>
#include <KLocalizedString>
#include <QDir>
#include <QFileInfo>
#include <QDomElement>
#include <solid/device.h>
#include <QStandardPaths>


Config::Config( Logger *_logger, QObject *parent )
    : QObject( parent ),
    logger( _logger )
{
    connect( this, SIGNAL(updateWriteLogFilesSetting(bool)), logger, SLOT(updateWriteSetting(bool)) );

    pPluginLoader = new PluginLoader( logger, this );
    pTagEngine = new TagEngine( this );
    pConversionOptionsManager = new ConversionOptionsManager( pPluginLoader, this );
}

Config::~Config()
{
    save();
    qDeleteAll(data.profiles);
}

void Config::load()
{
    QElapsedTimer time;
    time.start();

    QStringList formats;

    QSettings settings("soundkonverterrc", QSettings::IniFormat);

    settings.beginGroup("General");
    data.app.configVersion = settings.value( "configVersion", 0 ).toInt();
    data.general.startTab = settings.value( "startTab", 0 ).toInt();
    data.general.lastTab = settings.value( "lastTab", 0 ).toInt();
    data.general.defaultProfile = settings.value( "defaultProfile", i18n("Last used") ).toString();
    data.general.lastProfile = settings.value( "lastProfile", i18n("High") ).toString();
    data.general.defaultFormat = settings.value( "defaultFormat", i18n("Last used") ).toString();
    data.general.lastFormat = settings.value( "lastFormat", "ogg vorbis" ).toString();
    data.general.lastOutputDirectoryMode = settings.value( "lastOutputDirectoryMode", 0 ).toInt();
    data.general.specifyOutputDirectory = settings.value( "specifyOutputDirectory", QDir::homePath() + "/soundKonverter" ).toString();
    data.general.metaDataOutputDirectory = settings.value( "metaDataOutputDirectory", QDir::homePath() + "/soundKonverter/%b/%d - %n - %a - %t" ).toString();
    data.general.copyStructureOutputDirectory = settings.value( "copyStructureOutputDirectory", QDir::homePath() + "/soundKonverter" ).toString();
    data.general.lastMetaDataOutputDirectoryPaths = settings.value( "lastMetaDataOutputDirectoryPaths", QStringList() ).toStringList();
    data.general.lastNormalOutputDirectoryPaths = settings.value( "lastNormalOutputDirectoryPaths", QStringList() ).toStringList();
    data.general.waitForAlbumGain = settings.value( "waitForAlbumGain", true ).toBool();
    data.general.useVFATNames = settings.value( "useVFATNames", false ).toBool();
    data.general.copyIfSameCodec = settings.value( "copyIfSameCodec", false ).toBool();
    data.general.writeLogFiles = settings.value( "writeLogFiles", false ).toBool();
    data.general.conflictHandling = static_cast<Config::Data::General::ConflictHandling>(settings.value( "conflictHandling", 0 ).toInt());
//     data.general.priority = settings.value( "priority", 10 );
    data.general.numFiles = settings.value( "numFiles", 0 ).toInt();
    data.general.numReplayGainFiles = settings.value( "numReplayGainFiles", 0 ).toInt();
    if( data.general.numFiles == 0 || data.general.numReplayGainFiles == 0 )
    {
        QList<Solid::Device> processors = Solid::Device::listFromType(Solid::DeviceInterface::Processor, QString());
        const int num = processors.count() > 0 ? processors.count() : 1;
        if( data.general.numFiles == 0 )
            data.general.numFiles = num;
        if( data.general.numReplayGainFiles == 0 )
            data.general.numReplayGainFiles = num;
    }
//     data.general.executeUserScript = settings.value( "executeUserScript", false );
//     data.general.showToolBar = settings.value( "showToolBar", false );
//     data.general.outputFilePermissions = settings.value( "outputFilePermissions", 644 );
    data.general.actionMenuConvertMimeTypes = settings.value( "actionMenuConvertMimeTypes", QStringList() ).toStringList();
    data.general.actionMenuReplayGainMimeTypes = settings.value( "actionMenuReplayGainMimeTypes", QStringList() ).toStringList();
    data.general.replayGainGrouping = static_cast<Config::Data::General::ReplayGainGrouping>(settings.value( "replayGainGrouping", 0 ).toInt());
    data.general.preferredOggVorbisExtension = settings.value( "preferredOggVorbisExtension", "ogg" ).toString();
    data.general.preferredVorbisCommentCommentTag = settings.value( "preferredVorbisCommentCommentTag", "DESCRIPTION" ).toString();
    data.general.preferredVorbisCommentTrackTotalTag = settings.value( "preferredVorbisCommentTrackTotalTag", "TRACKTOTAL" ).toString();
    data.general.preferredVorbisCommentDiscTotalTag = settings.value( "preferredVorbisCommentDiscTotalTag", "DISCTOTAL" ).toString();

    // due to a bug lastNormalOutputDirectoryPaths could have more than 5 items
    while( data.general.lastNormalOutputDirectoryPaths.count() > 5 )
        data.general.lastNormalOutputDirectoryPaths.takeLast();

    settings.beginGroup("Advanced");
    data.advanced.useSharedMemoryForTempFiles = settings.value( "useSharedMemoryForTempFiles", false ).toBool();
    data.advanced.sharedMemorySize = 0;
    if( QFile::exists("/dev/shm") )
    {
        system("df -B 1M /dev/shm | tail -1 > /dev/shm/soundkonverter_shm_size");
        QFile chkdf("/dev/shm/soundkonverter_shm_size");
        if( chkdf.open(QIODevice::ReadOnly|QIODevice::Text) )
        {
            QTextStream t( &chkdf );
            QString s = t.readLine();
            QRegularExpression rxlen( "^(?:\\S+)(?:\\s+)(?:\\s+)(\\d+)(?:\\s+)(\\d+)(?:\\s+)(\\d+)(?:\\s+)(\\d+)" );
            QRegularExpressionMatch rxmatch = rxlen.match(s);
            if( rxmatch.hasMatch() )
            {
                data.advanced.sharedMemorySize = rxmatch.captured(1).toInt();
            }
            chkdf.close();
        }
        chkdf.remove();
    }
    data.advanced.maxSizeForSharedMemoryTempFiles = settings.value( "maxSizeForSharedMemoryTempFiles", data.advanced.sharedMemorySize / 4 ).toInt();
    data.advanced.usePipes = settings.value( "usePipes", false ).toBool();
    data.advanced.ejectCdAfterRip = settings.value( "ejectCdAfterRip", true ).toBool();

    settings.beginGroup("CoverArt");
    data.coverArt.writeCovers = settings.value( "writeCovers", 1 ).toInt();
    data.coverArt.writeCoverName = settings.value( "writeCoverName", 0 ).toInt();
    data.coverArt.writeCoverDefaultName = settings.value( "writeCoverDefaultName", i18nc("cover file name","cover") ).toString();

    settings.beginGroup("Backends");
    formats = settings.value( "formats", QStringList() ).toStringList();
    for(const QString& format : formats)
    {
        CodecData codecData;
        codecData.codecName = format;
        codecData.encoders = settings.value( format + "_encoders", QStringList() ).toStringList();
        codecData.decoders = settings.value( format + "_decoders", QStringList() ).toStringList();
        codecData.replaygain = settings.value( format + "_replaygain", QStringList() ).toStringList();
        data.backends.codecs += codecData;
    }
    data.backends.filters = settings.value( "filters", QStringList() ).toStringList();
    data.backends.enabledFilters = settings.value( "enabledFilters", QStringList() ).toStringList();

    pPluginLoader->load();

    QString pluginName;
    QStringList enabledPlugins;
    QStringList newPlugins;
    int codecIndex;

    formats = pPluginLoader->formatList( PluginLoader::Possibilities(PluginLoader::Encode|PluginLoader::Decode|PluginLoader::ReplayGain), PluginLoader::CompressionType(PluginLoader::InferiorQuality|PluginLoader::Lossy|PluginLoader::Lossless|PluginLoader::Hybrid) );

    // build default backend priority list

    for(const QString& format : formats)
    {
        if( format == "wav" )
            continue;

        // get the index of the format in the data.backends.codecs list for direct access
        codecIndex = -1;
        for( int i=0; i<data.backends.codecs.count(); i++ )
        {
            if( data.backends.codecs.at(i).codecName == format )
            {
                codecIndex = i;
                break;
            }
        }
        // add format to the data.backends.codecs list if it isn't already in it
        if( codecIndex == -1 )
        {
            CodecData codecData;
            codecData.codecName = format;
            data.backends.codecs += codecData;
            codecIndex = data.backends.codecs.count() - 1;
        }

        // encoders
        enabledPlugins.clear();
        newPlugins.clear();
        // register existing enabled plugins as such and list new enabled plugins
        for(const ConversionPipeTrunk& trunk : pPluginLoader->conversionFilterPipeTrunks)
        {
            if( trunk.codecTo == format && trunk.enabled )
            {
                pluginName = trunk.plugin->name();
                enabledPlugins += pluginName;
                if( !data.backends.codecs.at(codecIndex).encoders.contains(pluginName) && newPlugins.filter(QRegularExpression("[0-9]{8,8}"+pluginName)).count()==0 )
                {
                    newPlugins += QString::number(trunk.rating).rightJustified(8,'0') + pluginName;
                }
            }
        }
        // remove plugins from the encoder list if they aren't enabled any more
        for( int i=0; i<data.backends.codecs.at(codecIndex).encoders.count(); i++ )
        {
            if( !enabledPlugins.contains(data.backends.codecs.at(codecIndex).encoders.at(i)) )
            {
                data.backends.codecs[codecIndex].encoders.removeAt(i);
                i--;
            }
        }
        // sort new enabled plugins and append them to the encoder list
        newPlugins.sort();
        for( int i=newPlugins.count()-1; i>=0; i-- ) // QStringList doesn't support sorting in descending order
        {
            data.backends.codecs[codecIndex].encoders += newPlugins.at(i).right(newPlugins.at(i).length()-8);
        }

        // decoders
        enabledPlugins.clear();
        newPlugins.clear();
        // register existing enabled plugins as such and list new enabled plugins
        for(const ConversionPipeTrunk& trunk : pPluginLoader->conversionFilterPipeTrunks)
        {
            if( trunk.codecFrom == format && trunk.enabled )
            {
                pluginName = trunk.plugin->name();
                enabledPlugins += pluginName;
                if( !data.backends.codecs.at(codecIndex).decoders.contains(pluginName) && newPlugins.filter(QRegularExpression("[0-9]{8,8}"+pluginName)).count()==0 )
                {
                    newPlugins += QString::number(trunk.rating).rightJustified(8,'0') + pluginName;
                }
            }
        }
        // remove plugins from the decoder list if they aren't enabled any more
        for( int i=0; i<data.backends.codecs.at(codecIndex).decoders.count(); i++ )
        {
            if( !enabledPlugins.contains(data.backends.codecs.at(codecIndex).decoders.at(i)) )
            {
                data.backends.codecs[codecIndex].decoders.removeAt(i);
                i--;
            }
        }
        // sort new enabled plugins and append them to the decoder list
        newPlugins.sort();
        for( int i=newPlugins.count()-1; i>=0; i-- ) // QStringList doesn't support sorting in descending order
        {
            data.backends.codecs[codecIndex].decoders += newPlugins.at(i).right(newPlugins.at(i).length()-8);
        }

        // replaygain
        enabledPlugins.clear();
        const bool internalReplayGainEnabled = pPluginLoader->hasCodecInternalReplayGain(format);
        if( internalReplayGainEnabled )
        {
            enabledPlugins += i18n("Try internal");
        }
        newPlugins.clear();
        // register existing enabled plugins as such and list new enabled plugins
        for(const ReplayGainPipe& pipe : pPluginLoader->replaygainPipes)
        {
            if( pipe.codecName == format && pipe.enabled )
            {
                pluginName = pipe.plugin->name();
                enabledPlugins += pluginName;
                if( !data.backends.codecs.at(codecIndex).replaygain.contains(pluginName) && newPlugins.filter(QRegularExpression("[0-9]{8,8}"+pluginName)).count()==0 )
                {
                    newPlugins += QString::number(pipe.rating).rightJustified(8,'0') + pluginName;
                }
            }
        }
        // remove plugins from the replay gain list if they aren't enabled any more
        for( int i=0; i<data.backends.codecs.at(codecIndex).replaygain.count(); i++ )
        {
            if( !enabledPlugins.contains(data.backends.codecs.at(codecIndex).replaygain.at(i)) )
            {
                data.backends.codecs[codecIndex].replaygain.removeAt(i);
                i--;
            }
        }
        // append internal replay gain if it is enabled
        if( internalReplayGainEnabled && !data.backends.codecs.at(codecIndex).replaygain.contains(i18n("Try internal")) )
        {
            data.backends.codecs[codecIndex].replaygain += i18n("Try internal");
        }
        // sort new enabled plugins and append them to the replay gain list
        newPlugins.sort();
        for( int i=newPlugins.count()-1; i>=0; i-- ) // QStringList doesn't support sorting in descending order
        {
            data.backends.codecs[codecIndex].replaygain += newPlugins.at(i).right(newPlugins.at(i).length()-8);
        }
    }

    // filters
    enabledPlugins.clear();
    newPlugins.clear();
    // register existing enabled plugins as such and list new enabled plugins
    for(FilterPlugin *plugin : pPluginLoader->getAllFilterPlugins())
    {
        pluginName = plugin->name();
        for(const ConversionPipeTrunk& trunk : plugin->codecTable())
        {
            if( trunk.enabled && trunk.codecFrom == "wav" && trunk.codecTo == "wav" )
            {
                enabledPlugins += pluginName;
                if( !data.backends.filters.contains(pluginName) && newPlugins.filter(QRegularExpression("[0-9]{8,8}"+pluginName)).count()==0 )
                {
                    newPlugins += QString::number(trunk.rating).rightJustified(8,'0') + pluginName;
                    break;
                }
            }
        }
    }
    // remove plugins from the filter list if they aren't enabled any more
    for( int i=0; i<data.backends.filters.count(); i++ )
    {
        if( !enabledPlugins.contains(data.backends.filters.at(i)) )
        {
            data.backends.filters.removeAt(i);
            i--;
        }
    }
    // sort new enabled plugins and append them to the filter list
    newPlugins.sort();
    for( int i=newPlugins.count()-1; i>=0; i-- ) // QStringList doesn't support sorting in descending order
    {
        data.backends.filters += newPlugins.at(i).right(newPlugins.at(i).length()-8);
    }
    // since filters can be completely disabled we have to keep track of data.backends.enabledFilters as well
    // remove plugins from the enabledFilters list if they aren't enabled any more
    for( int i=0; i<data.backends.enabledFilters.count(); i++ )
    {
        if( !data.backends.filters.contains(data.backends.enabledFilters.at(i)) )
        {
            data.backends.enabledFilters.removeAt(i);
            i--;
        }
    }
    // always enable the first filter
    if( data.app.configVersion < 1005 && data.backends.enabledFilters.isEmpty() && data.backends.filters.count() > 0 )
    {
        data.backends.enabledFilters.append( data.backends.filters.first() );
    }

    // import KDE4 profiles - paths are hard-coded because I don't know how to properly get the KDE4 data path
    if( data.app.configVersion < 1006 )
    {
        const QString src = QDir::homePath() + "/.kde4/share/apps/soundkonverter/profiles.xml";
        const QString dest = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles.xml";
        if( QFile::exists(src) && !QFile::exists(dest) )
        {
            QFile::copy(src, dest);
            logger->log( 1000, i18n("Importing old profiles from: %1",src) );
        }
    }

    logger->log( 1000, "\nloading profiles ..." );
    QFile listFile( QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles.xml" );
    if( listFile.open( QIODevice::ReadOnly ) )
    {
        QDomDocument list("soundkonverter_profilelist");
        if( list.setContent( &listFile ) )
        {
            QDomElement root = list.documentElement();
            if( root.nodeName() == "soundkonverter" && root.attribute("type") == "profilelist" )
            {
                QDomNodeList conversionOptionsElements = root.elementsByTagName("conversionOptions");
                for( int i=0; i<conversionOptionsElements.count(); i++ )
                {
                    const QString profileName = conversionOptionsElements.at(i).toElement().attribute("profileName");
                    const QString pluginName = conversionOptionsElements.at(i).toElement().attribute("pluginName");
                    CodecPlugin *plugin = qobject_cast<CodecPlugin*>(pPluginLoader->backendPluginByName( pluginName ));
                    if( plugin )
                    {
                        QList<QDomElement> filterOptionsElements;
                        ConversionOptions *conversionOptions = plugin->conversionOptionsFromXml( conversionOptionsElements.at(i).toElement(), &filterOptionsElements );
                        if( conversionOptions )
                        {
                            for(const QDomElement& filterOptionsElement : filterOptionsElements)
                            {
                                FilterOptions *filterOptions = 0;
                                const QString filterPluginName = filterOptionsElement.attribute("pluginName");
                                FilterPlugin *filterPlugin = qobject_cast<FilterPlugin*>(pPluginLoader->backendPluginByName( filterPluginName ));
                                if( filterPlugin )
                                {
                                    filterOptions = filterPlugin->filterOptionsFromXml( filterOptionsElement );
                                }
                                else
                                {
                                    logger->log( 1000, "\tcannot load filter for profile: " + profileName );
                                    continue;
                                }
                                conversionOptions->filterOptions.append( filterOptions );
                            }

                            pConversionOptionsManager->addConversionOptions( conversionOptions );

                            data.profiles[profileName] = conversionOptions->copy();
                            if( profileName != "soundkonverter_last_used" )
                                logger->log( 1000, "\tname: " + profileName + ", plugin: " + pluginName );
                        }
                    }
                    else
                    {
                        logger->log( 1000, "\tname: " + profileName + ", plugin: " + pluginName );
                        logger->log( 1000, "\t\tcannot be loaded beacause the plugin cannot be found" );
                        continue;
                    }
                }
            }
        }
        listFile.close();
    }
    logger->log( 1000, "... all profiles loaded\n" );

    QString profile;
    QStringList sFormat;
    QStringList sProfile;
    sProfile += i18n("Last used");
    sProfile += i18n("Very low");
    sProfile += i18n("Low");
    sProfile += i18n("Medium");
    sProfile += i18n("High");
    sProfile += i18n("Very high");
    sProfile += i18n("Lossless");
    sProfile += i18n("Hybrid");
    sProfile += customProfiles();
    if( sProfile.indexOf(data.general.defaultProfile) == -1 )
    {
        data.general.defaultProfile = i18n("High");
    }
    else
    {
        profile = data.general.defaultProfile;

        if( profile == i18n("Very low") || profile == i18n("Low") || profile == i18n("Medium") || profile == i18n("High") || profile == i18n("Very high") )
        {
            sFormat = pPluginLoader->formatList(PluginLoader::Encode,PluginLoader::Lossy);
        }
        else if( profile == i18n("Lossless") )
        {
            sFormat = pPluginLoader->formatList(PluginLoader::Encode,PluginLoader::Lossless);
        }
        else if( profile == i18n("Hybrid") )
        {
            sFormat = pPluginLoader->formatList(PluginLoader::Encode,PluginLoader::Hybrid);
        }
        else if( profile == i18n("User defined") )
        {
            sFormat = pPluginLoader->formatList(PluginLoader::Encode,PluginLoader::CompressionType(PluginLoader::InferiorQuality|PluginLoader::Lossy|PluginLoader::Lossless|PluginLoader::Hybrid));
        }
        else
        {
            const ConversionOptions *conversionOptions = data.profiles.value( profile );
            if( conversionOptions )
                sFormat += conversionOptions->codecName;
        }
        if( sFormat.indexOf(data.general.defaultFormat) == -1 )
        {
            data.general.defaultFormat = i18n("Last used");
        }
    }

    settings.beginGroup("BackendOptimizationsIgnoreList");
    const int backendOptimizationsIgnoreListCount = settings.value( "count", 0 ).toInt();

    CodecOptimizations::Optimization optimization;
    for( int i=0; i<backendOptimizationsIgnoreListCount; i++ )
    {
        const QStringList backendOptimization = settings.value( QString("ignore_%1").arg(i), QStringList() ).toStringList();
        optimization.codecName = backendOptimization.at(0);
        const QString mode = backendOptimization.at(1);
        if( mode == "Encode" )
        {
            optimization.mode = CodecOptimizations::Optimization::Encode;
        }
        else if( mode == "Decode" )
        {
            optimization.mode = CodecOptimizations::Optimization::Decode;
        }
        else if( mode == "ReplayGain" )
        {
            optimization.mode = CodecOptimizations::Optimization::ReplayGain;
        }
        optimization.currentBackend = backendOptimization.at(2);
        optimization.betterBackend = backendOptimization.at(3);
        optimization.solution = CodecOptimizations::Optimization::Ignore;
        data.backendOptimizationsIgnoreList.optimizationList.append(optimization);
    }

    writeServiceMenu();

    logger->log( 1000, QString("loading of the configuration took %1 ms").arg(time.elapsed()) );
}

void Config::save()
{
    writeServiceMenu();

    QSettings settings("soundkonverterrc", QSettings::IniFormat);

    settings.beginGroup("General");
    settings.setValue( "configVersion", SOUNDKONVERTER_VERSION_NUMBER );
    settings.setValue( "startTab", data.general.startTab );
    settings.setValue( "lastTab", data.general.lastTab );
    settings.setValue( "defaultProfile", data.general.defaultProfile );
    settings.setValue( "lastProfile", data.general.lastProfile );
    settings.setValue( "defaultFormat", data.general.defaultFormat );
    settings.setValue( "lastFormat", data.general.lastFormat );
    settings.setValue( "lastOutputDirectoryMode", data.general.lastOutputDirectoryMode );
    settings.setValue( "specifyOutputDirectory", data.general.specifyOutputDirectory );
    settings.setValue( "metaDataOutputDirectory", data.general.metaDataOutputDirectory );
    settings.setValue( "copyStructureOutputDirectory", data.general.copyStructureOutputDirectory );
    settings.setValue( "lastMetaDataOutputDirectoryPaths", data.general.lastMetaDataOutputDirectoryPaths );
    settings.setValue( "lastNormalOutputDirectoryPaths", data.general.lastNormalOutputDirectoryPaths );
    settings.setValue( "waitForAlbumGain", data.general.waitForAlbumGain );
    settings.setValue( "useVFATNames", data.general.useVFATNames );
    settings.setValue( "copyIfSameCodec", data.general.copyIfSameCodec );
    settings.setValue( "writeLogFiles", data.general.writeLogFiles );
    settings.setValue( "conflictHandling", (int)data.general.conflictHandling );
//     settings.setValue( "priority", data.general.priority );
    settings.setValue( "numFiles", data.general.numFiles );
    settings.setValue( "numReplayGainFiles", data.general.numReplayGainFiles );
//     settings.setValue( "executeUserScript", data.general.executeUserScript );
//     settings.setValue( "showToolBar", data.general.showToolBar );
//     settings.setValue( "outputFilePermissions", data.general.outputFilePermissions );
    settings.setValue( "actionMenuConvertMimeTypes", data.general.actionMenuConvertMimeTypes );
    settings.setValue( "actionMenuReplayGainMimeTypes", data.general.actionMenuReplayGainMimeTypes );
    settings.setValue( "replayGainGrouping", (int)data.general.replayGainGrouping );
    settings.setValue( "preferredOggVorbisExtension", data.general.preferredOggVorbisExtension );
    settings.setValue( "preferredVorbisCommentCommentTag", data.general.preferredVorbisCommentCommentTag );
    settings.setValue( "preferredVorbisCommentTrackTotalTag", data.general.preferredVorbisCommentTrackTotalTag );
    settings.setValue( "preferredVorbisCommentDiscTotalTag", data.general.preferredVorbisCommentDiscTotalTag );

    settings.endGroup();
    settings.beginGroup("Advanced");
    settings.setValue( "useSharedMemoryForTempFiles", data.advanced.useSharedMemoryForTempFiles );
    settings.setValue( "maxSizeForSharedMemoryTempFiles", data.advanced.maxSizeForSharedMemoryTempFiles );
    settings.setValue( "usePipes", data.advanced.usePipes );
    settings.setValue( "ejectCdAfterRip", data.advanced.ejectCdAfterRip );

    settings.endGroup();
    settings.beginGroup("CoverArt");
    settings.setValue( "writeCovers", data.coverArt.writeCovers );
    settings.setValue( "writeCoverName", data.coverArt.writeCoverName );
    settings.setValue( "writeCoverDefaultName", data.coverArt.writeCoverDefaultName );

    settings.endGroup();
    settings.beginGroup("Backends");
    settings.remove( "rippers" );
    QStringList formats;
    for(const CodecData& codec : data.backends.codecs)
    {
        const QString format = codec.codecName;
        settings.setValue( format + "_encoders", codec.encoders );
        settings.setValue( format + "_decoders", codec.decoders );
        settings.setValue( format + "_replaygain", codec.replaygain );
        formats += format;
    }
    settings.setValue( "formats", formats );
    settings.setValue( "filters", data.backends.filters );
    settings.setValue( "enabledFilters", data.backends.enabledFilters );

    settings.beginGroup("BackendOptimizationsIgnoreList");
    settings.setValue( "count", data.backendOptimizationsIgnoreList.optimizationList.count() );

    for( int i=0; i<data.backendOptimizationsIgnoreList.optimizationList.count(); i++ )
    {
        QStringList backendOptimization;
        backendOptimization << data.backendOptimizationsIgnoreList.optimizationList.at(i).codecName;
        if( data.backendOptimizationsIgnoreList.optimizationList.at(i).mode == CodecOptimizations::Optimization::Encode )
        {
            backendOptimization << "Encode";
        }
        else if( data.backendOptimizationsIgnoreList.optimizationList.at(i).mode == CodecOptimizations::Optimization::Decode )
        {
            backendOptimization << "Decode";
        }
        else if( data.backendOptimizationsIgnoreList.optimizationList.at(i).mode == CodecOptimizations::Optimization::ReplayGain )
        {
            backendOptimization << "ReplayGain";
        }
        backendOptimization << data.backendOptimizationsIgnoreList.optimizationList.at(i).currentBackend;
        backendOptimization << data.backendOptimizationsIgnoreList.optimizationList.at(i).betterBackend;
        settings.setValue( QString("ignore_%1").arg(i), backendOptimization );
    }

    emit updateWriteLogFilesSetting( data.general.writeLogFiles );
}

void Config::writeServiceMenu()
{
    QString content;
    QStringList mimeTypes;

    content = "";
    content += "[Desktop Entry]\n";
    content += "Type=Service\n";
    content += "Encoding=UTF-8\n";

    const QStringList convertFormats = pPluginLoader->formatList( PluginLoader::Decode, PluginLoader::CompressionType(PluginLoader::InferiorQuality|PluginLoader::Lossy|PluginLoader::Lossless|PluginLoader::Hybrid) );

    mimeTypes.clear();
    for(const QString& format : convertFormats)
    {
        mimeTypes += pPluginLoader->codecMimeTypes( format );
    }

    content += "ServiceTypes=KonqPopupMenu/Plugin," + mimeTypes.join(",") + "\n";

    content += "Icon=soundkonverter\n";
    content += "Actions=ConvertWithSoundkonverter;\n\n";

    content += "[Desktop Action ConvertWithSoundkonverter]\n";
    content += "Name="+i18n("Convert with soundKonverter")+"\n";
    content += "Icon=soundkonverter\n";
    content += "Exec=soundkonverter %F\n";

    QString convertActionFileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "ServiceMenus/convert_with_soundkonverter.desktop" );
    if( convertActionFileName.isEmpty() )
        convertActionFileName = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/kf6/services/ServiceMenus/convert_with_soundkonverter.desktop";
    if( ( data.general.actionMenuConvertMimeTypes != mimeTypes || !QFile::exists(convertActionFileName) ) && mimeTypes.count() > 0 )
    {
        QDir().mkpath( QFileInfo(convertActionFileName).absolutePath() );
        QFile convertActionFile( convertActionFileName );
        if( convertActionFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
        {
            QTextStream convertActionStream( &convertActionFile );
            convertActionStream << content;
            convertActionFile.close();
        }
        data.general.actionMenuConvertMimeTypes = mimeTypes;
    }

    content = "";
    content += "[Desktop Entry]\n";
    content += "Type=Service\n";
    content += "Encoding=UTF-8\n";

    const QStringList replaygainFormats = pPluginLoader->formatList( PluginLoader::ReplayGain, PluginLoader::CompressionType(PluginLoader::InferiorQuality|PluginLoader::Lossy|PluginLoader::Lossless|PluginLoader::Hybrid) );

    mimeTypes.clear();
    for(const QString& format : replaygainFormats)
    {
        mimeTypes += pPluginLoader->codecMimeTypes( format );
    }

    content += "ServiceTypes=KonqPopupMenu/Plugin," + mimeTypes.join(",") + "\n";

    content += "Icon=soundkonverter_replaygain\n";
    content += "Actions=AddReplayGainWithSoundkonverter;\n\n";

    content += "[Desktop Action AddReplayGainWithSoundkonverter]\n";
    content += "Name="+i18n("Add Replay Gain with soundKonverter")+"\n";
    content += "Icon=soundkonverter-replaygain\n";
    content += "Exec=soundkonverter --replaygain %F\n";

    QString replaygainActionFileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "ServiceMenus/add_replaygain_with_soundkonverter.desktop" );
    if( replaygainActionFileName.isEmpty() )
        replaygainActionFileName = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/kf6/services/ServiceMenus/add_replaygain_with_soundkonverter.desktop";
    if( ( data.general.actionMenuReplayGainMimeTypes != mimeTypes || !QFile::exists(replaygainActionFileName) ) && mimeTypes.count() > 0 )
    {
        QDir().mkpath( QFileInfo(replaygainActionFileName).absolutePath() );
        QFile replaygainActionFile( replaygainActionFileName );
        if( replaygainActionFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
        {
            QTextStream replaygainActionStream( &replaygainActionFile );
            replaygainActionStream << content;
            replaygainActionFile.close();
        }
        data.general.actionMenuReplayGainMimeTypes = mimeTypes;
    }
}

QStringList Config::customProfiles()
{
    QStringList profiles;

    for(const QString& profileName : data.profiles.keys())
    {
        if( profileName == "soundkonverter_last_used" )
            continue;

        const ConversionOptions* profileConversionOptions = data.profiles.value(profileName);

        QList<CodecPlugin*> plugins = pPluginLoader->encodersForCodec( profileConversionOptions->codecName );
        for(const CodecPlugin *plugin : plugins)
        {
            if( plugin->name() == profileConversionOptions->pluginName )
            {
                profiles.append( profileName );
                break;
            }
        }
    }

    return profiles;
}

QList<CodecOptimizations::Optimization> Config::getOptimizations( bool includeIgnored )
{
    QElapsedTimer time;
    time.start();

    QList<CodecOptimizations::Optimization> optimizationList;
    CodecOptimizations::Optimization optimization;

    QStringList tempPluginList;
    QStringList optimizedPluginList;
    int currentBackendRating = 0;
    int betterBackendRating = 0;
    int codecIndex;
    bool ignore;

    const QStringList formats = pPluginLoader->formatList( PluginLoader::Possibilities(PluginLoader::Encode|PluginLoader::Decode|PluginLoader::ReplayGain), PluginLoader::CompressionType(PluginLoader::InferiorQuality|PluginLoader::Lossy|PluginLoader::Lossless|PluginLoader::Hybrid) );
    for(const QString& format : formats)
    {
        if( format == "wav" )
            continue;

        codecIndex = -1;
        for( int j=0; j<data.backends.codecs.count(); j++ )
        {
            if( data.backends.codecs.at(j).codecName == format )
            {
                codecIndex = j;
                break;
            }
        }
        if( codecIndex == -1 )
            continue;

        // encoders
        tempPluginList.clear();
        for( int j=0; j<pPluginLoader->conversionPipeTrunks.count(); j++ )
        {
            if( pPluginLoader->conversionPipeTrunks.at(j).codecTo == format && pPluginLoader->conversionPipeTrunks.at(j).enabled && pPluginLoader->conversionPipeTrunks.at(j).plugin->type() == "codec" )
            {
                const QString pluginName = pPluginLoader->conversionPipeTrunks.at(j).plugin->name();
                if( tempPluginList.filter(QRegularExpression("[0-9]{8,8}"+pluginName)).count() == 0 )
                {
                    tempPluginList += QString::number(pPluginLoader->conversionPipeTrunks.at(j).rating).rightJustified(8,'0') + pluginName;
                }
            }
        }
        for( int j=0; j<pPluginLoader->filterPipeTrunks.count(); j++ )
        {
            if( pPluginLoader->filterPipeTrunks.at(j).codecTo == format && pPluginLoader->filterPipeTrunks.at(j).enabled && pPluginLoader->filterPipeTrunks.at(j).plugin->type() == "filter" )
            {
                const QString pluginName = pPluginLoader->filterPipeTrunks.at(j).plugin->name();
                if( tempPluginList.filter(QRegularExpression("[0-9]{8,8}"+pluginName)).count() == 0 )
                {
                    tempPluginList += QString::number(pPluginLoader->filterPipeTrunks.at(j).rating).rightJustified(8,'0') + pluginName;
                }
            }
        }
        tempPluginList.sort();
        optimizedPluginList.clear();
        for( int j=tempPluginList.count()-1; j>=0; j-- )
        {
            const QString pluginName = tempPluginList.at(j).right(tempPluginList.at(j).length()-8);
            const int pluginRating = tempPluginList.at(j).left(8).toInt();
            optimizedPluginList += pluginName;
            if( data.backends.codecs.at(codecIndex).encoders.count() > 0 && pluginName == data.backends.codecs.at(codecIndex).encoders.first() )
            {
                currentBackendRating = pluginRating;
            }
            if( j == tempPluginList.count()-1 )
            {
                betterBackendRating = pluginRating;
            }
        }
        if( optimizedPluginList.count() != 0 && data.backends.codecs.at(codecIndex).encoders.count() != 0 )
        {
            ignore = false;
            for( int j=0; j<data.backendOptimizationsIgnoreList.optimizationList.count(); j++ )
            {
                if( data.backendOptimizationsIgnoreList.optimizationList.at(j).codecName == format &&
                    data.backendOptimizationsIgnoreList.optimizationList.at(j).mode == CodecOptimizations::Optimization::Encode &&
                    data.backendOptimizationsIgnoreList.optimizationList.at(j).currentBackend == data.backends.codecs.at(codecIndex).encoders.first() &&
                    data.backendOptimizationsIgnoreList.optimizationList.at(j).betterBackend == optimizedPluginList.first()
                )
                {
                    ignore = true;
                }
            }

            // is there a better plugin available and has the better plugin really a higher rating or was it just sorted alphabetically at the top
            if( optimizedPluginList.first() != data.backends.codecs.at(codecIndex).encoders.first() && betterBackendRating > currentBackendRating )
            {
                if( ignore && includeIgnored )
                {
                    optimization.codecName = format;
                    optimization.mode = CodecOptimizations::Optimization::Encode;
                    optimization.currentBackend = data.backends.codecs.at(codecIndex).encoders.first();
                    optimization.betterBackend = optimizedPluginList.first();
                    optimization.solution = CodecOptimizations::Optimization::Ignore;
                    optimizationList.append(optimization);
                }
                else if( !ignore )
                {
                    optimization.codecName = format;
                    optimization.mode = CodecOptimizations::Optimization::Encode;
                    optimization.currentBackend = data.backends.codecs.at(codecIndex).encoders.first();
                    optimization.betterBackend = optimizedPluginList.first();
                    optimization.solution = CodecOptimizations::Optimization::Undecided;
                    optimizationList.append(optimization);
                }
            }
        }

        // decoders
        tempPluginList.clear();
        for( int j=0; j<pPluginLoader->conversionPipeTrunks.count(); j++ )
        {
            if( pPluginLoader->conversionPipeTrunks.at(j).codecFrom == format && pPluginLoader->conversionPipeTrunks.at(j).enabled && pPluginLoader->conversionPipeTrunks.at(j).plugin->type() == "codec" )
            {
                const QString pluginName = pPluginLoader->conversionPipeTrunks.at(j).plugin->name();
                if( tempPluginList.filter(QRegularExpression("[0-9]{8,8}"+pluginName)).count() == 0 )
                {
                    tempPluginList += QString::number(pPluginLoader->conversionPipeTrunks.at(j).rating).rightJustified(8,'0') + pluginName;
                }
            }
        }
        for( int j=0; j<pPluginLoader->filterPipeTrunks.count(); j++ )
        {
            if( pPluginLoader->filterPipeTrunks.at(j).codecFrom == format && pPluginLoader->filterPipeTrunks.at(j).enabled && pPluginLoader->filterPipeTrunks.at(j).plugin->type() == "filter" )
            {
                const QString pluginName = pPluginLoader->filterPipeTrunks.at(j).plugin->name();
                if( tempPluginList.filter(QRegularExpression("[0-9]{8,8}"+pluginName)).count() == 0 )
                {
                    tempPluginList += QString::number(pPluginLoader->filterPipeTrunks.at(j).rating).rightJustified(8,'0') + pluginName;
                }
            }
        }
        tempPluginList.sort();
        optimizedPluginList.clear();
        for( int j=tempPluginList.count()-1; j>=0; j-- )
        {
            const QString pluginName = tempPluginList.at(j).right(tempPluginList.at(j).length()-8);
            const int pluginRating = tempPluginList.at(j).left(8).toInt();
            optimizedPluginList += pluginName;
            if( data.backends.codecs.at(codecIndex).decoders.count() > 0 && pluginName == data.backends.codecs.at(codecIndex).decoders.first() )
            {
                currentBackendRating = pluginRating;
            }
            if( j == tempPluginList.count()-1 )
            {
                betterBackendRating = pluginRating;
            }
        }
        if( optimizedPluginList.count() != 0 && data.backends.codecs.at(codecIndex).decoders.count() != 0 )
        {
            ignore = false;
            for( int j=0; j<data.backendOptimizationsIgnoreList.optimizationList.count(); j++ )
            {
                if( data.backendOptimizationsIgnoreList.optimizationList.at(j).codecName == format &&
                    data.backendOptimizationsIgnoreList.optimizationList.at(j).mode == CodecOptimizations::Optimization::Decode &&
                    data.backendOptimizationsIgnoreList.optimizationList.at(j).currentBackend == data.backends.codecs.at(codecIndex).decoders.first() &&
                    data.backendOptimizationsIgnoreList.optimizationList.at(j).betterBackend == optimizedPluginList.first()
                )
                {
                    ignore = true;
                }
            }

            // is there a better plugin available and has the better plugin really a higher rating or was it just sorted alphabetically at the top
            if( optimizedPluginList.first() != data.backends.codecs.at(codecIndex).decoders.first() && betterBackendRating > currentBackendRating )
            {
                if( ignore && includeIgnored )
                {
                    optimization.codecName = format;
                    optimization.mode = CodecOptimizations::Optimization::Decode;
                    optimization.currentBackend = data.backends.codecs.at(codecIndex).decoders.first();
                    optimization.betterBackend = optimizedPluginList.first();
                    optimization.solution = CodecOptimizations::Optimization::Ignore;
                    optimizationList.append(optimization);
                }
                else if( !ignore )
                {
                    optimization.codecName = format;
                    optimization.mode = CodecOptimizations::Optimization::Decode;
                    optimization.currentBackend = data.backends.codecs.at(codecIndex).decoders.first();
                    optimization.betterBackend = optimizedPluginList.first();
                    optimization.solution = CodecOptimizations::Optimization::Undecided;
                    optimizationList.append(optimization);
                }
            }
        }

        // replaygain
        tempPluginList.clear();
        for( int j=0; j<pPluginLoader->replaygainPipes.count(); j++ )
        {
            if( pPluginLoader->replaygainPipes.at(j).codecName == format && pPluginLoader->replaygainPipes.at(j).enabled )
            {
                const QString pluginName = pPluginLoader->replaygainPipes.at(j).plugin->name();
                if( tempPluginList.filter(QRegularExpression("[0-9]{8,8}"+pluginName)).count() == 0 )
                {
                    tempPluginList += QString::number(pPluginLoader->replaygainPipes.at(j).rating).rightJustified(8,'0') + pluginName;
                }
            }
        }
        tempPluginList.sort();
        optimizedPluginList.clear();
        for( int j=tempPluginList.count()-1; j>=0; j-- )
        {
            const QString pluginName = tempPluginList.at(j).right(tempPluginList.at(j).length()-8);
            const int pluginRating = tempPluginList.at(j).left(8).toInt();
            optimizedPluginList += pluginName;
            if( data.backends.codecs.at(codecIndex).replaygain.count() > 0 && pluginName == data.backends.codecs.at(codecIndex).replaygain.first() )
            {
                currentBackendRating = pluginRating;
            }
            if( j == tempPluginList.count()-1 )
            {
                betterBackendRating = pluginRating;
            }
        }
        if( optimizedPluginList.count() != 0 && data.backends.codecs.at(codecIndex).replaygain.count() != 0 )
        {
            ignore = false;
            for( int j=0; j<data.backendOptimizationsIgnoreList.optimizationList.count(); j++ )
            {
                if( data.backendOptimizationsIgnoreList.optimizationList.at(j).codecName == format &&
                    data.backendOptimizationsIgnoreList.optimizationList.at(j).mode == CodecOptimizations::Optimization::ReplayGain &&
                    data.backendOptimizationsIgnoreList.optimizationList.at(j).currentBackend == data.backends.codecs.at(codecIndex).replaygain.first() &&
                    data.backendOptimizationsIgnoreList.optimizationList.at(j).betterBackend == optimizedPluginList.first()
                )
                {
                    ignore = true;
                    break;
                }
            }

            // is there a better plugin available and has the better plugin really a higher rating or was it just sorted alphabetically at the top
            if( optimizedPluginList.first() != data.backends.codecs.at(codecIndex).replaygain.first() && betterBackendRating > currentBackendRating )
            {
                if( ignore && includeIgnored )
                {
                    optimization.codecName = format;
                    optimization.mode = CodecOptimizations::Optimization::ReplayGain;
                    optimization.currentBackend = data.backends.codecs.at(codecIndex).replaygain.first();
                    optimization.betterBackend = optimizedPluginList.first();
                    optimization.solution = CodecOptimizations::Optimization::Ignore;
                    optimizationList.append(optimization);
                }
                else if( !ignore )
                {
                    optimization.codecName = format;
                    optimization.mode = CodecOptimizations::Optimization::ReplayGain;
                    optimization.currentBackend = data.backends.codecs.at(codecIndex).replaygain.first();
                    optimization.betterBackend = optimizedPluginList.first();
                    optimization.solution = CodecOptimizations::Optimization::Undecided;
                    optimizationList.append(optimization);
                }
            }
        }
    }

    logger->log( 1000, QString("generation of the optimization list took %1 ms").arg(time.elapsed()) );

    return optimizationList;
}

void Config::doOptimizations( const QList<CodecOptimizations::Optimization>& optimizationList )
{
    int codecIndex;

    for( int i=0; i<optimizationList.count(); i++ )
    {
        if( optimizationList.at(i).solution == CodecOptimizations::Optimization::Fix )
        {
            codecIndex = -1;
            for( int j=0; j<data.backends.codecs.count(); j++ )
            {
                if( data.backends.codecs.at(j).codecName == optimizationList.at(i).codecName )
                {
                    codecIndex = j;
                    break;
                }
            }
            if( codecIndex == -1 )
                continue;

            for( int j=0; j<data.backendOptimizationsIgnoreList.optimizationList.count(); j++ )
            {
                if( data.backendOptimizationsIgnoreList.optimizationList.at(j).codecName == optimizationList.at(i).codecName &&
                    data.backendOptimizationsIgnoreList.optimizationList.at(j).mode == optimizationList.at(i).mode &&
                    data.backendOptimizationsIgnoreList.optimizationList.at(j).currentBackend == optimizationList.at(i).currentBackend &&
                    data.backendOptimizationsIgnoreList.optimizationList.at(j).betterBackend == optimizationList.at(i).betterBackend
                )
                {
                    data.backendOptimizationsIgnoreList.optimizationList.removeAt( j );
                    break;
                }
            }

            if( optimizationList.at(i).mode == CodecOptimizations::Optimization::Encode )
            {
                data.backends.codecs[codecIndex].encoders.removeAll( optimizationList.at(i).betterBackend );
                data.backends.codecs[codecIndex].encoders.prepend( optimizationList.at(i).betterBackend );
            }
            else if( optimizationList.at(i).mode == CodecOptimizations::Optimization::Decode )
            {
                data.backends.codecs[codecIndex].decoders.removeAll( optimizationList.at(i).betterBackend );
                data.backends.codecs[codecIndex].decoders.prepend( optimizationList.at(i).betterBackend );
            }
            else if( optimizationList.at(i).mode == CodecOptimizations::Optimization::ReplayGain )
            {
                data.backends.codecs[codecIndex].replaygain.removeAll( optimizationList.at(i).betterBackend );
                data.backends.codecs[codecIndex].replaygain.prepend( optimizationList.at(i).betterBackend );
            }
        }
        else if( optimizationList.at(i).solution == CodecOptimizations::Optimization::Ignore )
        {
            bool found = false;

            for( int j=0; j<data.backendOptimizationsIgnoreList.optimizationList.count(); j++ )
            {
                if( data.backendOptimizationsIgnoreList.optimizationList.at(j).codecName == optimizationList.at(i).codecName &&
                    data.backendOptimizationsIgnoreList.optimizationList.at(j).mode == optimizationList.at(i).mode &&
                    data.backendOptimizationsIgnoreList.optimizationList.at(j).currentBackend == optimizationList.at(i).currentBackend &&
                    data.backendOptimizationsIgnoreList.optimizationList.at(j).betterBackend == optimizationList.at(i).betterBackend
                )
                {
                    found = true;
                    break;
                }
            }

            if( !found )
                data.backendOptimizationsIgnoreList.optimizationList.append(optimizationList.at(i));
        }
    }
}

