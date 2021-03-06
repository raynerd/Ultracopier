/** \file SystrayIcon.cpp
\brief Define the class of the systray icon
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include <QMessageBox>
#include <QMimeData>
#include <QDesktopServices>

#include "SystrayIcon.h"
#include "PluginsManager.h"
#include "ThemesManager.h"
#include "LanguagesManager.h"
#include "HelpDialog.h"

#ifdef Q_OS_MAC
//extern void qt_mac_set_dock_menu(QMenu *menu);
#endif

/// \brief Initiate and show the icon in the systray
SystrayIcon::SystrayIcon(QObject * parent) :
    QSystemTrayIcon(parent)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    //setup the systray icon
    haveListenerInfo	= false;
    havePluginLoaderInfo	= false;
    systrayMenu         = new QMenu();
    actionMenuAbout		= new QAction(this);
    #ifdef ULTRACOPIER_DEBUG
    actionSaveBugReport		= new QAction(this);
    #endif
    actionMenuQuit		= new QAction(this);
    actionOptions		= new QAction(this);
    //actionTransfer		= new QAction(this);
    copyMenu		= new QMenu();
    //to prevent init bug
    stateListener=Ultracopier::NotListening;
    statePluginLoader=Ultracopier::Uncaught;

    setContextMenu(systrayMenu);
    #ifdef ULTRACOPIER_MODE_SUPERCOPIER
    setToolTip(QStringLiteral("Supercopier"));
    #else
    setToolTip(QStringLiteral("Ultracopier"));
    #endif
    #ifdef Q_OS_WIN32
    setIcon(QIcon(QStringLiteral(":/systray_Uncaught_Windows.png")));
    #else
    setIcon(QIcon(QStringLiteral(":/systray_Uncaught_Unix.png")));
    #endif
    //connect the action
    connect(&timerCheckSetTooltip,	&QTimer::timeout,					this,	&SystrayIcon::checkSetTooltip);
    #ifdef ULTRACOPIER_DEBUG
    connect(actionSaveBugReport,	&QAction::triggered,			this,	&SystrayIcon::saveBugReport);
    #endif
    connect(actionMenuQuit,		&QAction::triggered,					this,	&SystrayIcon::hide);
    connect(actionMenuQuit,		&QAction::triggered,					this,	&SystrayIcon::quit);
    connect(actionMenuAbout,	&QAction::triggered,					this,	&SystrayIcon::showHelp);
    connect(actionOptions,		&QAction::triggered,					this,	&SystrayIcon::showOptions);
    connect(this,			&SystrayIcon::activated,                    this,	&SystrayIcon::CatchAction);
    #ifdef ULTRACOPIER_INTERNET_SUPPORT
    connect(this,			&QSystemTrayIcon::messageClicked,           this,	&SystrayIcon::messageClicked);
    #endif
    connect(PluginsManager::pluginsManager,		&PluginsManager::pluginListingIsfinish,			this,	&SystrayIcon::reloadEngineList);
    //display the icon
    updateCurrentTheme();
    //if theme/language change, update graphic part
    connect(ThemesManager::themesManager,			&ThemesManager::theThemeIsReloaded,             this,	&SystrayIcon::updateCurrentTheme, Qt::QueuedConnection);
    connect(LanguagesManager::languagesManager,	&LanguagesManager::newLanguageLoaded,			this,	&SystrayIcon::retranslateTheUI, Qt::QueuedConnection);
    systrayMenu->addMenu(copyMenu);
    systrayMenu->addAction(actionOptions);
    systrayMenu->addAction(actionMenuAbout);
    #ifdef ULTRACOPIER_DEBUG
    systrayMenu->addAction(actionSaveBugReport);
    #endif
    systrayMenu->addAction(actionMenuQuit);
    #ifndef Q_OS_MAC
    systrayMenu->insertSeparator(actionOptions);
    #endif
    retranslateTheUI();
    updateSystrayIcon();

    #ifdef ULTRACOPIER_INTERNET_SUPPORT
    lastVersion=ULTRACOPIER_VERSION;
    #endif

    timerCheckSetTooltip.setSingleShot(true);
    timerCheckSetTooltip.start(1000);

    //impossible with Qt on systray
    /// \note important for drag and drop, \see dropEvent()
    systrayMenu->setAcceptDrops(true);

    #ifdef Q_OS_MAC
//    qt_mac_set_dock_menu(systrayMenu);
    #endif

    show();
}

/// \brief Hide and destroy the icon in the systray
SystrayIcon::~SystrayIcon()
{
    delete actionMenuQuit;
    #ifdef ULTRACOPIER_DEBUG
    delete actionSaveBugReport;
    #endif
    delete actionMenuAbout;
    delete actionOptions;
    delete systrayMenu;
    delete copyMenu;
}

void SystrayIcon::checkSetTooltip()
{
    if(isSystemTrayAvailable())
    {
        #ifdef ULTRACOPIER_MODE_SUPERCOPIER
        setToolTip(QStringLiteral("Supercopier"));
        #else
        setToolTip(QStringLiteral("Ultracopier"));
        #endif
        updateSystrayIcon();
    }
    else
        timerCheckSetTooltip.start();
}

void SystrayIcon::listenerReady(const Ultracopier::ListeningState &state,const bool &havePlugin,const bool &someAreInWaitOfReply)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("state: %1, havePlugin: %2, someAreInWaitOfReply: %3").arg(state).arg(havePlugin).arg(someAreInWaitOfReply));
    Q_UNUSED(someAreInWaitOfReply);
    stateListener=state;
    haveListenerInfo=true;
    haveListener=havePlugin;
    updateSystrayIcon();
    if(!havePlugin)
        showTryCatchMessageWithNoListener();
}

void SystrayIcon::pluginLoaderReady(const Ultracopier::CatchState &state,const bool &havePlugin,const bool &someAreInWaitOfReply)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("state: %1, havePlugin: %2, someAreInWaitOfReply: %3").arg(state).arg(havePlugin).arg(someAreInWaitOfReply));
    Q_UNUSED(someAreInWaitOfReply);
    statePluginLoader=state;
    havePluginLoaderInfo=true;
    havePluginLoader=havePlugin;
    updateSystrayIcon();
}

void SystrayIcon::showTryCatchMessageWithNoListener()
{
    showSystrayMessage(tr("No copy listener found. Do the copy manually by right click one the system tray icon."));
}

/// \brief To show a message linked to the systray icon
void SystrayIcon::showSystrayMessage(const QString& text)
{
    showMessage(tr("Information"),text,QSystemTrayIcon::Information,0);
}

#ifdef ULTRACOPIER_INTERNET_SUPPORT
void SystrayIcon::messageClicked()
{
    QDesktopServices::openUrl(HelpDialog::getUpdateUrl());
}
#endif

/// \brief To update the systray icon
void SystrayIcon::updateSystrayIcon()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start, haveListenerInfo %1, havePluginLoaderInfo: %2").arg(haveListenerInfo).arg(havePluginLoaderInfo));
    QString toolTip=QStringLiteral("???");
    QString icon;
    if(!haveListenerInfo || !havePluginLoaderInfo)
    {
        toolTip=tr("Searching information...");
        icon=QStringLiteral("Uncaught");
    }
    else
    {
        Ultracopier::ListeningState stateListener=this->stateListener;
        Ultracopier::CatchState statePluginLoader=this->statePluginLoader;
        if(!haveListener)
            stateListener=Ultracopier::NotListening;
        if((stateListener==Ultracopier::NotListening && statePluginLoader==Ultracopier::Uncaught) || (stateListener==Ultracopier::SemiListening && statePluginLoader==Ultracopier::Semiuncaught) || (stateListener==Ultracopier::FullListening && statePluginLoader==Ultracopier::Caught))
        {
            if(stateListener==Ultracopier::NotListening)
            {
                toolTip=tr("Do not replace the explorer copy/move");
                icon=QStringLiteral("Uncaught");
            }
            else if(stateListener==Ultracopier::SemiListening)
            {
                toolTip=tr("Semi replace the explorer copy/move");
                icon=QStringLiteral("Semiuncaught");
            }
            else
            {
                toolTip=tr("Replace the explorer copy/move");
                icon=QStringLiteral("Caught");
            }
        }
        else
        {
            icon=QStringLiteral("Semiuncaught");
            QString first_part;
            QString second_part;
            if(stateListener==Ultracopier::NotListening)
                first_part=QStringLiteral("No listening");
            else if(stateListener==Ultracopier::SemiListening)
                first_part=QStringLiteral("Semi listening");
            else if(stateListener==Ultracopier::FullListening)
                first_part=QStringLiteral("Full listening");
            else
                first_part=QStringLiteral("Unknow listening");
            if(statePluginLoader==Ultracopier::Uncaught)
                second_part=QStringLiteral("No replace");
            else if(statePluginLoader==Ultracopier::Semiuncaught)
                second_part=QStringLiteral("Semi replace");
            else if(statePluginLoader==Ultracopier::Caught)
                second_part=QStringLiteral("Full replace");
            else
                second_part=QStringLiteral("Unknow replace");
            toolTip=first_part+QStringLiteral("/")+second_part;
        }
    }
    QIcon theNewSystrayIcon;
    #ifdef Q_OS_WIN32
    theNewSystrayIcon=ThemesManager::themesManager->loadIcon(QStringLiteral("SystemTrayIcon/systray_")+icon+QStringLiteral("_Windows.png"));
    #else
    theNewSystrayIcon=ThemesManager::themesManager->loadIcon(QStringLiteral("SystemTrayIcon/systray_")+icon+QStringLiteral("_Unix.png"));
    #endif
    if(theNewSystrayIcon.isNull())
    {
        #ifdef Q_OS_WIN32
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"use the default systray icon: :/systray_"+icon+"_Windows.png");
        theNewSystrayIcon=QIcon(QStringLiteral(":/systray_")+icon+QStringLiteral("_Windows.png"));
        #else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"use the default systray icon: :/systray_"+icon+"_Unix.png");
        theNewSystrayIcon=QIcon(QStringLiteral(":/systray_")+icon+QStringLiteral("_Unix.png"));
        #endif
    }
    else
    {
        #ifdef Q_OS_WIN32
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"icon: systray_"+icon+"_Windows.png");
        #else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"icon: systray_"+icon+"_Unix.png");
        #endif
    }
    if(theNewSystrayIcon.isNull())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"All the icon include the default icon remain null");
    setIcon(theNewSystrayIcon);
    #ifdef ULTRACOPIER_MODE_SUPERCOPIER
    setToolTip(QStringLiteral("Supercopier - ")+toolTip);
    #else
    setToolTip(QStringLiteral("Ultracopier - ")+toolTip);
    #endif
}

/* drag event processing (impossible with Qt on systray)

need setAcceptDrops(true); into the constructor
need implementation to accept the drop:
void dragEnterEvent(QDragEnterEvent* event);
void dragMoveEvent(QDragMoveEvent* event);
void dragLeaveEvent(QDragLeaveEvent* event);
*/
void SystrayIcon::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    if(mimeData->hasUrls())
    {
        //impossible with Qt on systray
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"mimeData->urls().size()"+QString::number(mimeData->urls().size()));
        emit urlDropped(mimeData->urls());
        event->acceptProposedAction();
    }
}

void SystrayIcon::dragEnterEvent(QDragEnterEvent* event)
{
    // if some actions should not be usable, like move, this code must be adopted
    event->acceptProposedAction();
}

void SystrayIcon::dragMoveEvent(QDragMoveEvent* event)
{
    // if some actions should not be usable, like move, this code must be adopted
    event->acceptProposedAction();
}

void SystrayIcon::dragLeaveEvent(QDragLeaveEvent* event)
{
    event->accept();
}


/// \brief To update the current themes
void SystrayIcon::updateCurrentTheme()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("icon: start"));
    //load the systray menu item
    QIcon tempIcon;

    tempIcon=ThemesManager::themesManager->loadIcon(QStringLiteral("SystemTrayIcon/exit.png"));
    if(!tempIcon.isNull())
        IconQuit=QIcon(tempIcon);
    else
        IconQuit=QIcon(QStringLiteral(""));
    actionMenuQuit->setIcon(IconQuit);

    #ifdef ULTRACOPIER_DEBUG
    actionSaveBugReport->setIcon(QIcon(":/warning.png"));
    #endif

    tempIcon=ThemesManager::themesManager->loadIcon(QStringLiteral("SystemTrayIcon/informations.png"));
    if(!tempIcon.isNull())
        IconInfo=QIcon(tempIcon);
    else
        IconInfo=QIcon(QStringLiteral(""));
    actionMenuAbout->setIcon(IconInfo);

    tempIcon=ThemesManager::themesManager->loadIcon(QStringLiteral("SystemTrayIcon/options.png"));
    if(!tempIcon.isNull())
        IconOptions=QIcon(tempIcon);
    else
        IconOptions=QIcon(QStringLiteral(""));
    actionOptions->setIcon(IconOptions);

    tempIcon=ThemesManager::themesManager->loadIcon(QStringLiteral("SystemTrayIcon/add.png"));
    if(!tempIcon.isNull())
        IconAdd=QIcon(tempIcon);
    else
        IconAdd=QIcon(QStringLiteral(""));
    copyMenu->setIcon(IconAdd);

    //update the systray icon
    updateSystrayIcon();
    reloadEngineList();
}

/* \brief For catch an action on the systray icon
\param reason Why it activated */
void SystrayIcon::CatchAction(QSystemTrayIcon::ActivationReason reason)
{
    if(reason==QSystemTrayIcon::DoubleClick || reason==QSystemTrayIcon::Trigger)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Double click on system tray icon");
        if(stateListener!=Ultracopier::NotListening)
            emit tryUncatchCopy();
        else
        {
            if(!haveListener)
            {
                showTryCatchMessageWithNoListener();
                return;
            }
            emit tryCatchCopy();
        }
    }
    else if(reason==QSystemTrayIcon::Context)//do nothing on right click to show as auto the menu
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("The action on the systray icon is unknown: %1").arg(reason));
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("The action on the systray icon is unknown: %1").arg(reason));
        QMessageBox::warning(NULL,tr("Warning"),tr("The action on the systray icon is unknown!"));
    }
}

/// \brief To catch copy menu action
void SystrayIcon::CatchCopyQuery()
{
    QAction * currentAction=qobject_cast<QAction *>(QObject::sender());
    if(currentAction==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"action not found");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+currentAction->data().toString());
    emit addWindowCopyMove(Ultracopier::Copy,currentAction->data().toString());
}

/// \brief To catch move menu action
void SystrayIcon::CatchMoveQuery()
{
    QAction * currentAction=qobject_cast<QAction *>(QObject::sender());
    if(currentAction==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"action not found");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+currentAction->data().toString());
    emit addWindowCopyMove(Ultracopier::Move,currentAction->data().toString());
}

/// \brief To catch transfer menu action
void SystrayIcon::CatchTransferQuery()
{
    QAction * currentAction=qobject_cast<QAction *>(QObject::sender());
    if(currentAction==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"action not found");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+currentAction->data().toString());
    emit addWindowTransfer(currentAction->data().toString());
}

/// \brief to retranslate the ui
void SystrayIcon::retranslateTheUI()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"retranslateTheUI");
    #ifdef ULTRACOPIER_DEBUG
    actionMenuAbout		->setText(tr("&About/Debug report"));
    #else // ULTRACOPIER_DEBUG
    actionMenuAbout		->setText(tr("&About"));
    #endif // ULTRACOPIER_DEBUG
    #ifdef ULTRACOPIER_DEBUG
    actionSaveBugReport->setText(tr("&Save bug report"));
    #endif
    actionMenuQuit		->setText(tr("&Quit"));
    actionOptions		->setText(tr("&Options"));
    copyMenu		->setTitle(tr("A&dd copy/moving"));
    reloadEngineList();
    updateSystrayIcon();
}

void SystrayIcon::addCopyEngine(const QString &name,const bool &canDoOnlyCopy)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    EngineEntry entry;
    entry.name=name;
    entry.canDoOnlyCopy=canDoOnlyCopy;
    engineEntryList << entry;
    if(PluginsManager::pluginsManager->allPluginHaveBeenLoaded())
        reloadEngineList();
}

void SystrayIcon::removeCopyEngine(const QString &name)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    int index=0;
    while(index<engineEntryList.size())
    {
        if(engineEntryList.at(index).name==name)
        {
            engineEntryList.removeAt(index);
            break;
        }
        index++;
    }
    reloadEngineList();
}

#ifdef ULTRACOPIER_INTERNET_SUPPORT
void SystrayIcon::newUpdate(const QString &version)
{
    /*if(version==lastVersion)
        return;*/
    lastVersion=version;
    showSystrayMessage(tr("New version: %1").arg(version)+"\n"+
                           #if defined(ULTRACOPIER_CGMINER) || !defined(ULTRACOPIER_VERSION_ULTIMATE)
                               tr("Click here to go on download page")
                           #else
                               tr("Click here to go to the shop and login.\nDownload the new version into the order details.\nThe new version have been sended by email too, look into your spams if needed.")
                           #endif
                       );
}
#endif

void SystrayIcon::reloadEngineList()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    copyMenu->clear();
    if(engineEntryList.size()==0)
    {
        copyMenu->setEnabled(false);
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"engineEntryList.size(): "+QString::number(engineEntryList.size()));
    copyMenu->setEnabled(true);
    if(engineEntryList.size()==1)
    {
        QAction *copy=new QAction(IconAdd,tr("&Copy"),copyMenu);
        connect(copy,&QAction::triggered,this,&SystrayIcon::CatchCopyQuery);
        copy->setData(engineEntryList.first().name);
        copyMenu->addAction(copy);
        if(!engineEntryList.first().canDoOnlyCopy)
        {
            QAction *transfer=new QAction(IconAdd,tr("&Transfer"),copyMenu);
            connect(transfer,&QAction::triggered,this,&SystrayIcon::CatchTransferQuery);
            transfer->setData(engineEntryList.first().name);
            copyMenu->addAction(transfer);
            QAction *move=new QAction(IconAdd,tr("&Move"),copyMenu);
            connect(move,&QAction::triggered,this,&SystrayIcon::CatchMoveQuery);
            move->setData(engineEntryList.first().name);
            copyMenu->addAction(move);
        }
    }
    else
    {
        int index=0;
        while(index<engineEntryList.size())
        {
            QMenu * menu=new QMenu(engineEntryList.at(index).name);
            QAction *copy=new QAction(IconAdd,tr("Add &copy"),menu);
            connect(copy,&QAction::triggered,this,&SystrayIcon::CatchCopyQuery);
            copy->setData(engineEntryList.at(index).name);
            menu->addAction(copy);
            if(!engineEntryList.at(index).canDoOnlyCopy)
            {
                QAction *transfer=new QAction(IconAdd,tr("Add &transfer"),menu);
                connect(transfer,&QAction::triggered,this,&SystrayIcon::CatchTransferQuery);
                transfer->setData(engineEntryList.at(index).name);
                menu->addAction(transfer);
                QAction *move=new QAction(IconAdd,tr("Add &move"),menu);
                connect(move,&QAction::triggered,this,&SystrayIcon::CatchMoveQuery);
                move->setData(engineEntryList.at(index).name);
                menu->addAction(move);
            }
            copyMenu->addMenu(menu);
            index++;
        }
    }
}
