#include "DriveManagement.h"

#include <QDir>
#include <QFileInfoList>

DriveManagement::DriveManagement()
{
    #ifdef Q_OS_WIN32
    reg1=QRegularExpression(QStringLiteral("^(\\\\\\\\|//)[^\\\\\\\\/]+(\\\\|/)[^\\\\\\\\/]+"));
    reg2=QRegularExpression(QStringLiteral("^((\\\\\\\\|//)[^\\\\\\\\/]+(\\\\|/)[^\\\\\\\\/]+).*$"));
    reg3=QRegularExpression(QStringLiteral("^[a-zA-Z]:[\\\\/]"));
    reg4=QRegularExpression(QStringLiteral("^([a-zA-Z]:[\\\\/]).*$"));
    #endif
}

//get drive of an file or folder
QString DriveManagement::getDrive(const QString &fileOrFolder) const
{
    QString inode=QDir::toNativeSeparators(fileOrFolder);
    int size=mountSysPoint.size();
    for (int i = 0; i < size; ++i) {
        if(inode.startsWith(mountSysPoint.at(i)))
            return QDir::toNativeSeparators(mountSysPoint.at(i));
    }
    #ifdef Q_OS_WIN32
    if(fileOrFolder.contains(reg1))
    {
        QString returnString=fileOrFolder;
        returnString.replace(reg2,QStringLiteral("\\1"));
        return returnString;
    }
    //due to lack of WMI support into mingw, the new drive event is never called, this is a workaround
    if(fileOrFolder.contains(reg3))
    {
        QString returnString=fileOrFolder;
        returnString.replace(reg4,QStringLiteral("\\1"));
        return QDir::toNativeSeparators(returnString).toUpper();
    }
    #endif
    //if unable to locate the right mount point
    return QString();
}

QStorageInfo::DriveType DriveManagement::getDriveType(const QString &drive) const
{
    int index=mountSysPoint.indexOf(drive);
    if(index!=-1)
        return driveType.at(index);
    return QStorageInfo::UnknownDrive;
}

void DriveManagement::setDrive(const QStringList &mountSysPoint, const QList<QStorageInfo::DriveType> &driveType)
{
    this->mountSysPoint=mountSysPoint;
    this->driveType=driveType;
}

bool DriveManagement::isSameDrive(const QString &file1,const QString &file2) const
{
    if(mountSysPoint.size()==0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("no mount point found"));
        return false;
    }
    QString drive1=getDrive(file1);
    if(drive1.isEmpty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("drive for the file1 not found: %1").arg(file1));
        return false;
    }
    QString drive2=getDrive(file2);
    if(drive2.isEmpty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("drive for the file2 not found: %1").arg(file2));
        return false;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("%1 is egal to %2?").arg(drive1).arg(drive2));
    if(drive1==drive2)
        return true;
    else
        return false;
}
