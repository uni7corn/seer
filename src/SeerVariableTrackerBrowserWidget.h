#pragma once

#include <QtWidgets/QWidget>
#include <QtCore/QString>
#include "ui_SeerVariableTrackerBrowserWidget.h"

class SeerVariableTrackerBrowserWidget : public QWidget, protected Ui::SeerVariableTrackerBrowserWidgetForm {

    Q_OBJECT

    public:
        explicit SeerVariableTrackerBrowserWidget (QWidget* parent = 0);
       ~SeerVariableTrackerBrowserWidget ();

    public slots:
        void                handleText                      (const QString& text);
        void                handleStoppingPointReached      ();
        void                handleSessionTerminated         ();
        void                refresh                         ();
        void                refreshValues                   ();

    private slots:
        void                handleAddLineEdit               ();
        void                handleDeleteToolButton          ();
        void                handleDeleteAllToolButton       ();
        void                handleItemEntered               (QTreeWidgetItem* item, int column);
        void                handleItemExpanded              (QTreeWidgetItem* item);
        void                handleItemCollapsed             (QTreeWidgetItem* item);
        void                handleContextMenu               (const QPoint& pos);

    signals:
        void                refreshVariableTrackerNames     ();
        void                refreshVariableTrackerValues    ();
        void                addVariableExpression           (QString expression);
        void                deleteVariableExpressions       (QString expressionids);
        void                addMemoryVisualize              (QString expression);
        void                addArrayVisualize               (QString expression);
        void                addStructVisualize              (QString expression);
        void                raiseTab                        ();

    protected:
        void                handleItemCreate                (QTreeWidgetItem* item,       const QString& value_text);
        void                handleItemCreate                (QTreeWidgetItem* parentItem, const QString& id_text, const QString& name_text, const QString& value_text);
        void                showEvent                       (QShowEvent* event);

    private:

};

