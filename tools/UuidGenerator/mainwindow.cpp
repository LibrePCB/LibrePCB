#include <QtCore>
#include <QClipboard>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <librepcbcommon/uuid.h>

using namespace librepcb;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QSettings s;
    restoreGeometry(s.value("geometry").toByteArray());

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(on_timer_timeout()));

    //Uuid nullUuid = Uuid(0, 0, 16385, 128, 0, 0, 0, 0, 0, 0, 0);
}

MainWindow::~MainWindow()
{
    QSettings s;
    s.setValue("geometry", saveGeometry());
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    Uuid uuid = Uuid::createRandom();
    ui->lineEdit->setText(uuid.toStr());
    QApplication::clipboard()->setText(uuid.toStr());
}

void MainWindow::on_timer_timeout()
{
    if (QApplication::clipboard()->text() != ui->lineEdit->text())
    {
        ui->checkBox->setChecked(false);
        timer->stop();
        return;
    }

    on_pushButton_clicked();
}

void MainWindow::on_checkBox_toggled(bool checked)
{
    if (checked)
    {
        on_pushButton_clicked();
        timer->start(1000);
    }
    else
    {
        timer->stop();
    }
}
