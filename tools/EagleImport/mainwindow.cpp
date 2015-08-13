#include <QtCore>
#include <QtWidgets>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <librepcbcommon/fileio/smartxmlfile.h>
#include <librepcbcommon/fileio/xmldomdocument.h>
#include <librepcbcommon/fileio/xmldomelement.h>
#include <librepcblibrary/sym/symbol.h>
#include <librepcblibrary/fpt/footprint.h>
#include <librepcblibrary/pkg/package.h>
#include <librepcblibrary/cmp/component.h>
#include <librepcblibrary/gencmp/genericcomponent.h>
#include <librepcbcommon/boardlayer.h>
#include <librepcbcommon/schematiclayer.h>
#include "polygonsimplifier.h"

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
    ui->edtDuplicateFolders->setText(s.value("mainwindow/last_library_directory").toString());
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
    s.setValue("mainwindow/last_library_directory", ui->edtDuplicateFolders->text());
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
    QString allowedChars("_-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

    QString settingsKey = filepath.getFilename() % '_' % key1 % '_' % key2;
    settingsKey.replace("{", "");
    settingsKey.replace("}", "");
    settingsKey.replace(" ", "_");
    for (int i=0; i<settingsKey.length(); i++)
    {
        if (!allowedChars.contains(settingsKey[i]))
            settingsKey.replace(i, 1, QString("__U%1__").arg(QString::number(settingsKey[i].unicode(), 16).toUpper()));
    }
    settingsKey.prepend(cat % '/');

    QUuid uuid = QUuid::createUuid();
    QString value = outputSettings.value(settingsKey).toString();
    if (!value.isEmpty()) uuid = QUuid(value); //QUuid(QString("{%1}").arg(value));

    if (uuid.isNull())
    {
        addError("Invalid UUID in *.ini file: " % settingsKey, filepath);
        return QUuid::createUuid();
    }
    outputSettings.setValue(settingsKey, uuid.toString());
    return uuid;
}

QString MainWindow::createDescription(const FilePath& filepath, const QString& name)
{
    return QString("\n\nThis element was automatically imported from Eagle\n"
                   "Filepath: %1\nName: %2\n"
                   "NOTE: Remove this text after manual rework!")
            .arg(filepath.getFilename(), name);
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
        QSharedPointer<XmlDomDocument> doc = file.parseFileAndBuildDomTree(false);
        XmlDomElement* node = doc->getRoot().getFirstChild("drawing/library", true, true);

        switch (type)
        {
            case ConvertFileType_t::Symbols_to_Symbols:
                node = node->getFirstChild("symbols", true);
                break;
            case ConvertFileType_t::Packages_to_FootprintsAndComponents:
                node = node->getFirstChild("packages", true);
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
            else if (child->getName() == "package")
                success = convertPackage(outputSettings, filepath, child);
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
        QString name = node->getAttribute("name", true);
        QString desc = createDescription(filepath, name);
        QUuid uuid = getOrCreateUuid(outputSettings, filepath, "symbols", name);
        bool rotate180 = false;
        if (filepath.getFilename() == "con-lsta.lbr" && name.startsWith("FE")) rotate180 = true;
        if (filepath.getFilename() == "con-lstb.lbr" && name.startsWith("MA")) rotate180 = true;

        // create symbol
        Symbol* symbol = new Symbol(uuid, Version("0.1"), "LibrePCB", name, desc);

        for (XmlDomElement* child = node->getFirstChild(); child; child = child->getNextSibling())
        {
            if (child->getName() == "wire")
            {
                SymbolPolygon* polygon = new SymbolPolygon();
                switch (child->getAttribute<uint>("layer"))
                {
                    case 94: polygon->setLayerId(SchematicLayer::LayerID::SymbolOutlines); break;
                    case 95: polygon->setLayerId(SchematicLayer::LayerID::ComponentNames); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }
                polygon->setIsFilled(false);
                polygon->setWidth(child->getAttribute<Length>("width"));
                polygon->setIsGrabArea(true);
                Point startpos = Point(child->getAttribute<Length>("x1"), child->getAttribute<Length>("y1"));
                Point endpos = Point(child->getAttribute<Length>("x2"), child->getAttribute<Length>("y2"));
                Angle angle = child->hasAttribute("curve") ? child->getAttribute<Angle>("curve") : Angle(0);
                if (rotate180)
                {
                    startpos = Point(-startpos.getX(), -startpos.getY());
                    endpos = Point(-endpos.getX(), -endpos.getY());
                }
                polygon->setStartPos(startpos);
                polygon->appendSegment(new SymbolPolygonSegment(endpos, angle));
                symbol->addPolygon(polygon);
            }
            else if (child->getName() == "rectangle")
            {
                SymbolPolygon* polygon = new SymbolPolygon();
                switch (child->getAttribute<uint>("layer"))
                {
                    case 94: polygon->setLayerId(SchematicLayer::LayerID::SymbolOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }
                polygon->setIsFilled(true);
                if (child->hasAttribute("width"))
                    polygon->setWidth(child->getAttribute<Length>("width"));
                polygon->setIsGrabArea(true);
                polygon->setStartPos(Point(child->getAttribute<Length>("x1"), child->getAttribute<Length>("y1")));
                polygon->appendSegment(new SymbolPolygonSegment(Point(child->getAttribute<Length>("x2"), child->getAttribute<Length>("y1"))));
                polygon->appendSegment(new SymbolPolygonSegment(Point(child->getAttribute<Length>("x2"), child->getAttribute<Length>("y2"))));
                polygon->appendSegment(new SymbolPolygonSegment(Point(child->getAttribute<Length>("x1"), child->getAttribute<Length>("y2"))));
                polygon->appendSegment(new SymbolPolygonSegment(Point(child->getAttribute<Length>("x1"), child->getAttribute<Length>("y1"))));
                symbol->addPolygon(polygon);
            }
            else if (child->getName() == "polygon")
            {
                SymbolPolygon* polygon = new SymbolPolygon();
                switch (child->getAttribute<uint>("layer"))
                {
                    case 94: polygon->setLayerId(SchematicLayer::LayerID::SymbolOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }
                polygon->setIsFilled(false);
                if (child->hasAttribute("width"))
                    polygon->setWidth(child->getAttribute<Length>("width"));
                polygon->setIsGrabArea(true);
                for (XmlDomElement* vertex = child->getFirstChild(); vertex; vertex = vertex->getNextSibling())
                {
                    Point p(vertex->getAttribute<Length>("x"), vertex->getAttribute<Length>("y"));
                    if (vertex == child->getFirstChild())
                        polygon->setStartPos(p);
                    else
                        polygon->appendSegment(new SymbolPolygonSegment(p));
                }
                polygon->appendSegment(new SymbolPolygonSegment(polygon->getStartPos()));
                symbol->addPolygon(polygon);
            }
            else if (child->getName() == "circle")
            {
                Length radius(child->getAttribute<Length>("radius"));
                Point center(child->getAttribute<Length>("x"), child->getAttribute<Length>("y"));
                SymbolEllipse* ellipse = new SymbolEllipse();
                switch (child->getAttribute<uint>("layer"))
                {
                    case 94: ellipse->setLayerId(SchematicLayer::LayerID::SymbolOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }
                ellipse->setLineWidth(child->getAttribute<Length>("width"));
                ellipse->setIsFilled(ellipse->getLineWidth() == 0);
                ellipse->setIsGrabArea(true);
                ellipse->setCenter(center);
                ellipse->setRadiusX(radius);
                ellipse->setRadiusY(radius);
                symbol->addEllipse(ellipse);
            }
            else if (child->getName() == "text")
            {
                SymbolText* text = new SymbolText();
                switch (child->getAttribute<uint>("layer"))
                {
                    case 93: text->setLayerId(SchematicLayer::LayerID::SymbolPinNames); break;
                    case 94: text->setLayerId(SchematicLayer::LayerID::SymbolOutlines); break;
                    case 95: text->setLayerId(SchematicLayer::LayerID::ComponentNames); break;
                    case 96: text->setLayerId(SchematicLayer::LayerID::ComponentValues); break;
                    case 99: text->setLayerId(SchematicLayer::LayerID::OriginCrosses); break; // ???
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }
                QString textStr = child->getText(true);
                if (textStr == ">NAME")
                {
                    textStr = "${SYM::NAME}";
                    text->setHeight(Length::fromMm(3.175));
                }
                else if (textStr == ">VALUE")
                {
                    textStr = "${CMP::VALUE}";
                    text->setHeight(Length::fromMm(2.5));
                }
                else
                    text->setHeight(child->getAttribute<Length>("size")*2);
                text->setText(textStr);
                Point pos = Point(child->getAttribute<Length>("x"), child->getAttribute<Length>("y"));
                int angleDeg = 0;
                if (child->hasAttribute("rot")) angleDeg = child->getAttribute("rot").remove("R").toInt();
                if (rotate180)
                {
                    pos = Point(-pos.getX(), -pos.getY());
                    angleDeg += 180;
                }
                text->setPosition(pos);
                text->setRotation(Angle::fromDeg(angleDeg));
                text->setAlign(Alignment(HAlign::left(), VAlign::bottom()));
                symbol->addText(text);
            }
            else if (child->getName() == "pin")
            {
                QUuid pinUuid = getOrCreateUuid(outputSettings, filepath, "symbol_pins", uuid.toString(), child->getAttribute("name"));
                SymbolPin* pin = new SymbolPin(pinUuid, child->getAttribute("name"));
                Point pos = Point(child->getAttribute<Length>("x"), child->getAttribute<Length>("y"));
                Length len(7620000);
                if (child->hasAttribute("length"))
                {
                    if (child->getAttribute("length") == "point")
                        len.setLengthNm(0);
                    else if (child->getAttribute("length") == "short")
                        len.setLengthNm(2540000);
                    else if (child->getAttribute("length") == "middle")
                        len.setLengthNm(5080000);
                    else if (child->getAttribute("length") == "long")
                        len.setLengthNm(7620000);
                    else
                        throw Exception(__FILE__, __LINE__, "Invalid symbol pin length: " % child->getAttribute("length"));
                }
                int angleDeg = 0;
                if (child->hasAttribute("rot")) angleDeg = child->getAttribute("rot").remove("R").toInt();
                if (rotate180)
                {
                    pos = Point(-pos.getX(), -pos.getY());
                    angleDeg += 180;
                }
                pin->setPosition(pos);
                pin->setLength(len);
                pin->setRotation(Angle::fromDeg(angleDeg));
                symbol->addPin(pin);
            }
            else
            {
                addError(QString("Unknown node name: %1/%2").arg(node->getName()).arg(child->getName()), filepath);
                return false;
            }
        }

        // convert line rects to polygon rects
        PolygonSimplifier<Symbol, SymbolPolygon, SymbolPolygonSegment> polygonSimplifier(*symbol);
        polygonSimplifier.convertLineRectsToPolygonRects(false, true);

        // save symbol to file
        symbol->saveTo(FilePath(QString("%1/sym").arg(ui->output->text())));
        delete symbol;
    }
    catch (Exception& e)
    {
        addError(e.getUserMsg() % " [" % e.getDebugMsg() % "]");
        return false;
    }

    return true;
}

bool MainWindow::convertPackage(QSettings& outputSettings, const FilePath& filepath, XmlDomElement* node)
{
    try
    {
        QString name = node->getAttribute("name", true);
        QUuid uuid = getOrCreateUuid(outputSettings, filepath, "packages_to_footprints", name);
        QString desc = node->getFirstChild("description", false) ? node->getFirstChild("description", true)->getText() : "";
        desc.append(createDescription(filepath, name));
        bool rotate180 = false;
        //if (filepath.getFilename() == "con-lsta.lbr" && name.startsWith("FE")) rotate180 = true;
        //if (filepath.getFilename() == "con-lstb.lbr" && name.startsWith("MA")) rotate180 = true;

        // create footprint
        Footprint* footprint = new Footprint(uuid, Version("0.1"), "LibrePCB", name, desc);

        for (XmlDomElement* child = node->getFirstChild(); child; child = child->getNextSibling())
        {
            if (child->getName() == "description")
            {
                // nothing to do
            }
            else if (child->getName() == "wire")
            {
                FootprintPolygon* polygon = new FootprintPolygon();
                switch (child->getAttribute<uint>("layer"))
                {
                    case 21: polygon->setLayerId(BoardLayer::LayerID::TopOverlay); break;
                    case 25: polygon->setLayerId(BoardLayer::LayerID::TopOverlayNames); break;
                    case 39: polygon->setLayerId(BoardLayer::LayerID::TopKeepout); break;
                    case 46: polygon->setLayerId(BoardLayer::LayerID::BoardOutline); break; // milling
                    case 48: polygon->setLayerId(BoardLayer::LayerID::TopDeviceOutlines); break; // document
                    case 49: polygon->setLayerId(BoardLayer::LayerID::OriginCrosses); break; // reference
                    case 51: polygon->setLayerId(BoardLayer::LayerID::TopDeviceOutlines); break;
                    case 52: polygon->setLayerId(BoardLayer::LayerID::BottomDeviceOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }
                polygon->setIsFilled(false);
                polygon->setWidth(child->getAttribute<Length>("width"));
                polygon->setIsGrabArea(true);
                Point startpos = Point(child->getAttribute<Length>("x1"), child->getAttribute<Length>("y1"));
                Point endpos = Point(child->getAttribute<Length>("x2"), child->getAttribute<Length>("y2"));
                Angle angle = child->hasAttribute("curve") ? child->getAttribute<Angle>("curve") : Angle(0);
                if (rotate180)
                {
                    startpos = Point(-startpos.getX(), -startpos.getY());
                    endpos = Point(-endpos.getX(), -endpos.getY());
                }
                polygon->setStartPos(startpos);
                polygon->appendSegment(new FootprintPolygonSegment(endpos, angle));
                footprint->addPolygon(polygon);
            }
            else if (child->getName() == "rectangle")
            {
                bool valid = true;
                FootprintPolygon* polygon = new FootprintPolygon();
                switch (child->getAttribute<uint>("layer"))
                {
                    case 1: polygon->setLayerId(BoardLayer::LayerID::TopCopper); break;
                    case 16: polygon->setLayerId(BoardLayer::LayerID::BottomCopper); break;
                    case 21: polygon->setLayerId(BoardLayer::LayerID::TopOverlay); break;
                    case 29: polygon->setLayerId(BoardLayer::LayerID::TopStopMask); break;
                    case 31: polygon->setLayerId(BoardLayer::LayerID::TopPaste); break;
                    case 35: polygon->setLayerId(BoardLayer::LayerID::TopGlue); break;
                    case 41: valid = false; break; // tRestrict
                    case 42: valid = false; break; // bRestrict
                    case 43: valid = false; break; // vRestrict
                    case 51: polygon->setLayerId(BoardLayer::LayerID::TopDeviceOutlines); break;
                    case 52: polygon->setLayerId(BoardLayer::LayerID::BottomDeviceOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }
                polygon->setIsFilled(true);
                if (child->hasAttribute("width"))
                    polygon->setWidth(child->getAttribute<Length>("width"));
                polygon->setIsGrabArea(true);
                polygon->setStartPos(Point(child->getAttribute<Length>("x1"), child->getAttribute<Length>("y1")));
                polygon->appendSegment(new FootprintPolygonSegment(Point(child->getAttribute<Length>("x2"), child->getAttribute<Length>("y1"))));
                polygon->appendSegment(new FootprintPolygonSegment(Point(child->getAttribute<Length>("x2"), child->getAttribute<Length>("y2"))));
                polygon->appendSegment(new FootprintPolygonSegment(Point(child->getAttribute<Length>("x1"), child->getAttribute<Length>("y2"))));
                polygon->appendSegment(new FootprintPolygonSegment(Point(child->getAttribute<Length>("x1"), child->getAttribute<Length>("y1"))));
                if (valid)
                    footprint->addPolygon(polygon);
                else
                    delete polygon;
            }
            else if (child->getName() == "polygon")
            {
                FootprintPolygon* polygon = new FootprintPolygon();
                switch (child->getAttribute<uint>("layer"))
                {
                    case 1: polygon->setLayerId(BoardLayer::LayerID::TopCopper); break;
                    case 16: polygon->setLayerId(BoardLayer::LayerID::BottomCopper); break;
                    case 21: polygon->setLayerId(BoardLayer::LayerID::TopOverlay); break;
                    case 29: polygon->setLayerId(BoardLayer::LayerID::TopStopMask); break;
                    case 31: polygon->setLayerId(BoardLayer::LayerID::TopPaste); break;
                    case 51: polygon->setLayerId(BoardLayer::LayerID::TopDeviceOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }
                polygon->setIsFilled(false);
                if (child->hasAttribute("width"))
                    polygon->setWidth(child->getAttribute<Length>("width"));
                polygon->setIsGrabArea(true);
                for (XmlDomElement* vertex = child->getFirstChild(); vertex; vertex = vertex->getNextSibling())
                {
                    Point p(vertex->getAttribute<Length>("x"), vertex->getAttribute<Length>("y"));
                    if (vertex == child->getFirstChild())
                        polygon->setStartPos(p);
                    else
                        polygon->appendSegment(new FootprintPolygonSegment(p));
                }
                polygon->appendSegment(new FootprintPolygonSegment(polygon->getStartPos()));
                footprint->addPolygon(polygon);
            }
            else if (child->getName() == "circle")
            {
                bool valid = true;
                Length radius(child->getAttribute<Length>("radius"));
                Point center(child->getAttribute<Length>("x"), child->getAttribute<Length>("y"));
                FootprintEllipse* ellipse = new FootprintEllipse();
                switch (child->getAttribute<uint>("layer"))
                {
                    case 1: ellipse->setLayerId(BoardLayer::LayerID::TopCopper); break;
                    case 20: ellipse->setLayerId(BoardLayer::LayerID::BoardOutline); break;
                    case 21: ellipse->setLayerId(BoardLayer::LayerID::TopOverlay); break;
                    case 22: ellipse->setLayerId(BoardLayer::LayerID::BottomDeviceOutlines); break;
                    case 27: ellipse->setLayerId(BoardLayer::LayerID::TopOverlayValues); break;
                    case 29: ellipse->setLayerId(BoardLayer::LayerID::TopStopMask); break;
                    case 31: ellipse->setLayerId(BoardLayer::LayerID::TopPaste); break;
                    case 41: valid = false; break; // tRestrict
                    case 42: valid = false; break; // bRestrict
                    case 43: valid = false; break; // vRestrict
                    case 49: ellipse->setLayerId(BoardLayer::LayerID::OriginCrosses); break; // reference
                    case 51: ellipse->setLayerId(BoardLayer::LayerID::TopDeviceOutlines); break;
                    case 52: ellipse->setLayerId(BoardLayer::LayerID::BottomDeviceOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }
                ellipse->setLineWidth(child->getAttribute<Length>("width"));
                ellipse->setIsFilled(ellipse->getLineWidth() == 0);
                ellipse->setIsGrabArea(true);
                ellipse->setCenter(center);
                ellipse->setRadiusX(radius);
                ellipse->setRadiusY(radius);
                if (valid)
                    footprint->addEllipse(ellipse);
                else
                    delete ellipse;
            }
            else if (child->getName() == "text")
            {
                FootprintText* text = new FootprintText();
                switch (child->getAttribute<uint>("layer"))
                {
                    case 21: text->setLayerId(BoardLayer::LayerID::TopOverlay); break;
                    case 25: text->setLayerId(BoardLayer::LayerID::TopOverlayNames); break;
                    case 27: text->setLayerId(BoardLayer::LayerID::TopOverlayValues); break;
                    case 48: text->setLayerId(BoardLayer::LayerID::TopDeviceOutlines); break; // document
                    case 51: text->setLayerId(BoardLayer::LayerID::TopDeviceOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }
                QString textStr = child->getText(true);
                if (textStr == ">NAME")
                {
                    textStr = "${CMP::NAME}";
                    text->setHeight(Length::fromMm(2.5));
                }
                else if (textStr == ">VALUE")
                {
                    textStr = "${CMP::VALUE}";
                    text->setHeight(Length::fromMm(2));
                }
                else
                    text->setHeight(child->getAttribute<Length>("size")*2);
                text->setText(textStr);
                Point pos = Point(child->getAttribute<Length>("x"), child->getAttribute<Length>("y"));
                int angleDeg = 0;
                if (child->hasAttribute("rot")) angleDeg = child->getAttribute("rot").remove("R").toInt();
                if (rotate180)
                {
                    pos = Point(-pos.getX(), -pos.getY());
                    angleDeg += 180;
                }
                text->setPosition(pos);
                text->setRotation(Angle::fromDeg(angleDeg));
                text->setAlign(Alignment(HAlign::left(), VAlign::bottom()));
                footprint->addText(text);
            }
            else if (child->getName() == "pad")
            {
                QUuid padUuid = getOrCreateUuid(outputSettings, filepath, "footprint_pads", uuid.toString(), child->getAttribute("name"));
                FootprintPad* pad = new FootprintPad(padUuid, child->getAttribute("name"));
                Length drill = child->getAttribute<Length>("drill");
                pad->setDrillDiameter(drill);
                Length diameter = child->hasAttribute("diameter") ? child->getAttribute<Length>("diameter") : drill * 2;
                int angleDeg = 0;
                if (child->hasAttribute("rot")) angleDeg = child->getAttribute("rot").remove("R").toInt();
                QString shape = child->hasAttribute("shape") ? child->getAttribute("shape") : "round";
                if (shape == "square")
                {
                    pad->setType(FootprintPad::Type_t::ThtRect);
                    pad->setWidth(diameter);
                    pad->setHeight(diameter);
                }
                else if (shape == "octagon")
                {
                    pad->setType(FootprintPad::Type_t::ThtOctagon);
                    pad->setWidth(diameter);
                    pad->setHeight(diameter);
                }
                else if (shape == "round")
                {
                    pad->setType(FootprintPad::Type_t::ThtRound);
                    pad->setWidth(diameter);
                    pad->setHeight(diameter);
                }
                else if (shape == "long")
                {
                    pad->setType(FootprintPad::Type_t::ThtRound);
                    pad->setWidth(diameter * 2);
                    pad->setHeight(diameter);
                }
                else
                {
                    throw Exception(__FILE__, __LINE__, "Invalid shape: " % shape % " :: " % filepath.toStr());
                }
                Point pos = Point(child->getAttribute<Length>("x"), child->getAttribute<Length>("y"));
                if (rotate180)
                {
                    pos = Point(-pos.getX(), -pos.getY());
                    angleDeg += 180;
                }
                pad->setPosition(pos);
                pad->setRotation(Angle::fromDeg(angleDeg));
                footprint->addPad(pad);
            }
            else if (child->getName() == "smd")
            {
                QUuid padUuid = getOrCreateUuid(outputSettings, filepath, "footprint_pads", uuid.toString(), child->getAttribute("name"));
                FootprintPad* pad = new FootprintPad(padUuid, child->getAttribute("name"));
                pad->setType(FootprintPad::Type_t::SmdRect);
                Point pos = Point(child->getAttribute<Length>("x"), child->getAttribute<Length>("y"));
                int angleDeg = 0;
                if (child->hasAttribute("rot")) angleDeg = child->getAttribute("rot").remove("R").toInt();
                if (rotate180)
                {
                    pos = Point(-pos.getX(), -pos.getY());
                    angleDeg += 180;
                }
                pad->setPosition(pos);
                pad->setRotation(Angle::fromDeg(angleDeg));
                pad->setWidth(child->getAttribute<Length>("dx"));
                pad->setHeight(child->getAttribute<Length>("dy"));
                switch (child->getAttribute<uint>("layer"))
                {
                    case 1: pad->setLayerId(BoardLayer::LayerID::TopCopper); break;
                    case 16: pad->setLayerId(BoardLayer::LayerID::BottomCopper); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute("layer"));
                }

                footprint->addPad(pad);
            }
            else if (child->getName() == "hole")
            {
                Footprint::FootprintHole_t* hole = new Footprint::FootprintHole_t();
                hole->pos = Point(child->getAttribute<Length>("x"), child->getAttribute<Length>("y"));
                hole->diameter = child->getAttribute<Length>("drill");
                footprint->addHole(hole);
            }
            else
            {
                addError(QString("Unknown node name: %1/%2").arg(node->getName()).arg(child->getName()), filepath);
                return false;
            }
        }

        // convert line rects to polygon rects
        PolygonSimplifier<Footprint, FootprintPolygon, FootprintPolygonSegment> polygonSimplifier(*footprint);
        polygonSimplifier.convertLineRectsToPolygonRects(false, true);

        // save footprint to file
        footprint->saveTo(FilePath(QString("%1/fpt").arg(ui->output->text())));

        // create package
        QUuid pkgUuid = getOrCreateUuid(outputSettings, filepath, "packages_to_packages", name);
        Package* package = new Package(pkgUuid, Version("0.1"), "LibrePCB", name, desc);
        package->setFootprintUuid(footprint->getUuid());
        package->saveTo(FilePath(QString("%1/pkg").arg(ui->output->text())));

        // clean up
        delete package;
        delete footprint;
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
        QString name = node->getAttribute("name", true);
        QUuid uuid = getOrCreateUuid(outputSettings, filepath, "devices_to_genericcomponents", name);
        QString desc = node->getFirstChild("description", false) ? node->getFirstChild("description", true)->getText() : "";
        desc.append(createDescription(filepath, name));

        // create generic component
        GenericComponent* gencomp = new GenericComponent(uuid, Version("0.1"), "LibrePCB", name, desc);

        // properties
        gencomp->addDefaultValue("en_US", "");
        gencomp->addPrefix("", node->hasAttribute("prefix") ? node->getAttribute("prefix") : "", true);

        // symbol variant
        QUuid symbVarUuid = getOrCreateUuid(outputSettings, filepath, "gencomp_symbolvariants", uuid.toString());
        GenCompSymbVar* symbvar = new GenCompSymbVar(symbVarUuid, QString(), true);
        symbvar->setName("en_US", "default");
        symbvar->setDescription("en_US", "");
        gencomp->addSymbolVariant(*symbvar);

        // signals
        XmlDomElement* device = node->getFirstChild("devices/device", true, true);
        for (XmlDomElement* connect = device->getFirstChild("connects/connect", false, false);
             connect; connect = connect->getNextSibling())
        {
            QString gateName = connect->getAttribute("gate");
            QString pinName = connect->getAttribute("pin");
            if (pinName.contains("@")) pinName.truncate(pinName.indexOf("@"));
            if (pinName.contains("#")) pinName.truncate(pinName.indexOf("#"));
            QUuid signalUuid = getOrCreateUuid(outputSettings, filepath, "gatepins_to_gencompsignals", uuid.toString(), gateName % pinName);

            if (!gencomp->getSignalByUuid(signalUuid))
            {
                // create signal
                GenCompSignal* signal = new GenCompSignal(signalUuid, pinName);
                gencomp->addSignal(*signal);
            }
        }

        // symbol variant items
        for (XmlDomElement* gate = node->getFirstChild("gates/*", true, true); gate; gate = gate->getNextSibling())
        {
            QString gateName = gate->getAttribute("name");
            QString symbolName = gate->getAttribute("symbol");
            QUuid symbolUuid = getOrCreateUuid(outputSettings, filepath, "symbols", symbolName);

            // create symbol variant item
            QUuid symbVarItemUuid = getOrCreateUuid(outputSettings, filepath, "symbolgates_to_symbvaritems", uuid.toString(), gateName);
            GenCompSymbVarItem* item = new GenCompSymbVarItem(symbVarItemUuid, symbolUuid, true, (gateName == "G$1") ? "" : gateName);

            // connect pins
            for (XmlDomElement* connect = device->getFirstChild("connects/connect", false, false);
                 connect; connect = connect->getNextSibling())
            {
                if (connect->getAttribute("gate") == gateName)
                {
                    QString pinName = connect->getAttribute("pin");
                    QUuid pinUuid = getOrCreateUuid(outputSettings, filepath, "symbol_pins", symbolUuid.toString(), pinName);
                    if (pinName.contains("@")) pinName.truncate(pinName.indexOf("@"));
                    if (pinName.contains("#")) pinName.truncate(pinName.indexOf("#"));
                    QUuid signalUuid = getOrCreateUuid(outputSettings, filepath, "gatepins_to_gencompsignals", uuid.toString(), gateName % pinName);
                    item->addPinSignalMapping(pinUuid, signalUuid, GenCompSymbVarItem::PinDisplayType_t::GenCompSignal);
                }
            }

            symbvar->addItem(*item);
        }

        // create components
        for (XmlDomElement* device = node->getFirstChild("devices/*", true, true); device; device = device->getNextSibling())
        {
            if (!device->hasAttribute("package")) continue;

            QString deviceName = device->getAttribute("name");
            QString packageName = device->getAttribute("package");
            QUuid pkgUuid = getOrCreateUuid(outputSettings, filepath, "packages_to_packages", packageName);
            QUuid fptUuid = getOrCreateUuid(outputSettings, filepath, "packages_to_footprints", packageName);

            QUuid compUuid = getOrCreateUuid(outputSettings, filepath, "devices_to_components", name, deviceName);
            QString compName = deviceName.isEmpty() ? name : QString("%1_%2").arg(name, deviceName);
            Component* component = new Component(compUuid, Version("0.1"), "LibrePCB", compName, desc);
            component->setGenCompUuid(gencomp->getUuid());
            component->setPackageUuid(pkgUuid);

            // connect pads
            for (XmlDomElement* connect = device->getFirstChild("connects/*", false, false);
                 connect; connect = connect->getNextSibling())
            {
                QString gateName = connect->getAttribute("gate");
                QString pinName = connect->getAttribute("pin");
                QString padNames = connect->getAttribute("pad");
                if (pinName.contains("@")) pinName.truncate(pinName.indexOf("@"));
                if (pinName.contains("#")) pinName.truncate(pinName.indexOf("#"));
                if (connect->hasAttribute("route"))
                {
                    if (connect->getAttribute("route") != "any")
                        addError(QString("Unknown connect route: %1/%2").arg(node->getName()).arg(connect->getAttribute("route")), filepath);
                }
                foreach (const QString& padName, padNames.split(" ", QString::SkipEmptyParts))
                {
                    QUuid padUuid = getOrCreateUuid(outputSettings, filepath, "footprint_pads", fptUuid.toString(), padName);
                    QUuid signalUuid = getOrCreateUuid(outputSettings, filepath, "gatepins_to_gencompsignals", uuid.toString(), gateName % pinName);
                    component->addPadSignalMapping(padUuid, signalUuid);
                }
            }

            // save component
            component->saveTo(FilePath(QString("%1/cmp").arg(ui->output->text())));
            delete component;
        }

        // save generic component to file
        gencomp->saveTo(FilePath(QString("%1/gencmp").arg(ui->output->text())));
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

void MainWindow::on_pushButton_2_clicked()
{
    convertAllFiles(ConvertFileType_t::Packages_to_FootprintsAndComponents);
}

void MainWindow::on_btnPathsFromIni_clicked()
{
    FilePath inputDir(QFileDialog::getExistingDirectory(this, "Select Input Folder", mlastInputDirectory));
    if (!inputDir.isExistingDir()) return;

    QSettings outputSettings(UUID_LIST_FILEPATH, QSettings::IniFormat);

    foreach (QString key, outputSettings.allKeys())
    {
        key.remove(0, key.indexOf("/")+1);
        key.remove(key.indexOf(".lbr")+4, key.length() - key.indexOf(".lbr") - 4);
        QString filepath = inputDir.getPathTo(key).toNative();

        bool exists = false;
        for (int i = 0; i < ui->input->count(); i++)
        {
            FilePath fp(ui->input->item(i)->text());
            if (fp.toNative() == filepath) exists = true;
        }
        if (!exists) ui->input->addItem(filepath);
    }
}

void MainWindow::on_toolButton_clicked()
{
    ui->input->clear();
}

void MainWindow::on_toolButton_2_clicked()
{
    qDeleteAll(ui->input->selectedItems());
}

void MainWindow::on_toolButton_3_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Library Folder", ui->edtDuplicateFolders->text());
    if (dir.isEmpty()) return;
    ui->edtDuplicateFolders->setText(dir);
}

void MainWindow::on_toolButton_4_clicked()
{
    if (ui->edtDuplicateFolders->text().isEmpty()) return;
    if (ui->output->text().isEmpty()) return;

    QDir libDir(ui->edtDuplicateFolders->text());
    QDir outDir(ui->output->text());

    uint count = 0;

    foreach (const QString& subdir1, libDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        if (subdir1 == "EagleImport") continue;
        if (subdir1 == "Staging_Area") continue;
        QDir repoDir = libDir.absoluteFilePath(subdir1);
        foreach (const QString& subdir2, repoDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        {
            QDir repoSubDir = repoDir.absoluteFilePath(subdir2);
            foreach (const QString& elementDir, repoSubDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
            {
                QUuid elementUuid(elementDir);
                if (elementUuid.isNull()) continue;

                QDir outDirElement = outDir.absoluteFilePath(subdir2 % "/" % elementDir);
                if (outDirElement.exists())
                {
                    qDebug() << outDirElement.absolutePath();
                    if (outDirElement.removeRecursively())
                        count++;
                    else
                        qDebug() << "Failed!";
                }
            }
        }
    }

    QMessageBox::information(this, "Duplicates Removed", QString("%1 duplicates removed.").arg(count));
}
