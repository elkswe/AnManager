#ifndef FILEINFO_H
#define FILEINFO_H

#include <QAbstractTableModel>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::filesystem;
namespace pt = boost::posix_time;

class FileInfo : public QAbstractTableModel
{
    Q_OBJECT
public:
    FileInfo( QObject* parent = NULL);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QString &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    enum Column {
        NAME = 0,
        SIZE,
        DATE,
        LAST
    };

    typedef QHash< Column, QString> FileData;
    typedef QList< FileData > Files;

    //My methods
    void appendData(FileData & file);
    void resetData(Files & files);
    void prependData(FileData & file);
    void sort();
    void clear_data();
    static bool lessThan(const QHash< Column, QString> &fd1, const QHash< Column, QString> &fd2);

private:
    Files m_files;

public slots:

};

#endif // FILEINFO_H
