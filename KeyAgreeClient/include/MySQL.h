#pragma once
#include "mysql/mysql.h"
#include <iostream>
using namespace std;

class MySQL {
public:
	MySQL();
	~MySQL();

	bool InitSQL(const char* dbHost, const char* dbUser, const char* dbPasswd, const char* dbName);
	MYSQL_RES* GetSqlRes(const string& Sql);
	bool DoSql(const string& Sql);
private:
	MYSQL* myConnect;
	MYSQL_RES* result;
};