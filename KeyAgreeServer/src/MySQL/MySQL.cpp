#include "MySQL.h"

MySQL::MySQL():myConnect(nullptr), result(nullptr)
{

}

MySQL::~MySQL()
{
	mysql_free_result(result);
	mysql_close(myConnect);//¶Ï¿ªÁ¬½Ó
}

bool MySQL::InitSQL(const char* dbHost, const char* dbUser, const char* dbPasswd, const char* dbName)
{
	//1.init
	myConnect = mysql_init(nullptr);
	if (myConnect == nullptr) {
		cout << "init err..." << endl;
	}

	//2.real_connect
	if (mysql_real_connect(myConnect, dbHost, dbUser, dbPasswd, dbName, 0, nullptr, 0))
	{
		cout << "  database connect success..." << endl;
		return true;
	}
	else
	{
		cout << "  database conect err..." << endl;
		return false;
	}
}

MYSQL_RES* MySQL::GetSqlRes(const string& Sql)
{
	if (mysql_real_query(myConnect, Sql.c_str(), Sql.size())) {
		cout << "  mysql_real_query() err..." << endl;
		return nullptr;
	}
	
	if (result != nullptr) {
		mysql_free_result(result);
		result = nullptr;
	}

	result = mysql_store_result(myConnect);
	if (result == nullptr) {
		cout << "  mysql_store_result() err..." << endl;
		return nullptr;
	}

	return result;
}

bool MySQL::DoSql(const string& Sql)
{
	if (mysql_real_query(myConnect, Sql.c_str(), Sql.size())) {
		cout << "  mysql_real_query() err..." << endl;
		return false;
	}
	return true;
}
