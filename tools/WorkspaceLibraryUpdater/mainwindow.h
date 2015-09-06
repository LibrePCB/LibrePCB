#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtWidgets>
#include <librepcbcommon/fileio/filepath.h>

namespace Ui {
class MainWindow;
}

class XmlDomElement;

class MainWindow : public QMainWindow
{
        Q_OBJECT

    public:

        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

    private slots:

        void on_addDirectoryBtn_clicked();
        void on_removeDirectoryBtn_clicked();
        void on_clrLibraryBtn_clicked();
        void on_updateBtn_clicked();

    private:

        // Attributes
        Ui::MainWindow *ui;
        QString lastDir;
};

#endif // MAINWINDOW_H
