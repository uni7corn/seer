#include "SeerVariableTrackerBrowserWidget.h"
#include "SeerUtl.h"
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItemIterator>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtGui/QFontDatabase>
#include <QtGui/QClipboard>
#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <iostream>

SeerVariableTrackerBrowserWidget::SeerVariableTrackerBrowserWidget (QWidget* parent) : QWidget(parent) {

    // Construct the UI.
    setupUi(this);

    // Setup the widgets
    variablesTreeWidget->setMouseTracking(true);
    variablesTreeWidget->setSortingEnabled(false);
    variablesTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    variablesTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    variablesTreeWidget->resizeColumnToContents(0); // name
    variablesTreeWidget->resizeColumnToContents(1); // value
    variablesTreeWidget->resizeColumnToContents(2); // id
    variablesTreeWidget->resizeColumnToContents(3); // used
    variablesTreeWidget->setColumnHidden(2, true);  // Hide the 'id' column.
    variablesTreeWidget->setColumnHidden(3, true);  // Hide the 'used' column.
    variablesTreeWidget->clear();

    // Connect things.
    QObject::connect(variableAddLineEdit,            &QLineEdit::returnPressed,                     this, &SeerVariableTrackerBrowserWidget::handleAddLineEdit);
    QObject::connect(variableDeleteToolButton,       &QToolButton::clicked,                         this, &SeerVariableTrackerBrowserWidget::handleDeleteToolButton);
    QObject::connect(variableDeleteAllToolButton,    &QToolButton::clicked,                         this, &SeerVariableTrackerBrowserWidget::handleDeleteAllToolButton);
    QObject::connect(variablesTreeWidget,            &QTreeWidget::itemCollapsed,                   this, &SeerVariableTrackerBrowserWidget::handleItemCollapsed);
    QObject::connect(variablesTreeWidget,            &QTreeWidget::itemExpanded,                    this, &SeerVariableTrackerBrowserWidget::handleItemExpanded);
    QObject::connect(variablesTreeWidget,            &QTreeWidget::itemEntered,                     this, &SeerVariableTrackerBrowserWidget::handleItemEntered);
    QObject::connect(variablesTreeWidget,            &QTreeWidget::customContextMenuRequested,      this, &SeerVariableTrackerBrowserWidget::handleContextMenu);
}

SeerVariableTrackerBrowserWidget::~SeerVariableTrackerBrowserWidget () {
}

void SeerVariableTrackerBrowserWidget::handleText (const QString& text) {

    QApplication::setOverrideCursor(Qt::BusyCursor);

    while (1) {
        if (text.startsWith("^done,DataExpressionTable={") && text.endsWith("}")) {

            // "^done,DataExpressionTable={
            //          entry={id=\"1\",expression=\"s\"},
            //          entry={id=\"2\",expression=\"v\"},
            //          entry={id=\"4\",expression=\"l\"},
            //          entry={id=\"5\",expression=\"m\"}
            //      }"

            QString frame_text = Seer::parseFirst(text, "DataExpressionTable=", '{', '}', false);

            QStringList entries_list = Seer::parse(frame_text, "entry=", '{', '}', false);

            for (int i=0; i<entries_list.count(); i++) {

                QString entry_text      = entries_list[i];
                QString id_text         = Seer::parseFirst(entry_text, "id=",         '"', '"', false);
                QString expression_text = Seer::parseFirst(entry_text, "expression=", '"', '"', false);

                QList<QTreeWidgetItem*> matches = variablesTreeWidget->findItems(id_text, Qt::MatchExactly, 2);

                if (matches.count() == 0) {

                    QTreeWidgetItem* topItem = new QTreeWidgetItem;
                    topItem->setText(0, expression_text);
                    topItem->setText(1, "");
                    topItem->setFont(1, QFontDatabase::systemFont(QFontDatabase::FixedFont));
                    topItem->setText(2, id_text);
                    topItem->setText(3, "new");

                    variablesTreeWidget->addTopLevelItem(topItem);
                }
            }

        }else if (text.startsWith("^done,DataExpressionAdded={") && text.endsWith("}")) {

            // "^done,DataExpressionAdded={
            //          id=\"5\",
            //          expression=\"m\"
            //      }"

            QString frame_text      = Seer::parseFirst(text,       "DataExpressionAdded=", '{', '}', false);
            QString id_text         = Seer::parseFirst(frame_text, "id=",                  '"', '"', false);
            QString expression_text = Seer::parseFirst(frame_text, "expression=",          '"', '"', false);

            QList<QTreeWidgetItem*> matches = variablesTreeWidget->findItems(id_text, Qt::MatchExactly, 2);

            if (matches.count() == 0) {

                QTreeWidgetItem* topItem = new QTreeWidgetItem;
                topItem->setText(0, expression_text);
                topItem->setText(1, "");
                topItem->setFont(1, QFontDatabase::systemFont(QFontDatabase::FixedFont));
                topItem->setText(2, id_text);
                topItem->setText(3, "new");

                variablesTreeWidget->addTopLevelItem(topItem);

                refreshValues();

                emit raiseTab();
            }

        }else if (text.startsWith("^done,DataExpressionDeleted={") && text.endsWith("}")) {

            // "^done,DataExpressionDeleted={
            //          entry={id=\"1\",expression=\"s\"},
            //          entry={id=\"3\",expression=\"vb\"}
            //      }"

            QString frame_text = Seer::parseFirst(text, "DataExpressionDeleted=", '{', '}', false);

            QStringList entries_list = Seer::parse(frame_text, "entry=", '{', '}', false);

            for (int i=0; i<entries_list.count(); i++) {

                QString entry_text      = entries_list[i];
                QString id_text         = Seer::parseFirst(entry_text, "id=",         '"', '"', false);
                QString expression_text = Seer::parseFirst(entry_text, "expression=", '"', '"', false);

                QList<QTreeWidgetItem*> matches = variablesTreeWidget->findItems(id_text, Qt::MatchExactly, 2);

                qDeleteAll(matches);
            }

        }else if (text.contains(QRegularExpression("^([0-9]+)\\^done,value="))) {

            // "6^done,value=\"\\\"abc\\\"\""

            QString id_text    = text.section('^', 0,0);
            QString value_text = Seer::parseFirst(text, "value=", '"', '"', false);

            // Find the ones that match our 'id'.
            QList<QTreeWidgetItem*> matches = variablesTreeWidget->findItems(id_text, Qt::MatchExactly, 2);

            if (matches.count() == 1) {

                Q_ASSERT(matches.count() == 1);

                // There should be only one.
                QTreeWidgetItem* item = matches[0];

                // Mark each entry initially as "unused".
                // Later, some will be marked as "reused" or "new". Then the "unused" ones will
                // be deleted.
                QTreeWidgetItemIterator itmark(item);
                while (*itmark) {
                    (*itmark)->setText(3, "unused");
                    ++itmark;
                }

                // Set the value.
                handleItemCreate (item, value_text);

                emit raiseTab();

                // At this point, there are some new entries, some reused entries, and some unused ones.
                // For now, don't bother deleting 'unused' ones.
            }

        }else if (text.contains(QRegularExpression("^([0-9]+)\\^error,msg="))) {

            // "1^error,msg=\"No symbol \\\"j\\\" in current context.\""

            QString id_text  = text.section('^', 0,0);
            QString msg_text = Seer::parseFirst(text, "msg=", '"', '"', false);

            // Find the ones that match our 'id'.
            QList<QTreeWidgetItem*> matches = variablesTreeWidget->findItems(id_text, Qt::MatchExactly, 2);

            if (matches.count() == 1) {

                // There should be only one.
                QTreeWidgetItem* item = matches[0];

                // Remove any children.
                QList<QTreeWidgetItem*> children = item->takeChildren();

                qDeleteAll(children);

                // Set the text with the error message.
                item->setText(1, Seer::filterEscapes(msg_text));
                item->setText(3, "used");
            }

        }else if (text.startsWith("^error,msg=\"No registers.\"")) {
            variablesTreeWidget->clear();

        }else{
            // Ignore others.
        }

        variablesTreeWidget->resizeColumnToContents(0);
        variablesTreeWidget->resizeColumnToContents(1);
        variablesTreeWidget->resizeColumnToContents(2);
        variablesTreeWidget->resizeColumnToContents(3);

        break;
    }

    QApplication::restoreOverrideCursor();
}

void SeerVariableTrackerBrowserWidget::handleStoppingPointReached () {

    // Don't do any work if the widget is hidden.
    if (isHidden()) {
        return;
    }

    refresh();
}

void SeerVariableTrackerBrowserWidget::handleSessionTerminated () {

    // Delete previous contents.
    variablesTreeWidget->clear();
}

void SeerVariableTrackerBrowserWidget::refresh () {
    emit refreshVariableTrackerNames();
    emit refreshVariableTrackerValues();
}

void SeerVariableTrackerBrowserWidget::refreshValues () {
    emit refreshVariableTrackerValues();
}

void SeerVariableTrackerBrowserWidget::handleAddLineEdit () {

    QString variable = variableAddLineEdit->text();

    variableAddLineEdit->clear();

    if (variable != "") {
        emit addVariableExpression(variable);

        // After sending the 'add' signal, schedule a 'refresh' 200ms later.
        QTimer::singleShot(200, this, &SeerVariableTrackerBrowserWidget::refresh);
    }
}

void SeerVariableTrackerBrowserWidget::handleDeleteToolButton () {

    // Get selected tree items.
    QList<QTreeWidgetItem*> items = variablesTreeWidget->selectedItems();

    // Build a string that is a list of variable ids.
    QString variableids;

    QList<QTreeWidgetItem*>::iterator i;
    for (i = items.begin(); i != items.end(); ++i) {

        if (i != items.begin()) {
            variableids += " ";
        }

        variableids += (*i)->text(2);
    }

    // Don't do anything if the list of variables is empty.
    if (variableids == "") {
        return;
    }

    // Send the signal.
    emit deleteVariableExpressions(variableids);
}

void SeerVariableTrackerBrowserWidget::handleDeleteAllToolButton () {

    emit deleteVariableExpressions("*");
}

void SeerVariableTrackerBrowserWidget::handleItemExpanded (QTreeWidgetItem* item) {

    Q_UNUSED(item);

    variablesTreeWidget->resizeColumnToContents(0);
    variablesTreeWidget->resizeColumnToContents(1);
    variablesTreeWidget->resizeColumnToContents(2);
    variablesTreeWidget->resizeColumnToContents(3);
}

void SeerVariableTrackerBrowserWidget::handleItemCollapsed (QTreeWidgetItem* item) {

    Q_UNUSED(item);

    variablesTreeWidget->resizeColumnToContents(0);
    variablesTreeWidget->resizeColumnToContents(1);
    variablesTreeWidget->resizeColumnToContents(2);
    variablesTreeWidget->resizeColumnToContents(3);
}

void SeerVariableTrackerBrowserWidget::handleItemEntered (QTreeWidgetItem* item, int column) {

    Q_UNUSED(column);

    item->setToolTip(0, item->text(0) + " : " + Seer::elideText(item->text(1), Qt::ElideRight, 100));

    for (int i=1; i<variablesTreeWidget->columnCount(); i++) { // Copy tooltip to other columns.
        item->setToolTip(i, item->toolTip(0));
    }
}

void SeerVariableTrackerBrowserWidget::showEvent (QShowEvent* event) {

    QWidget::showEvent(event);

    refresh();
}

void SeerVariableTrackerBrowserWidget::handleItemCreate (QTreeWidgetItem* item, const QString& value_text) {

    handleItemCreate(item, item->text(2), item->text(0), value_text);
}

void SeerVariableTrackerBrowserWidget::handleItemCreate (QTreeWidgetItem* parentItem, const QString& id_text, const QString& name_text, const QString& value_text) {

    // Fill in parent item. Whether is a simple or complex entry.
    parentItem->setText(0, name_text);
    parentItem->setText(1, Seer::filterEscapes(value_text));
    parentItem->setFont(1, QFontDatabase::systemFont(QFontDatabase::FixedFont));
    parentItem->setText(2, id_text);
    parentItem->setText(3, "reused");

    // Look for bookmarks. This indicates a nested structure.
    // There are two types. With and without an address.

    QString capture0; // With bookends.
    QString capture1; // Without.

    QRegularExpression withaddress_re("^@0[xX][0-9a-fA-F]+: \\{(.*?)\\}$");
    QRegularExpressionMatch withaddress_match = withaddress_re.match(value_text, 0, QRegularExpression::PartialPreferCompleteMatch);

    if (withaddress_match.hasMatch()) {
        capture0 = withaddress_match.captured(0);
        capture1 = withaddress_match.captured(1);

    }else{
        QRegularExpression noaddress_re("^\\{(.*?)\\}$");
        QRegularExpressionMatch noaddress_match = noaddress_re.match(value_text, 0, QRegularExpression::PartialPreferCompleteMatch);

        if (noaddress_match.hasMatch()) {
            capture0 = noaddress_match.captured(0);
            capture1 = noaddress_match.captured(1);
        }
    }

    // Simple entries don't have children. Delete them.
    // Then we're done.
    if (capture0 == "" || capture1 == "") {

        while (parentItem->childCount() > 0) {
            QTreeWidgetItem* item = parentItem->child(0);
            delete item;
        }

        return;
    }

    // Add the complex entry to the tree. Reuse, if possible.
    // Instead of creating a new tree each time, we will reuse existing items, if they are there.
    // This allows the expanded items to remain expanded. We start by looking for matches that
    // may already be there. If there are matches, the code will reuse it.  If not, a new item
    // is created by the code. Note, when searching, we only look at the current level. Not any
    // children.

    // Use the one without bookends.
    QString text = capture1;

    // Convert to a list of name/value pairs.
    QStringList nv_pairs = Seer::parseCommaList(text, '{', '}');

    // Go through each pair and add the name and its value to the tree.
    for (const auto& nv : nv_pairs) {

        QStringPair pair = Seer::parseNameValue(nv, '=');

        // Look for the existing child, if any so we can reuse it.
        QTreeWidgetItem* childItem = 0;
        for (int i=0; i<parentItem->childCount(); i++) {
            if (parentItem->child(i)->text(0) == pair.first) {
                childItem = parentItem->child(i);
                childItem->setText(0, pair.first);
                childItem->setText(1, pair.second);
                childItem->setFont(1, QFontDatabase::systemFont(QFontDatabase::FixedFont));
                childItem->setText(2, id_text);
                childItem->setText(3, "reused");
                break;
            }
        }

        // Otherwise, create a new child.
        if (childItem == 0) {
            childItem = new QTreeWidgetItem;
            childItem->setText(0, pair.first);
            childItem->setText(1, pair.second);
            childItem->setFont(1, QFontDatabase::systemFont(QFontDatabase::FixedFont));
            childItem->setText(2, id_text);
            childItem->setText(3, "new");

            parentItem->addChild(childItem);
        }

        handleItemCreate(childItem, id_text, childItem->text(0), childItem->text(1));
    }
}

void SeerVariableTrackerBrowserWidget::handleContextMenu (const QPoint& pos) {

    // Get the item at the cursor.
    QTreeWidgetItem* item = variablesTreeWidget->itemAt(pos);

    // Construct the menu.
    QAction* addMemoryVisualizerAction           = new QAction();
    QAction* addMemoryAsteriskVisualizerAction   = new QAction();
    QAction* addMemoryAmpersandVisualizerAction  = new QAction();
    QAction* addArrayVisualizerAction            = new QAction();
    QAction* addArrayAsteriskVisualizerAction    = new QAction();
    QAction* addArrayAmpersandVisualizerAction   = new QAction();
    QAction* addStructVisualizerAction           = new QAction();
    QAction* addStructAsteriskVisualizerAction   = new QAction();
    QAction* addStructAmpersandVisualizerAction  = new QAction();

    QMenu menu("Options", this);

    QMenu memoryVisualizerMenu("Add variable to a Memory Visualizer");
    memoryVisualizerMenu.addAction(addMemoryVisualizerAction);
    memoryVisualizerMenu.addAction(addMemoryAsteriskVisualizerAction);
    memoryVisualizerMenu.addAction(addMemoryAmpersandVisualizerAction);
    menu.addMenu(&memoryVisualizerMenu);

    QMenu arrayVisualizerMenu("Add variable to an Array Visualizer");
    arrayVisualizerMenu.addAction(addArrayVisualizerAction);
    arrayVisualizerMenu.addAction(addArrayAsteriskVisualizerAction);
    arrayVisualizerMenu.addAction(addArrayAmpersandVisualizerAction);
    menu.addMenu(&arrayVisualizerMenu);

    QMenu structVisualizerMenu("Add variable to a Struct Visualizer");
    structVisualizerMenu.addAction(addStructVisualizerAction);
    structVisualizerMenu.addAction(addStructAsteriskVisualizerAction);
    structVisualizerMenu.addAction(addStructAmpersandVisualizerAction);
    menu.addMenu(&structVisualizerMenu);

    QAction* copyAction    = menu.addAction("Copy selected");
    QAction* copyAllAction = menu.addAction("Copy all");

    QString actionText;
    if (item != 0) {
        actionText = item->text(0);
    }

    QString variable;
    if (item != 0) {
        // Build up a variable string, incase it is a nested struct.
        QTreeWidgetItem* i = item;
        variable = item->text(0);
        while (i->parent() != 0) {
            variable = i->parent()->text(0) + "." + variable;
            i = i->parent();
        }
    }

    addMemoryVisualizerAction->setText(QString("\"%1\"").arg(actionText));
    addMemoryAsteriskVisualizerAction->setText(QString("\"*%1\"").arg(actionText));
    addMemoryAmpersandVisualizerAction->setText(QString("\"&&%1\"").arg(actionText));
    addArrayVisualizerAction->setText(QString("\"%1\"").arg(actionText));
    addArrayAsteriskVisualizerAction->setText(QString("\"*%1\"").arg(actionText));
    addArrayAmpersandVisualizerAction->setText(QString("\"&&%1\"").arg(actionText));
    addStructVisualizerAction->setText(QString("\"%1\"").arg(actionText));
    addStructAsteriskVisualizerAction->setText(QString("\"*%1\"").arg(actionText));
    addStructAmpersandVisualizerAction->setText(QString("\"&&%1\"").arg(actionText));

    // If no selected item, disable everything but allow 'copyall'.
    if (item == 0) {
        memoryVisualizerMenu.setEnabled(false);
        arrayVisualizerMenu.setEnabled(false);
        structVisualizerMenu.setEnabled(false);
        copyAction->setEnabled(false);
    }

    // Execute the menu. Return if nothing.
    QAction* action = menu.exec(variablesTreeWidget->mapToGlobal(pos));

    if (action == 0) {
        return;
    }

    if (action == copyAction || action == copyAction) {
        // Get selected tree items.
        QList<QTreeWidgetItem*> items;

        // Get list of 'select' items.
        if (action == copyAction) {
            items = variablesTreeWidget->selectedItems();
        }

        // Get list of 'all' items.
        if (action == copyAllAction) {
            items = variablesTreeWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);
        }

        // Populate the clipboard.
        if (items.count() == 0) {
            return;
        }

        QClipboard* clipboard = QGuiApplication::clipboard();

        QString text;

        for (int i=0; i<items.count(); i++) {

            if (i != 0) {
                text += '\n';
            }

            text += items[i]->text(0) + ":" + items[i]->text(1);
        }

        clipboard->setText(text, QClipboard::Clipboard);
        clipboard->setText(text, QClipboard::Selection);
    }

    // Handle adding memory to visualize.
    if (action == addMemoryVisualizerAction) {

        // Emit the signals.
        if (variable != "") {
            emit addMemoryVisualize(variable);
        }

        return;
    }

    // Handle adding memory to visualize.
    if (action == addMemoryAsteriskVisualizerAction) {

        // Emit the signals.
        if (variable != "") {
            emit addMemoryVisualize(QString("*") + variable);
        }

        return;
    }

    // Handle adding memory to visualize.
    if (action == addMemoryAmpersandVisualizerAction) {

        // Emit the signals.
        if (variable != "") {
            emit addMemoryVisualize(QString("&") + variable);
        }

        return;
    }

    // Handle adding array to visualize.
    if (action == addArrayVisualizerAction) {

        // Emit the signals.
        if (variable != "") {
            emit addArrayVisualize(variable);
        }

        return;
    }

    // Handle adding array to visualize.
    if (action == addArrayAsteriskVisualizerAction) {

        // Emit the signals.
        if (variable != "") {
            emit addArrayVisualize(QString("*") + variable);
        }

        return;
    }

    // Handle adding array to visualize.
    if (action == addArrayAmpersandVisualizerAction) {

        // Emit the signals.
        if (variable != "") {
            emit addArrayVisualize(QString("&") + variable);
        }

        return;
    }

    // Handle adding struct to visualize.
    if (action == addStructVisualizerAction) {

        // Emit the signals.
        if (variable != "") {
            emit addStructVisualize(variable);
        }

        return;
    }

    // Handle adding array to visualize.
    if (action == addStructAsteriskVisualizerAction) {

        // Emit the signals.
        if (variable != "") {
            emit addStructVisualize(QString("*") + variable);
        }

        return;
    }

    // Handle adding array to visualize.
    if (action == addStructAmpersandVisualizerAction) {

        // Emit the signals.
        if (variable != "") {
            emit addStructVisualize(QString("&") + variable);
        }

        return;
    }
}

