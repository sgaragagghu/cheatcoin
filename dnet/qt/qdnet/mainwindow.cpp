#include <string.h>
#include <unistd.h>
#include <string>
#include <QDesktopWidget>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../../dnet_command.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	QDesktopWidget *d = QApplication::desktop();
	if (d->width() <= 480 || d->height() <= 480) {
		setWindowState(Qt::WindowMaximized);
	}
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_lineEdit_returnPressed()
{
	QString text = ui->lineEdit->text();
	std::string str = text.toStdString();
	char buf[DNET_COMMAND_MAX];
	struct dnet_output out;
	memset(&out, 0, sizeof(out));
	out.f = 0;
	out.str = buf;
	out.len = DNET_COMMAND_MAX;
	buf[0] = 0;
	dnet_command(str.c_str(), &out);
	str = buf;
	text = QString::fromStdString(str);
	ui->textBrowser->setText(text);
}
