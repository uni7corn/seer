#include "SeerStackDumpBrowserWidget.h"
#include "SeerUtl.h"
#include <QtGui/QFontDatabase>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QApplication>
#include <QtCore/QVector>
#include <QtCore/QDebug>

SeerStackDumpBrowserWidget::SeerStackDumpBrowserWidget (QWidget* parent) : QWidget(parent) {

    _spExpressionId   = Seer::createID();
    _dumpExpressionId = Seer::createID();

    // Construct the UI.
    setupUi(this);

    // Setup the widgets
    stackTableWidget->setMouseTracking(true);
    stackTableWidget->setSortingEnabled(false);
    stackTableWidget->resizeColumnToContents(0); // 2 byte address
    stackTableWidget->resizeColumnToContents(1); // 2 byte value
    stackTableWidget->resizeColumnToContents(2); // 4 byte value
    stackTableWidget->resizeColumnToContents(3); // 8 byte value
    stackTableWidget->resizeColumnToContents(4); // 8 byte ascii
    stackTableWidget->resizeRowsToContents();

    stackTableWidget->clearContents();

    // Connect things.
    QObject::connect(formatComboBox,       &QComboBox::currentTextChanged,    this,  &SeerStackDumpBrowserWidget::handleFormatComboBox);
    QObject::connect(visualizerToolButton, &QToolButton::clicked,             this,  &SeerStackDumpBrowserWidget::handleVisualizerToolButton);
}

SeerStackDumpBrowserWidget::~SeerStackDumpBrowserWidget () {
}

void SeerStackDumpBrowserWidget::handleText (const QString& text) {

    // -data-read-memory-bytes -o 16 $sp 32
    // ^done,memory=[
    //               {
    //                  begin="0x00007fffffffda80",
    //                  offset="0x0000000000000000",
    //                  end="0x00007fffffffdaa0",
    //                  contents="0000000000000000e80300000000000098dcffffff7f0000e803000001000000"
    //               }
    //             ]
    //
    //
    // -data-evaluate-expression $sp
    // ^done,value="0x7fffffffda90"


    QApplication::setOverrideCursor(Qt::BusyCursor);

    while (1) {
        if (text.contains(QRegularExpression("^([0-9]+)\\^done,value="))) {

            // ^done,value="0x7fffffffda90"
            QString id_text    = text.section('^', 0,0);
            QString value_text = Seer::parseFirst(text, "value=", '"', '"', false);

            if (id_text.toInt() != _spExpressionId) {
                break;
            }

            addressLineEdit->setText(value_text);

            emit refreshStackDump(_dumpExpressionId, value_text, 0, 64);

        }else if (text.contains(QRegularExpression("^([0-9]+)\\^done,memory="))) {

            QString id_text = text.section('^', 0,0);

            if (id_text.toInt() != _dumpExpressionId) {
                break;
            }

            QString begin_text    = Seer::parseFirst(text, "begin=",    '"', '"', false);
            QString contents_text = Seer::parseFirst(text, "contents=", '"', '"', false);

            _populateTable(begin_text, contents_text);

        }else if (text.contains(QRegularExpression("^([0-9]+)\\^error,msg=\"No registers.\""))) {

            addressLineEdit->setText("");

        }else if (text.startsWith("^error,msg=\"No registers.\"")) {
            stackTableWidget->clearContents();

        }else{
            // Ignore others.
        }
        break;
    }

    stackTableWidget->resizeColumnToContents(0);
    stackTableWidget->resizeColumnToContents(1);
    stackTableWidget->resizeColumnToContents(2);
    stackTableWidget->resizeColumnToContents(3);
    stackTableWidget->resizeColumnToContents(4);
    stackTableWidget->resizeRowsToContents();

    QApplication::restoreOverrideCursor();
}

void SeerStackDumpBrowserWidget::handleStoppingPointReached () {

    // Don't do any work if the widget is hidden.
    if (isHidden()) {
        return;
    }

    refresh();
}

void SeerStackDumpBrowserWidget::handleFormatComboBox (const QString& text) {

    Q_UNUSED(text);

    refresh();
}

void SeerStackDumpBrowserWidget::handleVisualizerToolButton () {

    emit addMemoryVisualize(addressLineEdit->text());
}

void SeerStackDumpBrowserWidget::refresh () {

    // Don't do any work if the widget is hidden.
    if (isHidden()) {
        return;
    }

    emit refreshStackPointer(_spExpressionId, "$sp");
}

void SeerStackDumpBrowserWidget::showEvent (QShowEvent* event) {

    QWidget::showEvent(event);

    refresh();
}

void SeerStackDumpBrowserWidget::_populateTable (QString address, QString contents) {

    QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    // Loop off '0x'.
    if (address.mid(0,2) == "0x") {
        address = address.mid(2);
    }

    bool ok;

    quint64 address64 = address.toULongLong(&ok, 16);
    quint64 pos64     = address64;
    int     nrows     = contents.length()/2/2;  // Divide by 2 for a "FF" character, then again for 2 bytes per row.

    // Convert the contents to raw bytes.
    QVector<quint8> bytes;
    for (int pos=0; pos<contents.length(); pos+=2) {

        // Get a hex value "FF".
        QString str = contents.mid(pos, 2);

        // Ignore "0x";
        if (str == "0x" || str == "0X") {
            continue;
        }

        // Convert string hex to quint8.
        quint8 hex = str.toUInt(&ok, 16);
        if (ok) {
            bytes.push_back(hex);
        }
    }

    // Resize table.
    stackTableWidget->clearContents();
    stackTableWidget->setRowCount(nrows);

    // Fill in the address column.
    for (int i=0,r=0; i < contents.length()/2; i+=2,pos64+=2,r++) {
        QString str = QString::number(pos64, 16);

        QTableWidgetItem* item = new QTableWidgetItem;
        item->setText(str);
        item->setFont(fixedFont);

        stackTableWidget->setItem(r,0,item);
    }

    // Fill in the 2byte column.
    for (int i=0,r=0; i<bytes.size(); i+=2,r++) {

        QString str;

        if (formatComboBox->currentText() == "hex") {
            str = Seer::ucharToHex(bytes, i, 2);
        }else if (formatComboBox->currentText() == "octal") {
            str = Seer::ucharToOctal(bytes, i, 2);
        }else if (formatComboBox->currentText() == "uint") {
            str = Seer::ucharToUShort(bytes, i, 1);
        }else if (formatComboBox->currentText() == "int") {
            str = Seer::ucharToShort(bytes, i, 1);
        }

        QTableWidgetItem* item = new QTableWidgetItem;
        item->setText(str);
        item->setFont(fixedFont);

        stackTableWidget->setItem(r,1,item);
    }

    // Fill in the 4byte column.
    for (int i=0,r=0; i < bytes.size(); i+=2,r++) {

        QString str;

        if (formatComboBox->currentText() == "hex") {
            str = Seer::ucharToHex(bytes, i, 4);
        }else if (formatComboBox->currentText() == "octal") {
            str = Seer::ucharToOctal(bytes, i, 4);
        }else if (formatComboBox->currentText() == "uint") {
            str = Seer::ucharToUInt(bytes, i, 1);
        }else if (formatComboBox->currentText() == "int") {
            str = Seer::ucharToInt(bytes, i, 1);
        }else if (formatComboBox->currentText() == "float") {
            str = Seer::ucharToFloat(bytes, i, 1);
        }

        QTableWidgetItem* item = new QTableWidgetItem;
        item->setText(str);
        item->setFont(fixedFont);

        stackTableWidget->setItem(r,2,item);
    }

    // Fill in the 8byte column.
    for (int i=0,r=0; i < bytes.size(); i+=2,r++) {

        QString str;

        if (formatComboBox->currentText() == "hex") {
            str = Seer::ucharToHex(bytes, i, 8);
        }else if (formatComboBox->currentText() == "octal") {
            str = Seer::ucharToOctal(bytes, i, 8);
        }else if (formatComboBox->currentText() == "uint") {
            str = Seer::ucharToULong(bytes, i, 1);
        }else if (formatComboBox->currentText() == "int") {
            str = Seer::ucharToLong(bytes, i, 1);
        }else if (formatComboBox->currentText() == "float") {
            str = Seer::ucharToDouble(bytes, i, 1);
        }

        QTableWidgetItem* item = new QTableWidgetItem;
        item->setText(str);
        item->setFont(fixedFont);

        stackTableWidget->setItem(r,3,item);
    }

    // Fill in the 8byte ascii column.
    for (int i=0,r=0; i < bytes.size(); i+=2,r++) {

        QString str = Seer::ucharToAscii(bytes, i, 8);

        QTableWidgetItem* item = new QTableWidgetItem;
        item->setText(str);
        item->setFont(fixedFont);

        stackTableWidget->setItem(r,4,item);
    }
}

