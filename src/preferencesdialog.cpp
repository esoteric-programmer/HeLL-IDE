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

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include <QLayout>
#include <QFile>
#include <QFileDialog>
#include "properties.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    layout()->setSizeConstraint(QLayout::SetFixedSize);

    ui->lmaoCommand->setText(application_settings.value("assembler/command", QString()).toString());
    if (ui->lmaoCommand->text().length() <= 0)
        ui->lmaoCommand->setText("lmao");

    int set_fast_flag = application_settings.value("assembler/fastflag", FAST_FLAG_REASONABLE).toInt();
    switch (set_fast_flag) {
        case FAST_FLAG_ALWAYS:
            ui->fastFlagAlways->setChecked(true);
            break;
        case FAST_FLAG_NEVER:
            ui->fastFlagNever->setChecked(true);
            break;
        default:
            ui->fastFlagReasonable->setChecked(true);
            break;
    }
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::accept() {
    apply();
    QDialog::accept();
}

void PreferencesDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if (ui->buttonBox->standardButton(button) == QDialogButtonBox::Apply){
        apply();
    }
}

void PreferencesDialog::apply() {
    application_settings.setValue("assembler/command",ui->lmaoCommand->text());

    int fast_flag = FAST_FLAG_REASONABLE;
    if (ui->fastFlagReasonable->isChecked()) {

    } else if (ui->fastFlagAlways->isChecked()) {
        fast_flag = FAST_FLAG_ALWAYS;
    }else if (ui->fastFlagNever->isChecked()) {
        fast_flag = FAST_FLAG_NEVER;
    }
    application_settings.setValue("assembler/fastflag",fast_flag);
    application_settings.sync();
}

void PreferencesDialog::on_browseLmaoCommand_clicked()
{

    QString defaultFilename = (QFile(ui->lmaoCommand->text()).exists()?ui->lmaoCommand->text():QString());

    QString filename = QFileDialog::getOpenFileName(this, QString(), defaultFilename, "All files (*)", 0, 0);
    if (filename.length() > 0) {
        ui->lmaoCommand->setText(filename);
    }
}
