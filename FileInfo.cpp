#include "FileInfo.h"

FileInfo::FileInfo(QObject *parent) : QAbstractTableModel(parent)
{
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
    emit dataChanged( index, index );

    return true;
}

QVariant FileInfo::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if(orientation == Qt::Vertical)
    {
        return section;
    }

    switch (section) {
    case NAME:
        return "Name";
    case SIZE:
        return "Size";
    case DATE:
        return "Date";
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

void FileInfo::appendData(FileInfo::FileData &file)
{
    int row = m_files.count();
    this->beginInsertRows(QModelIndex(), row, row);
    m_files.append(file);
    this->endInsertRows();
}

void FileInfo::resetData(FileInfo::Files &files)
{
    this->beginResetModel();
    this->m_files.clear();
    this->m_files = files;
    this->endResetModel();
}

void FileInfo::prependData(FileInfo::FileData &file)
{
    int row = m_files.count();
    this->beginInsertRows(QModelIndex(), 0, 0);
    m_files.prepend(file);
    this->endInsertRows();
}

void FileInfo::sort()
{
    qSort(m_files.begin(), m_files.end(), lessThan);
}

void FileInfo::clear_data()
{
    this->beginResetModel();
    this->m_files.clear();
    this->endResetModel();
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
