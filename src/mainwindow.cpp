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


#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtGlobal>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QCloseEvent>
#include <QProcess>
#include <QDebug>
#include <QSettings>
#include "qscilexerhell.h"
#include "qmalbolgerunner.h"
#include "qmalbolgedebugger.h"
#include "preferencesdialog.h"
#include "properties.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::HeLL)
{
    ui->setupUi(this);

    qRegisterMetaType<LMAODebugInformations::SourcePosition>("LMAODebugInformations::SourcePosition");
    qRegisterMetaType<QLinkedList<LMAODebugInformations::SourcePosition> >("QLinkedList<LMAODebugInformations::SourcePosition>");

    QSettings application_settings;
    this->hell_file_name = application_settings.value("editor/lastfolder").toString();
    this->hell_filename_is_dir = true;
    this->hell_file_display_name = "(unnamed)";

    this->updateWindowsTitle();

    QsciLexer *lexer = new QsciLexerHeLL();
    ui->textEdit->setLexer(lexer);
    ui->textEdit->setFolding(QsciScintilla::PlainFoldStyle);

    ui->textEdit->SendScintilla(QsciScintilla::SCI_SETFOLDFLAGS, QsciScintilla::SC_FOLDFLAG_LEVELNUMBERS);

    ui->statusBar->addPermanentWidget(&this->statusbar_position);

    this->findDialog = new Find(this);
    this->replaceDialog = new Replace(this);
    searched = false;
    connect(this->findDialog, SIGNAL(find(QString,bool,bool)), this, SLOT(find(QString,bool,bool)));
    connect(this->replaceDialog, SIGNAL(find(QString,bool,bool)), this, SLOT(find(QString,bool,bool)));
    connect(this->replaceDialog, SIGNAL(replace(QString,QString,bool,bool,bool)), this, SLOT(replace(QString,QString,bool,bool,bool)));

    connect(this->ui->console, SIGNAL(gotoHeLLCodePosition(int,int)), this, SLOT(gotoHeLLCodePosition(int,int)));

    QStringList cmdline = QCoreApplication::arguments();
    if (cmdline.length() > 1) {
        read_file(cmdline.at(1));
    }

    this->ui->textEdit->markerDefine(QsciScintilla::Circle, 0);
    this->ui->textEdit->setMarkerBackgroundColor(Qt::red,0);
    this->ui->textEdit->markerDefine(QsciScintilla::RightArrow, 1);
    this->ui->textEdit->setMarkerBackgroundColor(Qt::yellow,1);
    this->ui->textEdit->markerDefine(QsciScintilla::RightArrow, 2);
    this->ui->textEdit->setMarkerBackgroundColor(Qt::green,2);

    QStringList headernames;
    headernames.append(QString("address"));
    headernames.append(QString("value"));
    this->ui->tableWidget->setHorizontalHeaderLabels(headernames);
    this->ui->tableWidget->horizontalHeader()->show();
#if QT_VERSION >= 0x050000
    this->ui->tableWidget->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Interactive);
    this->ui->tableWidget->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
#else
    this->ui->tableWidget->horizontalHeader()->setResizeMode(0,QHeaderView::Interactive);
    this->ui->tableWidget->horizontalHeader()->setResizeMode(1,QHeaderView::Stretch);
#endif
    this->ui->tableWidget->insertRow(0);
    QTableWidgetItem* item = new QTableWidgetItem();
    this->ui->tableWidget->setItem(0,0,item);
    item = new QTableWidgetItem();
    item->setFlags(item->flags() ^ Qt::ItemIsEditable);
    this->ui->tableWidget->setItem(0,1,item);

    this->ui->verticalFrame->setVisible(false);
}

void MainWindow::updateWindowsTitle() {
    this->setWindowTitle((this->ui->textEdit->SendScintilla(QsciScintilla::SCI_GETMODIFY)!=0?QString("*"):QString()).append(this->hell_file_display_name).append(" - HeLL IDE"));
}

void MainWindow::on_actionNew_triggered() {

    if (this->ui->console->is_running()) {
        QMessageBox::StandardButton clicked = QMessageBox::question(this, QString("Abort process?"), QString("A process is running. Quit the running process?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
        if (clicked == QMessageBox::No) {
            return;
        }else {
            this->ui->console->stop();
            while(this->ui->console->is_running()) {
                QCoreApplication::processEvents();
            }
        }
    }

    // save changes?
    if (this->ui->textEdit->SendScintilla(QsciScintilla::SCI_GETMODIFY)!=0) {
        QMessageBox::StandardButton clicked = QMessageBox::question(this, QString("Save changes?"), QString("Do you want to save the changes to ").append(this->hell_file_display_name).append("?"), QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::Yes);
        if (clicked == QMessageBox::Yes) {
            this->on_actionSave_triggered();
            if (this->ui->textEdit->SendScintilla(QsciScintilla::SCI_GETMODIFY)!=0) {
                return;
            }
        } else if (clicked == QMessageBox::Cancel) {
            return;
        }
    }

    this->ui->console->cleanUp();

    this->ui->textEdit->setText("");

    if (this->hell_file_name.length() > 0 && !this->hell_filename_is_dir) {
        QDir dir(this->hell_file_name);
        dir.cdUp();
        this->hell_file_name = dir.absolutePath();
        hell_filename_is_dir = true;
    }
    hell_file_display_name = "(unnamed)";
    this->ui->textEdit->SendScintilla(QsciScintilla::SCI_SETSAVEPOINT);
    this->updateWindowsTitle();
}

void MainWindow::on_actionOpen_triggered() {

    if (this->ui->console->is_running()) {
        QMessageBox::StandardButton clicked = QMessageBox::question(this, QString("Abort process?"), QString("A process is running. Quit the running process?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
        if (clicked == QMessageBox::No) {
            return;
        }else {
            this->ui->console->stop();
            while(this->ui->console->is_running()) {
                QCoreApplication::processEvents();
            }
        }
    }

    // save changes?
    if (this->ui->textEdit->SendScintilla(QsciScintilla::SCI_GETMODIFY)!=0) {
        QMessageBox::StandardButton clicked = QMessageBox::question(this, QString("Save changes?"), QString("Do you want to save the changes to ").append(this->hell_file_display_name).append("?"), QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::Yes);
        if (clicked == QMessageBox::Yes) {
            this->on_actionSave_triggered();
            if (this->ui->textEdit->SendScintilla(QsciScintilla::SCI_GETMODIFY)!=0) {
                return;
            }
        } else if (clicked == QMessageBox::Cancel) {
            return;
        }
    }


    QString selectedFilter = "";
    QString filename = QFileDialog::getOpenFileName(this, QString(), this->hell_file_name, "HeLL (*.hell)", &selectedFilter, 0);
    read_file(filename);
}

void MainWindow::read_file(QString filename) {
    if (filename.length() > 0) {

        QFile file(filename);
        if(!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Error", QString("Cannot open file ").append(filename).append(": ").append(file.errorString()));
        }

        this->ui->console->cleanUp();
        QTextStream in(&file);
        this->ui->textEdit->setText(in.readAll());

        file.close();

        this->hell_file_name = filename;
        hell_filename_is_dir = false;
        QDir dir(this->hell_file_name);
        hell_file_display_name = dir.dirName();
        this->ui->textEdit->SendScintilla(QsciScintilla::SCI_SETSAVEPOINT);
        this->updateWindowsTitle();

        QRegExp entry_point_regexp("(^|[\\s:{])(ENTRY:)");
        int pos = 0;
        QString source = this->ui->textEdit->text();
        if ((pos=entry_point_regexp.indexIn(source, pos)) >= 0) {
            int entry_position = entry_point_regexp.pos(2);
            int entry_line, entry_column = -1;
            // TODO: UTF-8-Kompatible Positionsberechnung!!
            // last line
            this->ui->textEdit->lineIndexFromPosition(source.length(), &entry_line, &entry_column);
            this->ui->textEdit->setCursorPosition(entry_line, 0);
            // entry point
            this->ui->textEdit->lineIndexFromPosition(entry_position, &entry_line, &entry_column);
            this->ui->textEdit->setCursorPosition(entry_line, entry_column);
        }
        this->ui->textEdit->setFocus();
        dir.cdUp();
        QSettings application_settings;
        application_settings.setValue("editor/lastfolder",dir.path());
    }
}

void MainWindow::on_actionSave_triggered() {
    if (this->hell_file_name.length() <= 0 || this->hell_filename_is_dir) {
        this->on_actionSave_as_triggered();
        return;
    }

    QFile file(this->hell_file_name);
    if(!file.open(QIODevice::WriteOnly)) {
        this->on_actionSave_as_triggered();
        return;
    }

    QTextStream out(&file);
    out << this->ui->textEdit->text();

    file.close();
    this->ui->textEdit->SendScintilla(QsciScintilla::SCI_SETSAVEPOINT);
    this->updateWindowsTitle();

}

void MainWindow::on_actionSave_as_triggered() {
    QString selectedFilter = "";
    QString filename = QFileDialog::getSaveFileName(this, QString(), this->hell_file_name, "HeLL (*.hell)", &selectedFilter, 0);
    if (filename.length() > 0) {

        if (!filename.contains('.'))
            filename = filename.append(".hell");

        QFile file(filename);
        if(!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "Error", QString("Cannot save file ").append(filename).append(": ").append(file.errorString()));
            return;
        }

        QTextStream out(&file);
        out << this->ui->textEdit->text();

        file.close();

        this->hell_file_name = filename;
        hell_filename_is_dir = false;
        QDir dir(this->hell_file_name);
        hell_file_display_name = dir.dirName();
        this->ui->textEdit->SendScintilla(QsciScintilla::SCI_SETSAVEPOINT);
        this->updateWindowsTitle();
        dir.cdUp();
        QSettings application_settings;
        application_settings.setValue("editor/lastfolder",dir.path());
    }
}

void MainWindow::on_actionQuit_triggered() {
    this->close();
}

void MainWindow::on_actionAbout_triggered() {
    QMessageBox::about(this, "About HeLL IDE", "HeLL IDE v0.1.1\n\n"
                       "(c) 2014-2017 Matthias Lutter, released under GNU GPL v3.\n"
                       "https://lutter.cc/\n"
                       "This software contains some code snippets of the qterminalwidget project, "
                       "which is (c) by simber86 and is released under GNU GPL v3, too.\n"
                       "https://code.google.com/p/qterminalwidget/\n"
                       "This software uses Qt and QScintilla, which are available under GNU GPL v3, too.\n"
                       "Please read the LICENSE file.");
}

void MainWindow::on_textEdit_textChanged() {
    this->updateWindowsTitle();
}

void MainWindow::closeEvent (QCloseEvent *event) {

    this->findDialog->hide();

    if (this->ui->console->is_running()) {
        QMessageBox::StandardButton clicked = QMessageBox::question(this, QString("Abort process?"), QString("A process is running. Quit anyway?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
        if (clicked == QMessageBox::No) {
            event->ignore();
            return;
        }else {
            this->ui->console->stop();
            while(this->ui->console->is_running()) {
                QCoreApplication::processEvents();
            }
        }
    }

    // save changes?
    if (this->ui->textEdit->SendScintilla(QsciScintilla::SCI_GETMODIFY)!=0) {
        QMessageBox::StandardButton clicked = QMessageBox::question(this, QString("Save changes?"), QString("Do you want to save the changes to ").append(this->hell_file_display_name).append("?"), QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::Yes);
        if (clicked == QMessageBox::Yes) {
            this->on_actionSave_triggered();
            if (this->ui->textEdit->SendScintilla(QsciScintilla::SCI_GETMODIFY)!=0) {
                event->ignore();
                return;
            }
        } else if (clicked == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }
    event->accept();
    QApplication::quit();
}

void MainWindow::on_actionFind_triggered() {
    findDialog->show();
    findDialog->raise();
}


void MainWindow::on_actionFind_next_triggered() {
    findNext(this);
}

void MainWindow::findNext(QWidget* trigger) {
    if (!searched) {
        on_actionFind_triggered();
    } else {
        if (!ui->textEdit->findNext()) {
            searched = false;
            QMessageBox::information(trigger,QString("Not found"),QString("The search string has not been found."));
        }
    }
}


void MainWindow::on_actionReplace_triggered() {
    replaceDialog->show();
    replaceDialog->raise();
}


void MainWindow::on_actionPreferences_triggered() {
    PreferencesDialog dialog;
    dialog.exec();
}


void MainWindow::on_actionAssemble_triggered() {

    this->on_actionSave_triggered();

    if (this->ui->textEdit->SendScintilla(QsciScintilla::SCI_GETMODIFY)!=0 || this->hell_file_name.length() <= 0 || this->hell_filename_is_dir)
        return;

    QSettings application_settings;
    QString lmao = application_settings.value("assembler/command").toString();
    if (lmao.length() <= 0)
        lmao = "lmao";

    int set_fast_flag = application_settings.value("assembler/fastflag", FAST_FLAG_REASONABLE).toInt();
    bool fast_flag = (set_fast_flag == FAST_FLAG_ALWAYS);

    QStringList lmao_params;
    if (fast_flag) {
        lmao_params << "-f";
    }
    lmao_params << this->hell_file_name;


    QIOWorker* execute = new QIOProcessWorker(lmao, lmao_params);
    connect(execute, SIGNAL(AssemblyOutput(QString)), this, SLOT(get_assembled_filename(QString)));
    this->ui->console->execute(execute, QString("Executing ").append(lmao).append(QString(fast_flag?" -f ":" ")).append(this->hell_file_name).append("...\n"));
}


void MainWindow::on_actionRun_triggered() {

    //if (this->ui->textEdit->SendScintilla(QsciScintilla::SCI_GETMODIFY)!=0) {
    this->on_actionSave_triggered();
    //}

    if (this->ui->textEdit->SendScintilla(QsciScintilla::SCI_GETMODIFY)!=0 || this->hell_file_name.length() <= 0 || this->hell_filename_is_dir)
        return;

    QIOWorker* execute;
    QString exec_message;

    // TODO: check if assembling is neccessary. if not, skip it!
    if (true || this->last_malbolge_file.length()<=0 || !QFile(last_malbolge_file).exists()) {

        QSettings application_settings;
        QString lmao = application_settings.value("assembler/command").toString();
        if (lmao.length() <= 0)
            lmao = "lmao";

        int set_fast_flag = application_settings.value("assembler/fastflag", FAST_FLAG_REASONABLE).toInt();
        bool fast_flag = (set_fast_flag != FAST_FLAG_NEVER);

        QStringList lmao_params;
        if (fast_flag) {
            lmao_params << "-f";
        }
        lmao_params << this->hell_file_name;

        execute = new QMalbolgeRunner(lmao, lmao_params);
        connect(execute, SIGNAL(AssemblyOutput(QString)), this, SLOT(get_assembled_filename(QString)));
        exec_message = QString("Executing ").append(lmao).append(QString(fast_flag?" -f ":" ")).append(this->hell_file_name).append("...\n");
    }else{
        execute = new QMalbolgeRunner(QString(), QStringList());
        ((QMalbolgeRunner*)execute)->setMalbolgeFileName(last_malbolge_file);
    }
    this->ui->console->execute(execute, exec_message);
}


void MainWindow::on_actionStop_triggered() {
    this->ui->console->stop();
}


void MainWindow::on_actionStart_triggered() {
    //if (this->ui->textEdit->SendScintilla(QsciScintilla::SCI_GETMODIFY)!=0) {
    this->on_actionSave_triggered();
    //}

    if (this->ui->textEdit->SendScintilla(QsciScintilla::SCI_GETMODIFY)!=0 || this->hell_file_name.length() <= 0 || this->hell_filename_is_dir)
        return;

    QIOWorker* execute;
    QString exec_message;

    // TODO: check if assembling is neccessary. if not, skip it!
    if (true || this->last_malbolge_file.length()<=0 || !QFile(last_malbolge_file).exists()) { // && debug_informations exist

        QSettings application_settings;
        QString lmao = application_settings.value("assembler/command").toString();
        if (lmao.length() <= 0)
            lmao = "lmao";

        int set_fast_flag = application_settings.value("assembler/fastflag", FAST_FLAG_REASONABLE).toInt();
        bool fast_flag = (set_fast_flag != FAST_FLAG_NEVER);

        QStringList lmao_params;
        lmao_params << "-d";
        if (fast_flag) {
            lmao_params << "-f";
        }
        lmao_params << this->hell_file_name;

        execute = new QMalbolgeDebugger(lmao, lmao_params);
        connect(execute, SIGNAL(AssemblyOutput(QString)), this, SLOT(get_assembled_filename(QString)));
        exec_message = QString("Executing ").append(lmao).append(" -d").append(QString(fast_flag?" -f ":" ")).append(this->hell_file_name).append("...\n");
    }else{
        execute = new QMalbolgeDebugger(QString(), QStringList());
        ((QMalbolgeDebugger*)execute)->setMalbolgeFileName(last_malbolge_file);
        ((QMalbolgeDebugger*)execute)->setDebugFileName(QString()); // TODO
    }
    connect((QMalbolgeDebugger*)execute, SIGNAL(execution_paused(int, int, int, int, int, int, int, int, bool)), this, SLOT(debugging_paused(int, int, int, int, int, int, int, int, bool)));
    connect((QMalbolgeDebugger*)execute, SIGNAL(registers_changed(QString, QString, QString, QString)),this, SLOT(debugging_registers_changed(QString, QString, QString, QString)));
    connect((QMalbolgeDebugger*)execute, SIGNAL(execution_continued()), this, SLOT(debugging_continued()));
    connect(this, SIGNAL(debug_step()),(QMalbolgeDebugger*)execute, SLOT(step()));
    connect(this, SIGNAL(debug_pause()),(QMalbolgeDebugger*)execute, SLOT(pause()));
    connect(this, SIGNAL(debug_continue()),(QMalbolgeDebugger*)execute, SLOT(run_until_breakpoint()));
    connect(this, SIGNAL(debug_reset()),(QMalbolgeDebugger*)execute, SLOT(reset()));
    connect(this, SIGNAL(add_breakpoint(int)),(QMalbolgeDebugger*)execute, SLOT(add_breakpoint(int)));
    connect(this, SIGNAL(remove_breakpoint(int)),(QMalbolgeDebugger*)execute, SLOT(remove_breakpoint(int)));
    connect((QMalbolgeDebugger*)execute, SIGNAL(query_breakpoints()),this, SLOT(query_breakpoints()));
    connect((QMalbolgeDebugger*)execute, SIGNAL(runtime_expression_value_changed(QString, int)), this, SLOT(debugging_runtime_expression_value_changed(QString, int)));
    connect(this, SIGNAL(register_runtime_expression(QString, int)),(QMalbolgeDebugger*)execute, SLOT(register_runtime_expression(QString, int)));
    connect(this, SIGNAL(remove_runtime_expression(int)),(QMalbolgeDebugger*)execute, SLOT(remove_runtime_expression(int)));
    connect(this, SIGNAL(clear_runtime_expressions()),(QMalbolgeDebugger*)execute, SLOT(clear_runtime_expressions()));
    connect(this, SIGNAL(clear_runtime_expressions()),(QMalbolgeDebugger*)execute, SLOT(clear_runtime_expressions()));
    connect((QMalbolgeDebugger*)execute, SIGNAL(active_xlat2_changed(QLinkedList<LMAODebugInformations::SourcePosition>)), (QsciLexerHeLL*)ui->textEdit->lexer(), SLOT(update_active_xlat2(QLinkedList<LMAODebugInformations::SourcePosition>)));
    connect((QMalbolgeDebugger*)execute, SIGNAL(execution_continued()), (QsciLexerHeLL*)ui->textEdit->lexer(), SLOT(remove_active_xlat2()));
    connect((QMalbolgeDebugger*)execute, SIGNAL(Terminated(int,QProcess::ExitStatus)), (QsciLexerHeLL*)ui->textEdit->lexer(), SLOT(remove_active_xlat2()));
    this->ui->console->execute(execute, exec_message);
}

void MainWindow::query_breakpoints() {
    int marker = this->ui->textEdit->markerFindNext(0, 1);
    while (marker >= 0) {
        emit add_breakpoint(marker+1); // emit, because process must be prepared (starts processing messages) before breakpoints are submitted.
        marker = this->ui->textEdit->markerFindNext(marker+1, 1);
    }

    emit clear_runtime_expressions();

    for (int i=0;i<this->ui->tableWidget->rowCount();i++) {
        QTableWidgetItem* item = this->ui->tableWidget->item(i,0);
        if (item != 0) {
            if (!item->text().isEmpty()) {
                emit register_runtime_expression(item->text(), i);
            }
        }
    }
}


void MainWindow::on_actionContinue_triggered() {
    emit debug_continue();
}


void MainWindow::on_actionStep_triggered() {
    //qDebug() << "Emit debugging step.";
    emit debug_step();
}


void MainWindow::on_actionPause_triggered() {
    emit debug_pause();
}


void MainWindow::on_actionReset_triggered() {
    emit debug_reset();
}


void MainWindow::on_actionStop_2_triggered() {
    this->on_actionStop_triggered();
}


void MainWindow::on_actionToggle_Breakpoint_triggered() {
    int line = -1;
    int tmp;

    this->ui->textEdit->getCursorPosition(&line, &tmp);
    int marker = this->ui->textEdit->markerFindNext(line, 1);
    if (marker == line) {
        do {
            this->ui->textEdit->markerDelete(line,0);
            marker = this->ui->textEdit->markerFindNext(line, 1);
        }while(marker == line);
        emit remove_breakpoint(line+1);
    }
    else {
        this->ui->textEdit->markerAdd(line,0);
        emit add_breakpoint(line+1);
    }
}

void MainWindow::on_actionUndo_triggered() {
    this->ui->textEdit->undo();
}
void MainWindow::on_actionRedo_triggered() {
    this->ui->textEdit->redo();
}
void MainWindow::on_actionCopy_triggered() {
    if (this->ui->console->hasFocus()) {
        this->ui->console->copy();
    }else{
        this->ui->textEdit->copy();
    }
}
void MainWindow::on_actionPaste_triggered() {
    if (this->ui->console->hasFocus()) {
        this->ui->console->paste();
    }else{
        this->ui->textEdit->paste();
    }
}
void MainWindow::on_actionCut_triggered() {
    if (this->ui->console->hasFocus()) {
        this->ui->console->cut();
    }else{
        this->ui->textEdit->cut();
    }
}
void MainWindow::on_actionDelete_triggered() {
    if (this->ui->console->hasFocus()) {
        this->ui->console->remove();
    }else{
        this->ui->textEdit->removeSelectedText();
    }
}
void MainWindow::on_actionSelect_all_triggered() {
    if (this->ui->console->hasFocus()) {
        this->ui->console->selectAll();
    }else{
        this->ui->textEdit->selectAll();
    }
}

void MainWindow::get_assembled_filename(QString filename) {
    this->last_malbolge_file = filename;
}

MainWindow::~MainWindow()
{
    delete ui;
    delete this->findDialog;
    delete this->replaceDialog;
    QsciLexer* lexer = ui->textEdit->lexer();
    ui->textEdit->setLexer(0);
    delete lexer;
}

void MainWindow::on_console_startExecute() {
    this->ui->actionAssemble->setEnabled(false);
    this->ui->actionRun->setEnabled(false);
    this->ui->actionStop->setEnabled(true);
    this->ui->actionStart->setEnabled(false);
    this->ui->console->setFocus();
}

void MainWindow::on_console_stopExecute() {
    this->ui->actionAssemble->setEnabled(true);
    this->ui->actionRun->setEnabled(true);
    this->ui->actionStop->setEnabled(false);
    this->ui->actionStop_2->setEnabled(false);
    this->ui->actionReset->setEnabled(false);
    this->ui->actionContinue->setEnabled(false);
    this->ui->actionPause->setEnabled(false);
    this->ui->actionStart->setEnabled(true);
    this->ui->actionStep->setEnabled(false);
    this->ui->c_reg_val->setText(QString("unknown"));
    this->ui->c_mem_val->setText(QString("unknown"));
    this->ui->d_mem_val->setText(QString("unknown"));
    this->ui->a_reg_val->setText(QString("unknown"));
    this->ui->textEdit->markerDeleteAll(1);
    this->ui->textEdit->markerDeleteAll(2);
    this->ui->textEdit->setFocus();
    this->ui->verticalFrame->setVisible(false);
}

void MainWindow::debugging_continued() {
    this->ui->actionStep->setEnabled(false);
    this->ui->actionContinue->setEnabled(false);
    this->ui->actionPause->setEnabled(true);

    this->ui->textEdit->markerDeleteAll(1);
    this->ui->textEdit->markerDeleteAll(2);
}

void MainWindow::debugging_paused(int first_line_d, int first_column_d, int last_line_d, int last_column_d, int first_line_c, int first_column_c, int last_line_c, int last_column_c, bool recommend_displaying_code_section) {

    this->ui->actionPause->setEnabled(false);
    this->ui->actionStep->setEnabled(true);
    this->ui->actionStop_2->setEnabled(true);
    this->ui->actionReset->setEnabled(true);
    this->ui->actionContinue->setEnabled(true);
    this->ui->verticalFrame->setVisible(true);

    this->ui->textEdit->markerDeleteAll(1);
    this->ui->textEdit->markerDeleteAll(2);

    if (first_line_d >= 0 && first_column_d >= 0) {
        this->ui->textEdit->markerAdd(first_line_d-1,1);
        if (!recommend_displaying_code_section) {
            this->gotoHeLLCodePosition(first_line_d, first_column_d, last_line_d, last_column_d);
        }
    }

    if (first_line_c >= 0 && first_column_c >= 0) {
        this->ui->textEdit->markerAdd(first_line_c-1,2);
        if (recommend_displaying_code_section) {
            this->gotoHeLLCodePosition(first_line_c, first_column_c, last_line_c, last_column_c);
        }
    }
}

void MainWindow::debugging_registers_changed(QString c_label, QString mem_c_xlat, QString mem_d, QString a_reg_val) {
    this->ui->c_reg_val->setText(c_label);
    this->ui->c_mem_val->setText(mem_c_xlat);
    this->ui->d_mem_val->setText(mem_d);
    this->ui->a_reg_val->setText(a_reg_val);
}

void MainWindow::on_textEdit_cursorPositionChanged(int line, int index) {
    //int line_state = 0;
    //if (line >= 0) {
    //    line_state = this->ui->textEdit->SendScintilla(QsciScintilla::SCI_GETLINESTATE, line);
    //}
    //QString msg = QString("Line: %1; Column: %2; State: %3").arg(line+1).arg(index+1).arg(line_state);
    QString msg = QString("Line: %1; Column: %2").arg(line+1).arg(index+1);
    //QMessageBox::information(0, "", msg);
    this->statusbar_position.setText(msg);

}

void MainWindow::find(QString text, bool case_sensitive, bool backwards) {
    this->_find(text, case_sensitive, backwards, this->findDialog);
}


bool MainWindow::_find(QString text, bool case_sensitive, bool backwards, QWidget* trigger, bool wrap_around, bool show_error_msg) {
    if (!ui->textEdit->findFirst(text, false, case_sensitive, false, wrap_around, !backwards)) {
        searched = false;
        if (show_error_msg) {
            QMessageBox::information(trigger ,QString("Not found"),QString("The search string has not been found."));
        }
        return false;
    } else {
        int lineFrom, indexFrom, lineTo, indexTo;
        ui->textEdit->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
        if (lineFrom >= 0) {
            if (((lineFrom < lineTo || (lineFrom == lineTo && indexFrom < indexTo)) && backwards)
                    || ((lineFrom > lineTo || (lineFrom == lineTo && indexFrom > indexTo)) && !backwards))
                ui->textEdit->setSelection(lineTo, indexTo, lineFrom, indexFrom);
        }

        searched = true;
        //this->findDialog->hide();
    }
    return true;
}


void MainWindow::replace(QString text, QString replaceString, bool case_sensitive, bool backwards, bool all) {
    if (text.isEmpty())
        return;
    if (all) {
        this->ui->textEdit->setCursorPosition(0,0);
    }
    //bool replaced = false;
    do {
        //replaced = false;
        int selection_length = ui->textEdit->SendScintilla(QsciScintilla::SCI_GETSELTEXT);
        if (selection_length == 0) {
            return; // ERROR
        }
        //qDebug() << "Sel-Len: " << selection_length;
        char* selection = new char[selection_length];
        if (selection == 0) {
            return; // ERROR
        }
        if (selection_length > 1) {
            ui->textEdit->SendScintilla(QsciScintilla::SCI_GETSELTEXT, (unsigned long)0, (void*)selection);
        }
        selection[selection_length-1] = 0;
        QString source;
        if (ui->textEdit->isUtf8()) {
            source = QString::fromUtf8(selection);
        }else{
            source = QString::fromLatin1(selection);
        }
        delete[] selection;
        if (source.compare(text, case_sensitive?Qt::CaseSensitive:Qt::CaseInsensitive) == 0) {
            ui->textEdit->replace(replaceString);
            //replaced = true;
        } //else {
            //qDebug() << "Did not replace " << source << " instead of " << text << " by " << replaceString;
        //}
        if (!this->_find(text, case_sensitive, backwards && !all, this->replaceDialog, !all, !all))
            break;
    } while (all);
}

void MainWindow::gotoHeLLCodePosition(int line, int column) {
    if (line <= 0)
        line = 1;
    if (column <= 0)
        column = 1;
    int index_line_start = this->ui->textEdit->positionFromLineIndex(line-1,0);
    int index = index_line_start + column - 1;
    this->ui->textEdit->SendScintilla(QsciScintilla::SCI_ENSUREVISIBLE, line-1);
    this->ui->textEdit->lineIndexFromPosition(index, &line, &column);
    this->ui->textEdit->setCursorPosition(line, column);
    this->ui->textEdit->setFocus();
}


void MainWindow::gotoHeLLCodePosition(int first_line, int first_column, int last_line, int last_column) {
    if (first_line >= 0 && first_column >= 0) {
        this->gotoHeLLCodePosition(first_line, 0);
        if (last_line >= 0 && last_column >= 0) {
            if (last_line != first_line) {
                this->gotoHeLLCodePosition(last_line, 0);
            }
            this->gotoHeLLCodePosition(last_line, last_column+1);
            if (last_line != first_line) {
                // goto first_line, last column in that line
                // get last column in first line
                int line2_start = this->ui->textEdit->positionFromLineIndex(first_line,0);
                int fl, fllc = -1;
                this->ui->textEdit->lineIndexFromPosition(line2_start-1,&fl,&fllc);
                if (fl != first_line-1) {
                    qDebug() << "Error while determining last character in first line of Hell code destination.";
                }else{
                    this->gotoHeLLCodePosition(first_line, fllc);
                }
            }
        }
        this->gotoHeLLCodePosition(first_line, first_column);
    }
}

void MainWindow::on_tableWidget_itemChanged(QTableWidgetItem *item)
{
    if (item == 0)
        return;
    if (item->column() != 0)
        return;
    // TODO: inform debugger about changes: emit something...
    int rc = this->ui->tableWidget->rowCount();
    int itemrow = item->row();
    if (!item->text().isEmpty()) {
        this->ui->tableWidget->item(item->row(),1)->setText(QString("waiting for value..."));
        emit register_runtime_expression(item->text(), item->row());
    } else if (item->row() != rc-1) {
        emit remove_runtime_expression(item->row());
    }
    if (item->row() == rc-1) {
        if (!item->text().isEmpty()) {
            this->ui->tableWidget->insertRow(rc);
            QTableWidgetItem* item = new QTableWidgetItem();
            this->ui->tableWidget->setItem(rc,0,item);
            QTableWidgetItem* item2 = new QTableWidgetItem();
            item2->setFlags(item2->flags() ^ Qt::ItemIsEditable);
            this->ui->tableWidget->setItem(rc,1,item2);
            this->ui->tableWidget->clearSelection();
            this->ui->tableWidget->setCurrentIndex(this->ui->tableWidget->model()->index(rc,0));
            //this->ui->tableWidget->edit(this->ui->tableWidget->model()->index(rc,0));
        }
    } else if (item->row() != rc-1) {
         if (item->text().isEmpty()) {
             this->ui->tableWidget->removeRow(item->row());
             this->ui->tableWidget->clearSelection();
             this->ui->tableWidget->setCurrentIndex(this->ui->tableWidget->model()->index(itemrow,0));
         }else{
             this->ui->tableWidget->clearSelection();
             this->ui->tableWidget->setCurrentIndex(this->ui->tableWidget->model()->index(itemrow+1,0));
         }
    }
}


void MainWindow::debugging_runtime_expression_value_changed(QString new_value, int expression_id) {
    int rc = this->ui->tableWidget->rowCount();
    if (expression_id < rc) {
        QTableWidgetItem* item = this->ui->tableWidget->item(expression_id,1);
        if (item != 0) {
            qDebug() << "writing " << new_value << " to item " << expression_id << ",1";
            item->setText(new_value);
        }
    }
}
