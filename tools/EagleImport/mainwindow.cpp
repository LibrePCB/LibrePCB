#include <QtCore>
#include <QtWidgets>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <common/smartxmlfile.h>
#include <common/file_io/xmldomdocument.h>
#include <common/file_io/xmldomelement.h>
#include <library/symbol.h>
#include <library/genericcomponent.h>

using namespace library;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lblUuidList->setText(UUID_LIST_FILEPATH);

    QSettings s;
    restoreGeometry(s.value("mainwindow/geometry").toByteArray());
    restoreState(s.value("mainwindow/state").toByteArray());
    mlastInputDirectory = s.value("mainwindow/last_input_directory").toString();
    ui->input->addItems(s.value("mainwindow/input").toStringList());
    ui->output->setText(s.value("mainwindow/output").toString());

    reset();
}

MainWindow::~MainWindow()
{
    QStringList inputList;
    for (int i = 0; i < ui->input->count(); i++)
        inputList.append(ui->input->item(i)->text());

    QSettings s;
    s.setValue("mainwindow/geometry", saveGeometry());
    s.setValue("mainwindow/state", saveState());
    s.setValue("mainwindow/last_input_directory", mlastInputDirectory);
    s.setValue("mainwindow/input", QVariant::fromValue(inputList));
    s.setValue("mainwindow/output", ui->output->text());

    delete ui;
}

void MainWindow::reset()
{
    mAbortConversion = false;
    mReadedElementsCount = 0;
    mConvertedElementsCount = 0;

    ui->errors->clear();
    ui->pbarElements->setValue(0);
    ui->pbarElements->setMaximum(0);
    ui->pbarFiles->setValue(0);
    ui->pbarFiles->setMaximum(ui->input->count());
    ui->lblConvertedElements->setText("0 of 0");
}

void MainWindow::addError(const QString& msg, const FilePath& inputFile, int inputLine)
{
    ui->errors->addItem(QString("%1 (%2:%3)").arg(msg).arg(inputFile.toNative()).arg(inputLine));
}

QUuid MainWindow::getOrCreateUuid(QSettings& outputSettings, const FilePath& filepath,
                                  const QString& cat, const QString& key1, const QString& key2)
{
    QString settingsKey = cat % '/' % filepath.getFilename() % '_' % key1 % '_' % key2;
    QUuid uuid = outputSettings.value(settingsKey, QUuid::createUuid().toString()).toUuid();
    if (uuid.isNull())
    {
        addError("Invalid UUID in *.ini file: " % settingsKey, filepath);
        return QUuid::createUuid();
    }
    outputSettings.setValue(settingsKey, uuid.toString());
    return uuid;
}

void MainWindow::convertAllFiles(ConvertFileType_t type)
{
    reset();

    // create output directory
    FilePath outputDir(ui->output->text());
    outputDir.mkPath();

    QSettings outputSettings(UUID_LIST_FILEPATH, QSettings::IniFormat);

    for (int i = 0; i < ui->input->count(); i++)
    {
        FilePath filepath(ui->input->item(i)->text());
        if (!filepath.isExistingFile())
        {
            addError("File not found: " % filepath.toNative());
            continue;
        }

        convertFile(type, outputSettings, filepath);
        ui->pbarFiles->setValue(i + 1);

        if (mAbortConversion)
            break;
    }
}

void MainWindow::convertFile(ConvertFileType_t type, QSettings& outputSettings, const FilePath& filepath)
{
    try
    {
        // Check input file and read XML content
        SmartXmlFile file(filepath, false, true);
        QSharedPointer<XmlDomDocument> doc = file.parseFileAndBuildDomTree();
        XmlDomElement* node = doc->getRoot().getFirstChild("drawing/library", true, true);

        switch (type)
        {
            case ConvertFileType_t::Symbols_to_Symbols:
                node = node->getFirstChild("symbols", true);
                break;
            case ConvertFileType_t::Devices_to_GenericComponents:
                node = node->getFirstChild("devicesets", true);
                break;
            default:
                throw Exception(__FILE__, __LINE__);
        }

        ui->pbarElements->setValue(0);
        ui->pbarElements->setMaximum(node->getChildCount());

        // Convert Elements
        for (XmlDomElement* child = node->getFirstChild(); child; child = child->getNextSibling())
        {
            bool success;
            if (child->getName() == "symbol")
                success = convertSymbol(outputSettings, filepath, child);
            else if (child->getName() == "deviceset")
                success = convertDevice(outputSettings, filepath, child);
            else
                throw Exception(__FILE__, __LINE__, child->getName());

            mReadedElementsCount++;
            if (success) mConvertedElementsCount++;
            ui->pbarElements->setValue(ui->pbarElements->value() + 1);
            ui->lblConvertedElements->setText(QString("%1 of %2").arg(mConvertedElementsCount)
                                                                 .arg(mReadedElementsCount));
        }
    }
    catch (Exception& e)
    {
        addError(e.getUserMsg() % " [" % e.getDebugMsg() % "]");
        return;
    }
}

bool MainWindow::convertSymbol(QSettings& outputSettings, const FilePath& filepath, XmlDomElement* node)
{
    try
    {
        QUuid uuid = getOrCreateUuid(outputSettings, filepath, "symbols", node->getAttribute("name", true));
        QString name = node->getAttribute("name", true);

        // create symbol
        Symbol* symbol = new Symbol(uuid, Version("0.1"), "EDA4U", name);

        for (XmlDomElement* child = node->getFirstChild(); child; child = child->getNextSibling())
        {
            if (child->getName() == "wire")
            {
                SymbolPolygon* polygon = new SymbolPolygon();
                switch (child->getAttribute<uint>("layer"))
                {
                    case 94: polygon->setLineLayerId(10); break;
                    case 95: polygon->setLineLayerId(20); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }
                polygon->setFillLayerId(0);
                polygon->setLineWidth(child->getAttribute<Length>("width"));
                polygon->setIsGrabArea(true);
                Point startpos = Point(child->getAttribute<Length>("x1"), child->getAttribute<Length>("y1"));
                polygon->setStartPos(startpos);
                Point endpos = Point(child->getAttribute<Length>("x2"), child->getAttribute<Length>("y2"));
                Angle angle = child->hasAttribute("curve") ? Angle::fromDeg(child->getAttribute<int>("curve")) : Angle(0);
                if (angle == 0)
                {
                    polygon->appendSegment(new SymbolPolygonSegment(SymbolPolygonSegment::Type_t::Line, endpos));
                }
                else
                {
                    Length length = Point(endpos-startpos).getLength();
                    Length radius = length / Length::fromMm(2 * qSin(angle.toRad() / 2));

                    qreal angleSgn = angle >= 0 ? 1 : -1;
                    QVector2D diff_2((endpos.toMmQPointF()/2) - (startpos.toMmQPointF()/2));
                    QVector2D normale(angleSgn*diff_2.y(), (-1)*angleSgn*diff_2.x());
                    normale = normale.normalized() * radius.toMm() * qCos(angle.toRad() / 2);
                    Point center = startpos + Point::fromMm(diff_2.toPointF()) + Point::fromMm(normale.toPointF());

                    SymbolPolygonSegment* segment = new SymbolPolygonSegment(SymbolPolygonSegment::Type_t::Arc, endpos);
                    segment->setCenter(center);
                    polygon->appendSegment(segment);
                }
                symbol->addPolygon(polygon);
            }
            else if (child->getName() == "rectangle")
            {
                SymbolPolygon* polygon = new SymbolPolygon();
                switch (child->getAttribute<uint>("layer"))
                {
                    case 94: polygon->setLineLayerId(10); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }
                polygon->setFillLayerId(10);
                if (child->hasAttribute("width"))
                    polygon->setLineWidth(child->getAttribute<Length>("width"));
                polygon->setIsGrabArea(true);
                polygon->setStartPos(Point(child->getAttribute<Length>("x1"), child->getAttribute<Length>("y1")));
                polygon->appendSegment(new SymbolPolygonSegment(SymbolPolygonSegment::Type_t::Line,
                                       Point(child->getAttribute<Length>("x2"), child->getAttribute<Length>("y1"))));
                polygon->appendSegment(new SymbolPolygonSegment(SymbolPolygonSegment::Type_t::Line,
                                       Point(child->getAttribute<Length>("x2"), child->getAttribute<Length>("y2"))));
                polygon->appendSegment(new SymbolPolygonSegment(SymbolPolygonSegment::Type_t::Line,
                                       Point(child->getAttribute<Length>("x1"), child->getAttribute<Length>("y2"))));
                polygon->appendSegment(new SymbolPolygonSegment(SymbolPolygonSegment::Type_t::Line,
                                       Point(child->getAttribute<Length>("x1"), child->getAttribute<Length>("y1"))));
                symbol->addPolygon(polygon);
            }
            else if (child->getName() == "polygon")
            {
                SymbolPolygon* polygon = new SymbolPolygon();
                switch (child->getAttribute<uint>("layer"))
                {
                    case 94: polygon->setLineLayerId(10); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }
                polygon->setFillLayerId(0);
                if (child->hasAttribute("width"))
                    polygon->setLineWidth(child->getAttribute<Length>("width"));
                polygon->setIsGrabArea(true);
                for (XmlDomElement* vertex = child->getFirstChild(); vertex; vertex = vertex->getNextSibling())
                {
                    Point p(vertex->getAttribute<Length>("x"), vertex->getAttribute<Length>("y"));
                    if (vertex == child->getFirstChild())
                        polygon->setStartPos(p);
                    else
                        polygon->appendSegment(new SymbolPolygonSegment(SymbolPolygonSegment::Type_t::Line, p));
                }
                polygon->appendSegment(new SymbolPolygonSegment(SymbolPolygonSegment::Type_t::Line, polygon->getStartPos()));
                symbol->addPolygon(polygon);
            }
            else if (child->getName() == "circle")
            {
                SymbolPolygon* polygon = new SymbolPolygon();
                switch (child->getAttribute<uint>("layer"))
                {
                    case 94: polygon->setLineLayerId(10); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }
                polygon->setLineWidth(child->getAttribute<Length>("width"));
                polygon->setFillLayerId(polygon->getLineWidth() == 0 ? 10 : 0);
                polygon->setIsGrabArea(true);
                Length radius(child->getAttribute<Length>("radius"));
                Point center(child->getAttribute<Length>("x"), child->getAttribute<Length>("y"));
                Point startEndPos(center.getX(), center.getY() + radius);
                polygon->setStartPos(startEndPos);
                SymbolPolygonSegment* segment = new SymbolPolygonSegment(SymbolPolygonSegment::Type_t::Arc, startEndPos);
                segment->setCenter(center);
                polygon->appendSegment(segment);
                symbol->addPolygon(polygon);
            }
            else if (child->getName() == "text")
            {
                SymbolText* text = new SymbolText();
                switch (child->getAttribute<uint>("layer"))
                {
                    case 93: text->setLayerId(13); break;
                    case 94: text->setLayerId(10); break;
                    case 95: text->setLayerId(20); break;
                    case 96: text->setLayerId(21); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }
                QString textStr = child->getText(true);
                if (textStr == ">NAME") textStr = "${SYM::NAME}";
                if (textStr == ">VALUE") textStr = "${CMP::VALUE}";
                text->setText(textStr);
                text->setPosition(Point(child->getAttribute<Length>("x"),
                                        child->getAttribute<Length>("y")));
                text->setAngle(Angle(0));
                text->setHeight(child->getAttribute<Length>("size")*2);
                text->setAlign(Alignment(HAlign::left(), VAlign::bottom()));
                symbol->addText(text);
            }
            else if (child->getName() == "pin")
            {
                QUuid pinUuid = getOrCreateUuid(outputSettings, filepath, "pins", uuid.toString(), child->getAttribute("name"));
                SymbolPin* pin = new SymbolPin(pinUuid, child->getAttribute("name"));

                pin->setPosition(Point(child->getAttribute<Length>("x"),
                                       child->getAttribute<Length>("y")));

                if (!child->hasAttribute("length"))
                    pin->setLength(Length(7620000));
                else if (child->getAttribute("length") == "point")
                    pin->setLength(Length(0));
                else if (child->getAttribute("length") == "short")
                    pin->setLength(Length(2540000));
                else if (child->getAttribute("length") == "middle")
                    pin->setLength(Length(5080000));
                else if (child->getAttribute("length") == "long")
                    pin->setLength(Length(7620000));
                else
                    throw Exception(__FILE__, __LINE__, "Invalid symbol pin length: " % child->getAttribute("length"));

                int angleDeg = 0;
                if (child->hasAttribute("rot"))
                    angleDeg = -child->getAttribute("rot").remove("R").toInt();
                pin->setAngle(Angle::fromDeg(angleDeg) + Angle::deg90());

                symbol->addPin(pin);
            }
            else
            {
                addError(QString("Unknown node name: %1/%2").arg(node->getName()).arg(child->getName()), filepath);
                return false;
            }
        }

        symbol->convertLineRectsToPolygonRects(true, true);

        // save symbol to file
        symbol->saveToFile(FilePath(QString("%1/sym/%2/v%3.xml").arg(ui->output->text())
                                    .arg(uuid.toString()).arg(APP_VERSION_MAJOR)));
        delete symbol;
    }
    catch (Exception& e)
    {
        addError(e.getUserMsg() % " [" % e.getDebugMsg() % "]");
        return false;
    }

    return true;
}

bool MainWindow::convertDevice(QSettings& outputSettings, const FilePath& filepath, XmlDomElement* node)
{
    try
    {
        QUuid uuid = getOrCreateUuid(outputSettings, filepath, "devices", node->getAttribute("name", true));
        QString name = node->getAttribute("name", true);
        QString desc = node->getFirstChild("description", false) ? node->getFirstChild("description", true)->getText() : "";

        // create generic component
        GenericComponent* gencomp = new GenericComponent(uuid, Version("0.1"), "EDA4U", name, desc);

        // properties
        gencomp->addDefaultValue("en_US", "");
        gencomp->addPrefix("", node->hasAttribute("prefix") ? node->getAttribute("prefix") : "", true);

        // symbol variant
        GenCompSymbVar* symbvar = new GenCompSymbVar(QUuid::createUuid(), QString(), true);
        symbvar->setName("en_US", "default");
        symbvar->setDescription("en_US", "");
        gencomp->addSymbolVariant(symbvar);

        // signals
        XmlDomElement* connects = node->getFirstChild("devices/device/connects", true, true);
        for (XmlDomElement* connect = connects->getFirstChild("connect", false);
             connect; connect = connect->getNextSibling())
        {
            QString gateName = connect->getAttribute("gate");
            QString pinName = connect->getAttribute("pin");
            QUuid signalUuid = getOrCreateUuid(outputSettings, filepath, "signals", uuid.toString(), gateName % pinName);

            // create signal
            GenCompSignal* signal = new GenCompSignal(signalUuid, pinName);
            gencomp->addSignal(signal);
        }

        // symbol variant items
        int index = 0;
        for (XmlDomElement* gate = node->getFirstChild("gates/*", true, true); gate; gate = gate->getNextSibling())
        {
            QString gateName = gate->getAttribute("name");
            QString symbolName = gate->getAttribute("symbol");
            QUuid symbolUuid = getOrCreateUuid(outputSettings, filepath, "symbols", symbolName);

            // create symbol variant item
            QUuid symbVarItemUuid = getOrCreateUuid(outputSettings, filepath, "symbvaritem", uuid.toString(), gateName);
            GenCompSymbVarItem* item = new GenCompSymbVarItem(symbVarItemUuid, symbolUuid, index, true, gateName);

            // connect pins
            for (XmlDomElement* connect = connects->getFirstChild("connect", false);
                 connect; connect = connect->getNextSibling())
            {
                if (connect->getAttribute("gate") == gateName)
                {
                    QString pinName = connect->getAttribute("pin");
                    QUuid pinUuid = getOrCreateUuid(outputSettings, filepath, "pins", symbolUuid.toString(), pinName);
                    QUuid signalUuid = getOrCreateUuid(outputSettings, filepath, "signals", uuid.toString(), gateName % pinName);
                    item->addPinSignalMapping(pinUuid, signalUuid, GenCompSymbVarItem::PinDisplayType_t::GenCompSignal);
                }
            }

            symbvar->addItem(item);
            index++;
        }


        // save generic component to file
        gencomp->saveToFile(FilePath(QString("%1/gencmp/%2/v%3.xml").arg(ui->output->text())
                                     .arg(uuid.toString()).arg(APP_VERSION_MAJOR)));
        delete gencomp;
    }
    catch (Exception& e)
    {
        addError(e.getUserMsg() % " [" % e.getDebugMsg() % "]");
        return false;
    }

    return true;
}

void MainWindow::on_inputBtn_clicked()
{
    ui->input->clear();
    ui->input->addItems(QFileDialog::getOpenFileNames(this, "Select Eagle Library Files",
                                                    mlastInputDirectory, "*.lbr"));
    ui->pbarFiles->setMaximum(ui->input->count());

    if (ui->input->count() > 0)
        mlastInputDirectory = QFileInfo(ui->input->item(0)->text()).absolutePath();
}

void MainWindow::on_outputBtn_clicked()
{
    ui->output->setText(QFileDialog::getExistingDirectory(this, "Select Output Directory",
                                                          ui->output->text()));
}

void MainWindow::on_btnAbort_clicked()
{
    mAbortConversion = true;
}

void MainWindow::on_btnConvertSymbols_clicked()
{
    convertAllFiles(ConvertFileType_t::Symbols_to_Symbols);
}

void MainWindow::on_btnConvertDevices_clicked()
{
    convertAllFiles(ConvertFileType_t::Devices_to_GenericComponents);
}
