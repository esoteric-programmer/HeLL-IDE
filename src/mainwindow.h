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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTableWidget>
#include "find.h"
#include "replace.h"

namespace Ui {
class HeLL;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void debug_continue();
    void debug_step();
    void debug_pause();
    void debug_reset();
    void add_breakpoint(int line);
    void remove_breakpoint(int line);
    void register_runtime_expression(QString expression, int expression_id);
    void remove_runtime_expression(int expression_id);
    void clear_runtime_expressions();

public slots:
    void gotoHeLLCodePosition(int line, int column);
    void gotoHeLLCodePosition(int first_line, int first_column, int last_line, int last_column);
    void debugging_paused(int first_line_d, int first_column_d, int last_line_d, int last_column_d, int first_line_c, int first_column_c, int last_line_c, int last_column_c, bool recommend_displaying_code_section);
    void debugging_registers_changed(QString c_label, QString mem_c_xlat, QString mem_d, QString a_reg_val);
    void debugging_runtime_expression_value_changed(QString new_value, int expression_id);
    void debugging_continued();
    void query_breakpoints();

private slots:
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSave_as_triggered();
    void on_actionQuit_triggered();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionCut_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionDelete_triggered();
    void on_actionSelect_all_triggered();
    void on_actionFind_triggered();
    void on_actionFind_next_triggered();
    void on_actionReplace_triggered();
    void on_actionPreferences_triggered();
    void on_actionAssemble_triggered();
    void on_actionRun_triggered();
    void on_actionStop_triggered();
    void on_actionStart_triggered();
    void on_actionContinue_triggered();
    void on_actionStep_triggered();
    void on_actionPause_triggered();
    void on_actionReset_triggered();
    void on_actionStop_2_triggered();
    void on_actionToggle_Breakpoint_triggered();
    void on_actionAbout_triggered();
    void on_tableWidget_itemChanged(QTableWidgetItem *item);

    void on_textEdit_textChanged();
    void on_textEdit_cursorPositionChanged(int line, int index);

    void get_assembled_filename(QString filename);

    void on_console_startExecute();
    void on_console_stopExecute();

    void find(QString text, bool case_sensitive, bool backwards);

    void replace(QString text, QString replaceString, bool case_sensitive, bool backwards, bool all);


protected:
    void closeEvent (QCloseEvent *event);

private:
    Ui::HeLL *ui;
    QString hell_file_name;
    QString hell_file_display_name;
    QString last_malbolge_file;
    bool hell_filename_is_dir;
    void updateWindowsTitle();
    //bool hell_changes_saved_since_last_assembling;
    QLabel statusbar_position;
    Find* findDialog;
    bool searched;
    Replace* replaceDialog;
    bool _find(QString text, bool case_sensitive, bool backwards, QWidget* trigger, bool wrap_around = true, bool show_error_msg = true);
    void findNext(QWidget* trigger);
    void read_file(QString gilename);
};

#endif // MAINWINDOW_H
