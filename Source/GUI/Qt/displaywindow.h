/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a GPLv3+/MPLv2+ license that can
 *  be found in the License.html file in the root of the source tree.
 */

#ifndef DISPLAYWINDOW_H
#define DISPLAYWINDOW_H

#include <QFileInfo>
#include <QString>

class QTableWidget;

namespace MediaConch {

class MainWindow;
class DisplayMenu;

class DisplayWindow : public QObject
{
    Q_OBJECT

public:
    explicit DisplayWindow(MainWindow* m);
    ~DisplayWindow();

    void displayDisplay();
    void clearDisplay();
    void fillTable();

private:
    // Visual elements
    DisplayMenu  *displayMenu;
    MainWindow   *mainwindow;

private Q_SLOTS:
    void add_new_file();
    void delete_file();
};

}

#endif // DISPLAYWINDOW_H
