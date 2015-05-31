#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtWidgets>
#include <eda4ucommon/fileio/filepath.h>

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

        void on_inputBtn_clicked();
        void on_outputBtn_clicked();
        void on_btnAbort_clicked();
        void on_btnConvertSymbols_clicked();
        void on_btnConvertDevices_clicked();

    private:

        enum class ConvertFileType_t {
            Symbols_to_Symbols,
            Devices_to_GenericComponents
        };

        void reset();
        void addError(const QString& msg, const FilePath& inputFile = FilePath(), int inputLine = 0);
        QUuid getOrCreateUuid(QSettings& outputSettings, const FilePath& filepath,
                              const QString& cat, const QString& key1, const QString& key2 = QString());
        void convertAllFiles(ConvertFileType_t type);
        void convertFile(ConvertFileType_t type, QSettings& outputSettings, const FilePath& filepath);
        bool convertSymbol(QSettings& outputSettings, const FilePath& filepath, XmlDomElement* node);
        bool convertDevice(QSettings& outputSettings, const FilePath& filepath, XmlDomElement* node);

        // Attributes
        Ui::MainWindow *ui;
        bool mAbortConversion;
        QString mlastInputDirectory;
        int mReadedElementsCount;
        int mConvertedElementsCount;
};

#endif // MAINWINDOW_H
