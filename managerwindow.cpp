#include "managerwindow.h"

ManagerWindow::ManagerWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::ManagerWindow)
{
	ui->setupUi(this);

	this->setWindowTitle("AnManager v1.0");

	fOps.op = NULL;
	fOps.from = path();
	fOps.index = QModelIndex();
	ui->hBoxForDisks->setAlignment(Qt::AlignLeft);
	this->resize(750, 750);

	configureTableView();
	mountSections();

	//CHANGE
	ui->curr_path_label->setText(QString::fromStdWString(this->curr_path.wstring()));
	ui->curr_path_label->repaint();
}

ManagerWindow::~ManagerWindow()
{
	delete ui;
	this->sections.clear();
}

void ManagerWindow::configureTableView()
{
	QTableView* m_view = ui->mainTableView;
    m_view->verticalHeader()->setVisible(false);
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_view->setModel(m_model = new FileInfo);
	m_view->setShowGrid(true);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection); //maybe multiple
	m_view->setSelectionBehavior(QAbstractItemView::SelectRows);

	m_view->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(m_view, SIGNAL(customContextMenuRequested(QPoint)),
		this, SLOT(slotCustomMenuRequested(QPoint)));

	connect(m_view, SIGNAL(doubleClicked(QModelIndex)),
		this, SLOT(mainTableView_dbClicked_handler(QModelIndex)));

	connect(ui->cmd, SIGNAL(returnPressed()),
		this, SLOT(cmd_returnPressed_handler()));

    connect(ui->shwHidden, SIGNAL(stateChanged(int)),
            this, SLOT(slotChngHiddenState(int)));
}
//NEED CHECKING
void ManagerWindow::mountSections()
{
#ifdef __linux__
	addSection(path(L"/"), "Root");
	char* username = getenv("USER");
	addSection(path("/home/") /= username, "Home dir");

	path mount_devices(L"/media/");
	mount_devices /= username;
	for (directory_entry d_entry
		: directory_iterator(mount_devices))
	{
		if (is_directory(d_entry.path()) &&
			exists(d_entry.path()))
		{
			addSection(d_entry.path(),
				QString::fromStdWString(
					d_entry.path().filename().wstring()));
		}
	}
#elif _WIN32 | _WIN64
	int bitValue;
	DWORD dr = GetLogicalDrives(); // функция возвращает битовую маску
	for (int bitNumber = 0; bitNumber < 26; bitNumber++) // проходимся циклом по битам
	{
		bitValue = ((dr >> bitNumber) & 1); // узнаём значение текущего бита
		if (bitValue) // если единица - диск с номером bitNumber есть
		{
			std::wstring drive_litera;
			drive_litera += (wchar_t)(65 + bitNumber); // получаем литеру диска
			std::wstring drive_path(drive_litera);
			drive_path += L":\\";

			if (exists(drive_path))
			{
				UINT drive_type = GetDriveType(drive_path.c_str()); // узнаём тип диска

				if (drive_type == DRIVE_FIXED
					|| drive_type == DRIVE_REMOVABLE
					|| drive_type == DRIVE_CDROM)
				{
					QString drive_label = QString::fromStdWString(drive_litera);
					drive_label += " [";
					if (drive_type != DRIVE_CDROM)
					{
						wchar_t drive_label_buff[30];
						if (GetVolumeInformationW(
							drive_path.c_str(),
							drive_label_buff,
							sizeof(drive_label_buff),
							NULL,
							NULL,
							NULL,
							NULL,
							NULL))
						{
							drive_label += QString::fromWCharArray(drive_label_buff);
						}
						drive_label += "]";
						addSection(drive_path, drive_label);
					}
				}
			}
		}
	}
#endif
	if (!this->sections.isEmpty())
		this->curr_path = this->sections.first()->diskPath;
	else curr_path = current_path();
	fillWithData(curr_path);
	//CHANGE
	ui->curr_path_label->setText(QString::fromStdWString(this->curr_path.wstring()));
	ui->curr_path_label->repaint();
}

void ManagerWindow::addSection(const path _path, QString _name)
{
	if (_name == NULL) _name = QString::fromStdWString(_path.filename().wstring());
	DiskButton *d_btn = new DiskButton(_path, _name);
	d_btn->setFont(QFont("Tahoma", 10));
	connect(d_btn, SIGNAL(diskClicked(const path&)),
		this, SLOT(disk_button_clicked_handler(const path&)));
	ui->hBoxForDisks->addWidget(d_btn, 0, Qt::AlignLeft);
	this->sections.push_back(d_btn);
}

bool ManagerWindow::fillWithData(const path &_path)
{
	boost::system::error_code ec;
	if (!exists(_path, ec)) return false;
	if (ec.value() != boost::system::errc::success)
		return false;

	FileInfo::Files new_files;

	for (directory_entry entry
		: directory_iterator(_path, ec))
	{
		try
        {
#ifdef _WIN32 | _WIN64
            if (!(FILE_ATTRIBUTE_HIDDEN & GetFileAttributesW(entry.path().c_str()))
                    || this->fHidden)
#elif __linux__
            if ((entry.path().filename().wstring()[0] != L'.'
                 && !entry.path().filename_is_dot_dot()
                 && entry.path().filename() != path(L"./"))
                    || this->fHidden)
#endif
            {
                path file_path = entry.path();
                FileInfo::FileData file = fillFileData(file_path);
                new_files.append(file);
            }
        }
		catch (filesystem_error & ex) {
#ifdef MDEBUG
			std::cout << ex.what() << std::endl
				<< "path1: " << ex.path1() << std::endl
				<< "path2: " << ex.path2() << std::endl;
			std::printf("perms: %o\n", status(ex.path1()).permissions());
			std::cout << ex.code().category().name() << ':'
				<< ex.code().value() << std::endl;
#endif
			QMessageBox::information(this, "Oooops",
				QString::fromStdWString(ex.path1().wstring()) + "\n" +
				QString::fromLocal8Bit(ex.code().message().c_str()) + "\n" +
				QString::fromStdString(ex.code().category().name()) + ':' +
				QString::number(ex.code().value()));
		}
		catch (...)
		{
			QMessageBox::critical(this, "Magic", "So what did you do?");
		}
	}
	if (ec.value() == boost::system::errc::success)
	{
		m_model->resetData(new_files);
		m_model->sort();
	}
	else
	{
		utils::showErrorCode(_path, ec);
		return false;
	}

	if (_path != _path.root_path())
	{
		FileInfo::FileData to_past;
		to_past[FileInfo::NAME] = "..";
		to_past[FileInfo::SIZE] = "";
		to_past[FileInfo::DATE] = "";
		this->m_model->prependData(to_past);
	}

	return true;
}

FileInfo::FileData ManagerWindow::fillFileData(const path & _path)
{
	FileInfo::FileData file;
	path file_path = _path;

	file[FileInfo::NAME] = QString::fromStdWString(
		file_path.filename().wstring());

	if (is_symlink(file_path) && symbolic_link_exists(file_path))
		file_path = utils::extractPathFromSymlink(file_path);

	file[FileInfo::SIZE] = utils::getSize(file_path);

	file[FileInfo::DATE] = utils::getLastWriteTime(file_path);

	return file;
}

void ManagerWindow::readThePath(path &_path)
{
	try
	{
		boost::system::error_code ec;
		if (!exists(_path, ec))
		{
			QMessageBox::information(this, "Error!",
				"The file does not exist.");
			return;
		}

		if (_path.filename() == L".." && _path.has_parent_path())
		{
			_path = _path.parent_path();
			if (_path != _path.root_path() && _path.has_parent_path())
			{
				path prev_dir = _path.parent_path();
				if (exists(prev_dir))
				{
					if (fillWithData(prev_dir)) curr_path = prev_dir;
					return;
				}
			}
		}

		if (is_regular_file(_path))
		{
#ifdef _WIN32 | _WIN64
			if (_path.extension() != L".lnk")
				ShellExecuteW(NULL, L"open", _path.wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
			else
			{
				LPCTSTR lpszShortcutPath = _path.wstring().c_str();
				TCHAR szFilePath[MAX_PATH];
				HRESULT hres = ResolveShortcut(lpszShortcutPath, szFilePath);
				if (hres == S_OK)
					readThePath(path(szFilePath));
			}
#elif __linux__  //added
			QString command("xdg-open \"");
			command += QString::fromStdWString(_path.wstring());
			command += '\"';
			system(command.toStdString().c_str());
#endif
			return;
		}

		if (is_symlink(_path) && symbolic_link_exists(_path))
		{
#ifdef __linux__
			path sym_path = utils::extractPathFromSymlink(_path);
			if (sym_path.empty())
				QMessageBox::information(this, "Ooops", "The link is incorrect.");
			else
				readThePath(sym_path);
#elif _WIN32 | _WIN64
			if (IsDirectoryJunction(_path.c_str()))
			{
                LPTSTR dest = new TCHAR[32767];
                if (GetJunctionDestination(_path.c_str(), dest))
                {
                    path sym_path(dest);
                    readThePath(sym_path);
                }
				else
					QMessageBox::information(this, "Ooops", "The link (junction) is incorrect.");
			}
			else
				QMessageBox::information(this, "Ooops", "Is not junction! (Something interesting...)");
#endif
			return;
		}

		if (is_directory(_path))
		{
			if (fillWithData(_path)) curr_path = _path;
			return;
		}

		if (is_other(_path))
			QMessageBox::information(this, "Sorry",
				"This file can not be opened.");

	}
	catch (filesystem_error & ex) {
#ifdef DEBUG
		std::cout << ex.what() << std::endl
			<< "path1: " << ex.path1() << std::endl
			<< "path2: " << ex.path2() << std::endl;
		std::printf("perms: %o\n", status(ex.path1()).permissions());
		std::cout << ex.code().category().name() << ':'
			<< ex.code().value() << std::endl;
#endif
		QMessageBox::information(this, "Oooops",
			QString::fromStdWString(ex.path1().wstring()) + "\n" +
			QString::fromLocal8Bit(ex.code().message().c_str()) + "\n" +
			QString::fromStdString(ex.code().category().name()) + ':' +
			QString::number(ex.code().value()));
	}
}

void ManagerWindow::deleteFile(const path & path, const QModelIndex & index, int ask)
{
	boost::system::error_code ec;
	int res;

	if (is_regular_file(path) || is_symlink(path))
	{
        if (ask) res = QMessageBox::information(0,
			"Deleting",
			"Delete the selected file " +
			QString::fromStdWString(path.filename().wstring()) + " ?",
			QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        else res = QMessageBox::Yes;

		if (res == QMessageBox::Yes)
		{
#ifdef _WIN32 | _WIN64
			if (IsDirectoryJunction(path.wstring().c_str()))
			{
				if (!DeleteJunctionPoint(path.wstring().c_str()))
					QMessageBox::information(this, "Ooops", "Error: system:" + QString::number(GetLastError()));
				if (exists(path) && is_directory(path))
				{
					fs::remove_all(path, ec);
					if (ec.value() == boost::system::errc::success)
						m_model->remove(index);
					else
					{
						utils::showErrorCode(path, ec);
						ec.clear();
					}
				}
				return;
			}
#endif
			fs::remove(path, ec);
            if (ec.value() == boost::system::errc::success && index.isValid())
            {if(curr_path == path.parent_path()) m_model->remove(index);}
			else
				utils::showErrorCode(path, ec);
		}
		return;
	}

	if (is_directory(path))
	{
        if (ask) res = QMessageBox::information(0,
			"Deleting",
			"Delete the selected directory " +
			QString::fromStdWString(path.filename().wstring()) + " ?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        else res = QMessageBox::Yes;

		if (res == QMessageBox::Yes)
		{
			fs::remove_all(path, ec);
            if (ec.value() == boost::system::errc::success && index.isValid())
            {if(curr_path == path.parent_path()) m_model->remove(index);}
            else
				utils::showErrorCode(path, ec);
		}
        return;
	}
}

void ManagerWindow::createFileWithExtension(const char *ext)
{
	bool bOK;
	QString name = QInputDialog::getText(
		this, "New text file",
		"Name:", QLineEdit::Normal,
		QString(ext),
		&bOK);


	if (bOK)
	{
		boost::system::error_code ec;
		path file_path(curr_path /
			name.toStdWString());

		if (exists(file_path, ec)) return;

        std::wofstream wofs(file_path.string());
		if (!wofs.is_open())
			QMessageBox::information(this, "Error", "File is not created.");
		wofs.close();
		last_write_time(file_path, pt::to_time_t(pt::second_clock::local_time()));
        FileInfo::FileData file = fillFileData(file_path);
        m_model->appendData(file);
		m_model->sort(
			curr_path == curr_path.root_path()
			? 0
			: 1);
    }
}

bool ManagerWindow::copyFromTo(path from, path to)
{
    boost::system::error_code ec;
    fs::copy(from, to, ec);
    if(ec.value() != boost::system::errc::success) return false;
    if(!is_symlink(from) && is_directory(from))
        for(directory_entry entry
            : directory_iterator(from))
            copyFromTo(entry.path(), to / entry.path().filename());
    return true;
}
