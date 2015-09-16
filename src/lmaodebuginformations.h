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

#ifndef LMAODEBUGINFORMATIONS_H
#define LMAODEBUGINFORMATIONS_H

#include <QObject>
#include <QHash>

class LMAODebugInformations : public QObject
{
    Q_OBJECT
public:
    explicit LMAODebugInformations(QObject *parent = 0);

    typedef struct MemoryAddress {
        int address;
        enum {
            CODE = 0,
            DATA = 1
        } type;
    } MemoryAddress;

    typedef struct SourcePosition {
        int first_line;
        int first_column;
        int last_line;
        int last_column;
        enum {
            CODE = 0,
            DATA = 1
        } type;
    } SourcePosition;

    int read_from_file(QString filename);
    int get_label_address(QString label, bool* is_inside_codesection = 0);
    int get_source_position(int memory_cell, int& first_line, int& first_column, int& last_line, int& last_column, bool* is_inside_codesection = 0);
    int get_first_memory_cell_starting_at_line(int line, int &memory_cell, bool* is_inside_codesection);
    int get_steps_until_entry_point();
    bool get_address_label(int address, QString &label, bool &is_inside_codesection);


signals:

public slots:

private:
    QHash<QString, MemoryAddress> labels_in_memory;
    QHash<int, SourcePosition> memory_cells_in_source;
    int steps_until_entry_point;
    QString hell_source_file;
    QString malbolge_file;

};

#endif // LMAODEBUGINFORMATIONS_H
