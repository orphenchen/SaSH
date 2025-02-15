﻿#include "stdafx.h"
#include "settingfiledialog.h"

#include "util.h"

settingfiledialog::settingfiledialog(const QString& defaultName, QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	setFont(util::getFont());
	util::setWidget(this);
	setWindowFlags(Qt::Tool | Qt::Dialog | Qt::WindowCloseButtonHint);
	setModal(true);
	setAttribute(Qt::WA_QuitOnClose);
	setWindowTitle("");
	setAttribute(Qt::WA_StyledBackground);


	QStringList list;
	util::searchFiles(util::applicationDirPath() + "/settings", "", ".json", &list, false);

	if (list.isEmpty())
		return;

	if (!defaultName.isEmpty())
	{
		QFileInfo info(defaultName);
		firstItem_ = q_check_ptr(new QListWidgetItem(info.fileName()));
		sash_assume(firstItem_ != nullptr);
		if (firstItem_ != nullptr)
		{
			firstItem_->setData(Qt::UserRole, QString("custom"));
			firstItem_->setForeground(QColor("#BD5F5F"));
			ui.listWidget->addItem(firstItem_);
			lineEdit_ = q_check_ptr(new QLineEdit(info.fileName()));
			sash_assume(lineEdit_ != nullptr);
			if (lineEdit_ != nullptr)
			{
				util::setLineEdit(lineEdit_);
				connect(lineEdit_, &QLineEdit::textChanged, this, &settingfiledialog::onLineEditTextChanged);
				this->layout()->addWidget(lineEdit_);
			}

		}
	}

	for (const QString& str : list)
	{
		if (str.contains("system.json"))
			continue;

		QFileInfo info(str);

		QListWidgetItem* item = q_check_ptr(new QListWidgetItem(info.fileName()));
		sash_assume(item != nullptr);
		if (item == nullptr)
			continue;

		item->setData(Qt::UserRole, str);
		ui.listWidget->addItem(item);
	}

	QRect parentRect = parent->geometry();
	QPoint centerPos = parentRect.center();
	QSize thisSize = size();
	QPoint newPos = centerPos - QPoint(thisSize.width() / 2, thisSize.height() / 2);
	util::FormSettingManager formSettingManager(this);
	formSettingManager.loadSettings();
	move(newPos);
}

settingfiledialog::~settingfiledialog()
{
	util::FormSettingManager formSettingManager(this);
	formSettingManager.saveSettings();
}

void settingfiledialog::onLineEditTextChanged(const QString& text)
{
	if (firstItem_ == nullptr)
		return;

	firstItem_->setText(text);
}

void settingfiledialog::on_listWidget_itemDoubleClicked(QListWidgetItem* item)
{
	if (item == nullptr)
		return;

	QString text = item->data(Qt::UserRole).toString();
	if (text.isEmpty())
		return;

	if (text == "custom")
	{
		text = lineEdit_->text();
		if (text.isEmpty())
			return;

		text = util::applicationDirPath() + "/settings/" + text;
		if (!text.endsWith(".json"))
			text += ".json";
	}

	returnValue = text;

	accept();
}
