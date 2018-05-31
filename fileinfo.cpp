#include "fileinfo.h"

FileInfo::FileInfo(QObject *parent) : QAbstractTableModel(parent)
{
//    path path(L"/home/elkswe");


//    for(directory_entry entry
//        : directory_iterator(path))
//    {
//        try
//        {
//            FileData file;
//            file[NAME] = QString::fromStdWString(entry.path().filename().wstring());
//            QString fileSize;
//            if(is_directory(entry.path()))
//            {
//                fileSize = "<DIR>";
//            }
//            else
//            {
//                fileSize = QString::number(file_size(entry.path())/1024) + " KB";
//            }
//            file[SIZE] =  fileSize;
//            file[DATE] = QString::fromStdWString(
//                        pt::to_simple_wstring(
//                            pt::from_time_t(
//                                last_write_time(entry.path()))));


//            int row = m_files.count();
//            this->beginInsertRows(QModelIndex(), row, row);
//            m_files.append(file);
//            this->endInsertRows();
//        } catch (filesystem_error & ex){
//            std::cout << ex.what() << std::endl;
//        }
//    }

//    qSort(m_files.begin(), m_files.end(), FileInfo::lessThan);
}

int FileInfo::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_files.count();
}

int FileInfo::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return LAST;
}

QVariant FileInfo::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() ||
            m_files.count() <= index.row() ||
            (role != Qt::DisplayRole && role != Qt::EditRole))
    {
        return QVariant();
    }

    return m_files[index.row()][Column(index.column())];
}

bool FileInfo::setData(const QModelIndex &index, const QString &value, int role)
{
    if(!index.isValid() || role != Qt::EditRole || m_files.count() <= index.row())
    {
        return false;
    }

    m_files[index.row()][Column(index.column())] = value;
    //emit dataChanged( index, index );

    return true;
}

QVariant FileInfo::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if(orientation  == Qt::Vertical)
    {
        return section;
    }

    switch (section) {
    case NAME:
        return trUtf8("Name");
    case SIZE:
        return trUtf8("Size");
    case DATE:
        return trUtf8("Date");
    }

    return QVariant();
}

Qt::ItemFlags FileInfo::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    if(index.isValid())
    {
        //flags |= Qt::ItemIsEditable;
    }

    return flags;
}

bool FileInfo::lessThan(const QHash<FileInfo::Column, QString> &fd1, const QHash<FileInfo::Column, QString> &fd2)
{
    if(fd1[SIZE] == "<DIR>" &&
            fd2[SIZE] == "<DIR>")
    {
        return fd1[NAME].toLower() < fd2[NAME].toLower();
    }

    if(fd1[SIZE] == "<DIR>") return true;
    if(fd2[SIZE] == "<DIR>") return false;

    return fd1[NAME].toLower() < fd2[NAME].toLower();
}
