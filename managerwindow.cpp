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
	//    m_view->horizontalHeader()->setCascadingSectionResizes(true);
	m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	//    m_view->horizontalHeader()->setStretchLastSection(true);
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
			path file_path = entry.path();
			FileInfo::FileData file = fillFileData(file_path);
			new_files.append(file);
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
				GetJunctionDestination(_path.c_str(), dest);
				path sym_path(dest);
				if (!sym_path.empty() && exists(sym_path))
					readThePath(sym_path);
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
		if (ask) res = QMessageBox::information(this,
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
			if (ec.value() == boost::system::errc::success)
				m_model->remove(index);
			else
				utils::showErrorCode(path, ec);
		}
		return;
	}

	if (is_directory(path))
	{
		if (ask) res = QMessageBox::information(this,
			"Deleting",
			"Delete the selected directory " +
			QString::fromStdWString(path.filename().wstring()) + " ?",
			QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
		else ask = QMessageBox::Yes;

		if (res == QMessageBox::Yes)
		{
			fs::remove_all(path, ec);
			if (ec.value() == boost::system::errc::success)
				m_model->remove(index);
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

		std::wofstream wofs(file_path.wstring());
		if (!wofs.is_open())
			QMessageBox::information(this, "Error", "File is not created.");
		wofs.close();
		last_write_time(file_path, pt::to_time_t(pt::second_clock::local_time()));
		m_model->appendData(fillFileData(file_path));
		m_model->sort(
			curr_path == curr_path.root_path()
			? 0
			: 1);
	}
}

//SLOTS
void ManagerWindow::disk_button_clicked_handler(const path &_path)
{
	this->curr_path = _path;
	fillWithData(_path);

	//CHANGE
	ui->curr_path_label->setText(QString::fromStdWString(this->curr_path.wstring()));
	ui->curr_path_label->repaint();
}

void ManagerWindow::mainTableView_dbClicked_handler(const QModelIndex &index)
{
	if (!index.isValid()) return;

	QModelIndex ind = ui->mainTableView->model()->index(index.row(), 0);

	path new_path(curr_path /
		ind.data(Qt::DisplayRole).toString().toStdWString());

	readThePath(new_path);

	//CHANGE
	ui->curr_path_label->setText(QString::fromStdWString(this->curr_path.wstring()));
	ui->curr_path_label->repaint();
}
//ADD RUNNING ON NEW THREAD
void ManagerWindow::cmd_returnPressed_handler()
{
#ifdef _WIN32 | _ WIN64
	ui->cmd->selectAll();
	QString cmd('\"');
	cmd += this->curr_path.root_name().string().c_str();
	cmd += " &&";
	cmd += " cd ";
	cmd += this->curr_path.string().c_str();
	if (!ui->cmd->text().isEmpty())
	{
		cmd += " && ";
		cmd += ui->cmd->text();
	}
	cmd += '\"';
	system(cmd.toLocal8Bit());
#endif
}

void ManagerWindow::slotCustomMenuRequested(QPoint pos)
{
	QMenu *menu = new QMenu(this);

	QAction *renameFile = new QAction("Rename", this);
	QAction *copyFile = new QAction("Copy", this);
	QAction *cutFile = new QAction("Cut", this);
	QAction *pasteFile = new QAction("Paste", this);
	QAction *deleteFile = new QAction("Delete", this);
	QAction *newSmth = new QAction("New", this);
	//New sub menu
	QMenu *newSubMenu = new QMenu(menu);
	QAction *createDir = new QAction("Folder", this);
	QAction *createTxtFile = new QAction("Text document (.txt)", this);
	QAction *createDocxFile = new QAction("Microsoft Word Document (.docx)", this);

	connect(renameFile, SIGNAL(triggered()),
		this, SLOT(slotRenameFile()));
	connect(copyFile, SIGNAL(triggered()),
		this, SLOT(slotCopyFile()));
	connect(cutFile, SIGNAL(triggered()),
		this, SLOT(slotCutFile()));
	connect(pasteFile, SIGNAL(triggered()),
		this, SLOT(slotPasteFile()));
	connect(deleteFile, SIGNAL(triggered()),
		this, SLOT(slotDeleteFile()));

	//New sub menu
	newSubMenu->addAction(createDir);
	newSubMenu->addAction(createTxtFile);
	newSubMenu->addAction(createDocxFile);

	connect(createDir, SIGNAL(triggered()),
		this, SLOT(slotCreateDir()));
	connect(createTxtFile, SIGNAL(triggered()),
		this, SLOT(slotCreateTxtFile()));
	connect(createDocxFile, SIGNAL(triggered()),
		this, SLOT(slotCreateDocxFile()));

	newSmth->setMenu(newSubMenu);

	menu->addAction(renameFile);
	menu->addAction(copyFile);
	menu->addAction(cutFile);
	menu->addAction(pasteFile);
	menu->addAction(deleteFile);
	menu->addSeparator();
	menu->addAction(newSmth);

	menu->popup(ui->mainTableView->viewport()->mapToGlobal(pos));
}

void ManagerWindow::slotRenameFile()
{
	QModelIndex index = ui->mainTableView->selectionModel()->currentIndex();
	if (!index.isValid()) return;
	QModelIndex ind = ui->mainTableView->model()->index(index.row(), 0);
	path old_path(curr_path /
		ind.data(Qt::DisplayRole).toString().toStdWString());

	bool bOK;
	QString new_name = QInputDialog::getText(
		this, "Renaming",
		"New name:", QLineEdit::Normal,
		QString::fromStdWString(old_path.filename().wstring()),
		&bOK);

	if (bOK)
	{
		path new_path(curr_path /
			new_name.toStdWString());
		boost::system::error_code ec;
		fs::rename(old_path, new_path, ec);
		if (ec.value() == boost::system::errc::success)
			m_model->setData(ind, QString::fromStdWString(new_path.filename().wstring()), Qt::EditRole);
		else
			utils::showErrorCode(old_path, ec);
	}
}

void ManagerWindow::slotCopyFile()
{
	QModelIndex index = ui->mainTableView->selectionModel()->currentIndex();
	if (!index.isValid()) return;
	QModelIndex ind = ui->mainTableView->model()->index(index.row(), 0);
	path path(curr_path /
		ind.data(Qt::DisplayRole).toString().toStdWString());
	fOps.from = path;
	fOps.op = COPY;
}

void ManagerWindow::slotCutFile()
{
	QModelIndex index = ui->mainTableView->selectionModel()->currentIndex();
	if (!index.isValid()) return;
	QModelIndex ind = ui->mainTableView->model()->index(index.row(), 0);
	path path(curr_path /
		ind.data(Qt::DisplayRole).toString().toStdWString());
	fOps.from = path;
	fOps.op = CUT;
	fOps.index = index;
}

void ManagerWindow::slotPasteFile()
{
	if (fOps.op == NULL || this->fOps.from.empty()) return;
	path to(curr_path / fOps.from.filename());

	boost::system::error_code ec;
	if (!exists(fOps.from, ec))
	{
		QMessageBox::information(this, "Ooops", "The file does not exist.");
		return;
	}

	if (ec.value() != boost::system::errc::success)
	{
		utils::showErrorCode(to, ec);
		return;
	}
	ec.clear();

	switch (fOps.op)
	{
	case COPY:
	{
		fs::copy(fOps.from, to, ec);
		if (ec.value() == boost::system::errc::success)
		{
			m_model->appendData(fillFileData(to));
			m_model->sort(
				curr_path == curr_path.root_path()
				? 0
				: 1);
		}
		else
			utils::showErrorCode(fOps.from, ec);
		break;
	}
	case CUT:
	{
		if (!fOps.index.isValid()) break;
		fs::copy(fOps.from, to, ec);
		if (ec.value() == boost::system::errc::success)
		{
			m_model->appendData(fillFileData(to));
			m_model->sort(
				curr_path == curr_path.root_path()
				? 0
				: 1);
			deleteFile(fOps.from, fOps.index, 0);
		}
		else
			utils::showErrorCode(fOps.from, ec);

		fOps.from = path();
		fOps.index = QModelIndex();
		fOps.op = NULL;

		break;
	}
	default:
		break;
	}
}

void ManagerWindow::slotDeleteFile()
{
	QModelIndex index = ui->mainTableView->selectionModel()->currentIndex();
	if (!index.isValid()) return;

	QModelIndex ind = ui->mainTableView->model()->index(index.row(), 0);
	path path(curr_path /
		ind.data(Qt::DisplayRole).toString().toStdWString());

	boost::system::error_code ec;

	if (!exists(path, ec))
		QMessageBox::information(this, "Ooops", "The file does not exist.");

	if (ec.value() != boost::system::errc::success)
	{
		utils::showErrorCode(path, ec);
		return;
	}

	deleteFile(path, index);
}

void ManagerWindow::slotCreateDir()
{
	bool bOK;
	QString new_name = QInputDialog::getText(
		this, "New folder",
		"Name:", QLineEdit::Normal,
		"New folder",
		&bOK);

	if (bOK)
	{
		path dir_path(curr_path /
			new_name.toStdWString());
		boost::system::error_code ec;
		fs::create_directory(dir_path, ec);
		if (ec.value() == boost::system::errc::success)
		{
			last_write_time(dir_path, pt::to_time_t(pt::second_clock::local_time()));
			m_model->appendData(fillFileData(dir_path));
			m_model->sort(
				curr_path == curr_path.root_path()
				? 0
				: 1);
		}
		else
			utils::showErrorCode(dir_path, ec);
	}
}

void ManagerWindow::slotCreateTxtFile()
{
	createFileWithExtension(".txt");
}

void ManagerWindow::slotCreateDocxFile()
{
	createFileWithExtension(".docx");
}
