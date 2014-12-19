
/*
 * buddy manager - manager/update/cache the group/buddies information
 *
 * copyright 2013 microcai
 * copyright 2012 - 2013 avplayer.org
 */

#pragma once

#include <boost/asio.hpp>

#include <soci-sqlite3.h>
#include <boost-optional.h>
#include <boost-tuple.h>
#include <boost-gregorian-date.h>
#include <soci.h>

#include "libwebqq/webqq.hpp"

namespace webqq {
namespace qqimpl {
namespace detail {

} // nsmespace detail

class buddy_mgr
{
public:
	buddy_mgr(std::string dbname  = ":memory:")
	{
		m_sql.open(soci::sqlite3, dbname);
		db_initialize();
	}

	void update_group_list(std::string gid, std::string name, std::string code);

	bool group_has_qqnum(std::string code);
	void map_group_qqnum(std::string code, std::string qqnum);

	qqGroup_ptr get_group_by_gid(std::string gid);
	qqGroup_ptr get_group_by_qq(std::string qqnum);
	std::vector< qqBuddy_ptr > get_buddies();

	std::vector<std::string> get_group_all_buddies_uin(std::string gid);

	qqBuddy_ptr get_buddy_by_uin(std::string uid);

	bool buddy_has_qqnum(std::string uid);
	void map_buddy_qqnum(std::string uid, std::string qqnum);

	void set_group_owner(std::string gid, std::string owner);

	void group_new_buddy(std::string gid, std::string uid, std::string qqnum, std::string nick);

	void group_buddy_update_mflag(std::string uid,  unsigned int mflag);
	void group_buddy_update_card(std::string uid, std::string card);

	void new_catgory(int index, int sort, std::string name);
	void new_buddy(std::string uid, int flag, int category);

	void buddy_update_markname(std::string uid,  std::string markname);
	void buddy_update_nick(std::string uid,  std::string nickname);

	void clean_out_outdated();
private:

	void db_initialize();

private:
	soci::session	m_sql;

};


} // nsmespace qqimpl
} // namespace webqq