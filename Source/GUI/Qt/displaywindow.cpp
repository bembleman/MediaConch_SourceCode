/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a GPLv3+/MPLv2+ license that can
 *  be found in the License.html file in the root of the source tree.
 */

#include "displaywindow.h"
#include "displaymenu.h"
#include "mainwindow.h"
#include <QTableWidget>
#include <QHeaderView>
#include <QDir>
#include <QFileDialog>
#include <QPushButton>
#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

namespace MediaConch {

//***************************************************************************
// Constructor / Desructor
//***************************************************************************

DisplayWindow::DisplayWindow(MainWindow* m) : displayMenu(NULL), mainwindow(m)
{
    // Visual elements
    displayMenu = NULL;
}

DisplayWindow::~DisplayWindow()
{
    clearDisplay();
}

void DisplayWindow::displayDisplay()
{
    displayMenu = new DisplayMenu(mainwindow);
    fillTable();
    mainwindow->set_widget_to_layout(displayMenu);

    QTableWidget *table = displayMenu->get_display_table();
    if (table)
    {
        table->verticalHeader()->hide();
        table->horizontalHeader()->setStretchLastSection(true);
        table->resizeColumnsToContents();
        table->resizeRowsToContents();
    }

    QObject::connect(displayMenu->get_addFile_button(), SIGNAL(clicked()),
                     this, SLOT(add_new_file()));
    QObject::connect(displayMenu->get_delFile_button(), SIGNAL(clicked()),
                     this, SLOT(delete_file()));
}

void DisplayWindow::clearDisplay()
{
    if (!displayMenu)
        return;

    mainwindow->remove_widget_from_layout(displayMenu);
    delete displayMenu;
    displayMenu = NULL;
}

void DisplayWindow::fillTable()
{
    if (!displayMenu)
        return;

    QTableWidget *table = displayMenu->get_display_table();
    if (!table)
        return;

    table->clear();
    table->setRowCount(0);

    QTableWidgetItem *itemFile = new QTableWidgetItem(tr("File"));
    table->setHorizontalHeaderItem(0, itemFile);
    QTableWidgetItem *itemDir = new QTableWidgetItem(tr("Path"));
    table->setHorizontalHeaderItem(1, itemDir);

    std::vector<QString>& displays = mainwindow->get_displays();
    for (size_t i = 0; i < displays.size(); ++i)
    {
        QFileInfo file(displays[i]);

        QTableWidgetItem *itemFile = new QTableWidgetItem(file.baseName());
        QTableWidgetItem *itemDir = new QTableWidgetItem(file.absoluteFilePath());

        Qt::ItemFlags flag = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsTristate;
        itemDir->setFlags(flag);
        itemFile->setFlags(flag);
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, itemFile);
        table->setItem(row, 1, itemDir);
    }

    table->resizeColumnsToContents();
    table->resizeRowsToContents();
    table->horizontalHeader()->setStretchLastSection(true);
}

void DisplayWindow::add_new_file()
{
    if (!displayMenu)
        return;

    QStringList List = QFileDialog::getOpenFileNames(mainwindow, "Open file", "", "Display files (*.xsl);;All (*.*)", 0, QFileDialog::DontUseNativeDialog);
    if (List.empty())
        return;

#if QT_VERSION >= 0x050400
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#elif QT_VERSION >= 0x050000
    QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#else
    QString path = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif

    QDir dir(path);
    dir.cd("Display");

    if (!dir.exists())
        if (!dir.mkpath(dir.absolutePath()))
            return;

    std::vector<QString>& displays = mainwindow->get_displays();
    for (int i = 0; i < List.count(); ++i)
    {
        QFileInfo file(List[i]);
        for (int j = 0; 1; ++j)
        {
            QString str;
            if (!j)
                str = QString("%1/%2.xsl").arg(dir.absolutePath()).arg(file.baseName());
            else
                str = QString("%1/%2_%3.xsl").arg(dir.absolutePath()).arg(file.baseName()).arg(j);
            QFile info(str);
            if (info.exists())
                continue;

            QFile::copy(file.absoluteFilePath(), str);
            displays.push_back(str);
            break;
        }
    }
    fillTable();
}

void DisplayWindow::delete_file()
{
    if (!displayMenu)
        return;

    QTableWidget *table = displayMenu->get_display_table();
    if (!table)
        return;

    QItemSelectionModel *select = table->selectionModel();

    if (!select->hasSelection())
        return;

    QModelIndexList list = select->selectedRows();

    for (int i = 0; i < list.count(); ++i)
    {
        QTableWidgetItem* itemDir = table->item(list[i].row(), 1);
        if (!itemDir)
            continue;
        QFile file(itemDir->text());
        file.remove();
        table->removeRow(list[i].row());
    }
}

}
