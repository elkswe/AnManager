#ifndef MANAGERWINDOW_H
#define MANAGERWINDOW_H

#include "ui_managerwindow.h"
#include <QMainWindow>
#include <QMessageBox>
#include <QString>
#include <QMenu>
#include <QInputDialog>
#include <FileInfo.h>
#include <DiskButton.h>
#include <Utils.h>

#ifdef _WIN32 | _WIN64
#include <ShortcutWork.h>
#include <JunctionPoint.h>
#include <Windows.h>
#include <shellapi.h>
#endif

namespace fs  = boost::filesystem;
using namespace neosmart;

struct FileOps{
	path from;
	int op;
	QModelIndex index;
};

//#define MDEBUG

namespace Ui {
class ManagerWindow;
}

class ManagerWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit ManagerWindow(QWidget *parent = 0);
	~ManagerWindow();

private:
	Ui::ManagerWindow *ui;

	//My data
	FileInfo* m_model;
	path curr_path;
	QVector<DiskButton*> sections;
	FileOps fOps;

	enum Operations {
		COPY = 1,
		CUT
	};

	//My methods
	void configureTableView();
	void mountSections();
	void addSection(const path _path, QString _name = NULL);
	bool fillWithData(const path & _path);
	FileInfo::FileData fillFileData(const path & _path);
	void readThePath(path & _path);
	void deleteFile(const path & path, const QModelIndex & index, int ask = 1);
	void createFileWithExtension(const char *ext);

private slots:
    void disk_button_clicked_handler(const path & path);
    void mainTableView_dbClicked_handler(const QModelIndex & index);
	void cmd_returnPressed_handler();
	//Context menu
	void slotCustomMenuRequested(QPoint pos);
	void slotRenameFile();
	void slotCopyFile();
	void slotCutFile();
	void slotPasteFile();
	void slotDeleteFile();
	void slotCreateDir();
	void slotCreateTxtFile();
	void slotCreateDocxFile();
};

#endif // MANAGERWINDOW_H
