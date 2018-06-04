#include "Utils.h"

utils::utils()
{
}

path utils::extractPathFromSymlink(const path &sym_path)
{
    boost::system::error_code ec;
    if(!exists(sym_path, ec))
        return path();
    if(ec.value() != boost::system::errc::success)
    {
//        showErrorCode(sym_path, ec);
        return path();
    }

    path spath = read_symlink(sym_path, ec);
    if(ec.value() != boost::system::errc::success)
    {
//        showErrorCode(sym_path, ec);
        return path();
    }

    if(spath.empty())
        return path();

    if(spath.is_relative()) spath = sym_path.root_directory() /
            spath;

    if(!exists(spath, ec))
        return path();
    if(ec.value() != boost::system::errc::success)
    {
//        showErrorCode(sym_path, ec);
        return path();
    }

    //MUST CHANGED (MAYBE)
    if(!spath.has_root_directory())
        spath = path(L"/") / spath;
    return spath;
}

QString utils::getSize(const path &_path)
{
    if(_path.empty()) return "";

    boost::system::error_code ec;
    if(!exists(_path, ec))
        return "";
    if(ec.value() != boost::system::errc::success)
    {
//        showErrorCode(_path, ec);
        return "";
    }

    if(is_directory(_path, ec)) return "<DIR>";
    if(ec.value() != boost::system::errc::success)
    {
//        showErrorCode(_path, ec);
        return "";
    }

    boost::uintmax_t fileSize = file_size(_path, ec);
    if(ec.value() != boost::system::errc::success)
    {
//        showErrorCode(_path, ec);
        return "";
    }

    if(fileSize != static_cast<uintmax_t>(-1))
    {
        if( fileSize > GB) return Num_to_QStr(fileSize / GB) + " GB";
        else if( fileSize > MB) return Num_to_QStr(fileSize / MB) + " MB";
        else if( fileSize > KB) return Num_to_QStr(fileSize / KB) + " KB";
        else return Num_to_QStr(fileSize) + " B";
    }

    return "";
}

QString utils::getLastWriteTime(const path &_path)
{
    if(_path.empty()) return "";

    boost::system::error_code ec;
    if(!exists(_path, ec))
        return "";
    if(ec.value() != boost::system::errc::success)
    {
//        showErrorCode(_path, ec);
        return "";
    }

    std::time_t lwtime = last_write_time(_path, ec);
    if(ec.value() != boost::system::errc::success)
    {
//        showErrorCode(_path, ec);
        return "";
    }

    if(lwtime != static_cast<std::time_t>(-1))
        return QString::fromStdWString(
                    pt::to_simple_wstring(
                        pt::from_time_t(lwtime)));

    return "";
}

void utils::showErrorCode(const path & _path, const boost::system::error_code &ec)
{
    QMessageBox::information(Q_NULLPTR, "Oooops",
                             QString::fromStdWString(_path.wstring()) + '\n' +
                             QString::fromStdString(ec.message()) + '\n' +
                             QString::fromStdString(ec.category().name()) + ':' +
                             QString::number(ec.value()));
}
