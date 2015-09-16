/*
 * This file is part of HeLL IDE, IDE for the low-level Malbolge
 * assembly language HeLL.
 * Copyright (C) 2013 Matthias Ernst
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

#ifndef REPLACE_H
#define REPLACE_H

#include <QDialog>

namespace Ui {
class Replace;
}

class Replace : public QDialog
{
    Q_OBJECT

public:
    explicit Replace(QWidget *parent = 0);
    ~Replace();

protected:
    void showEvent(QShowEvent* event);

signals:
    void find(QString text, bool case_sensitive, bool backwards);
    void replace(QString text, QString replaceString, bool case_sensitive, bool backwards, bool all);

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

private:
    Ui::Replace *ui;
};

#endif // REPLACE_H
