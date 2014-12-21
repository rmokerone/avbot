#include "dialog.hpp"
#include "ui_dialog.h"
#include <QString>
#include <iostream>

Dialog::Dialog(
		std::string &qqnum, std::string &qqpwd,
		std::string &ircnick, std::string &ircroom, std::string &ircpwd,
		std::string &xmppuser, std::string &xmppserver, std::string &xmpppwd, std::string &xmpproom, std::string &xmppnick)
	: QDialog(NULL)
	, ui(new Ui::Dialog)
	, qqnum(qqnum)
	, qqpwd(qqpwd)
	, ircnick(ircnick)
	, ircroom(ircroom)
	, ircpwd(ircpwd)
	, xmppnick(xmppnick)
	, xmpppwd(xmpppwd)
	, xmpproom(xmpproom)
	, xmppserver(xmppserver)
{
	ui->setupUi(this);
}

Dialog::~Dialog()
{
#define ASSIGN(field) (field) = ui->field->text().toStdString()
	ASSIGN(qqnum);
	ASSIGN(qqpwd);
	if(ui->enable_irc->checkState() == Qt::Checked)
	{
		ASSIGN(ircnick);
		ASSIGN(ircroom);
		ASSIGN(ircpwd);
	}
	if(ui->enable_xmpp->checkState() == Qt::Checked)
	{
		ASSIGN(xmppnick);
		ASSIGN(xmpppwd);
		ASSIGN(xmpproom);
		ASSIGN(xmppserver);
	}
	delete ui;
}


void Dialog::on_enable_irc_toggled(bool checked)
{
	for(int i = 0; i < ui->ircbox->count(); i++)
	{
		ui->ircbox->itemAt(i)->widget()->setEnabled(checked);
	}
}

void Dialog::on_enable_xmpp_toggled(bool checked)
{
	for(int i = 0; i < ui->xmppbox1->count(); i++)
	{
		ui->xmppbox1->itemAt(i)->widget()->setEnabled(checked);
	}
	for(int i = 0; i < ui->xmppbox2->count(); i++)
	{
		ui->xmppbox2->itemAt(i)->widget()->setEnabled(checked);
	}
}

void setup_dialog(
		std::string &qqnum, std::string &qqpwd,
		std::string &ircnick, std::string &ircroom, std::string &ircpwd,
		std::string &xmppuser, std::string &xmppserver, std::string &xmpppwd, std::string &xmpproom, std::string &xmppnick)
{
	int argc = 1;
	char* argv[] = { "avbot", NULL };
	QApplication app(argc, argv);
	Dialog d(qqnum, qqpwd, ircnick, ircroom, ircpwd, xmppuser, xmppserver, xmpppwd, xmpproom, xmppnick);
	d.setWindowTitle(QString::fromUtf8("编辑qqbotrc"));
	d.show();
	app.exec();
}


