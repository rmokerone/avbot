#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <string>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
	Q_OBJECT

public:
	Dialog(
		std::string &qqnum, std::string &qqpwd,
		std::string &ircnick, std::string &ircroom, std::string &ircpwd,
		std::string &xmppuser, std::string &xmppserver, std::string &xmpppwd, std::string &xmpproom, std::string &xmppnick);
	~Dialog();



private Q_SLOTS:
	void on_enable_irc_toggled(bool checked);

	void on_enable_xmpp_toggled(bool checked);

private:
	Ui::Dialog *ui;
	std::string& qqnum;
	std::string& qqpwd;
	std::string& ircnick;
	std::string& ircroom;
	std::string& ircpwd;
	std::string& xmppnick;
	std::string& xmpppwd;
	std::string& xmpproom;
	std::string& xmppserver;
};


#endif // DIALOG_H
