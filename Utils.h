#ifndef UTILS_H
#define UTILS_H

#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <QString>
#include <QMessageBox>

#define KB 1024
#define MB (KB * 1024)
#define GB (MB * 1024)
#define Num_to_QStr(num) QString::number(num)

using namespace boost::filesystem;
namespace pt = boost::posix_time;

class utils
{
public:
    utils();

    static path extractPathFromSymlink(const path &sym_path);
    static QString getSize(const path &_path);
    static QString getLastWriteTime(const path &_path);
    static void showErrorCode(const path & _path, const boost::system::error_code & ec);

};

#endif // UTILS_H
