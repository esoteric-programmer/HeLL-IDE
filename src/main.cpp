/*
 * This file is part of HeLL IDE, IDE for the low-level Malbolge
 * assembly language HeLL.
 * Copyright (C) 2013 Matthias Lutter
 *
 * HeLL IDE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HeLL IDE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include <QCoreApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("malbolge");
    QCoreApplication::setOrganizationDomain("matthias-ernst.eu");
    QCoreApplication::setApplicationName("HeLL IDE");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    return a.exec();
}
