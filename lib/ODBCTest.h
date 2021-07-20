#pragma once
#include <Windows.h>
#include <sqlext.h>
#include <vector>
#include <mbstring.h>
#include <string>


constexpr auto MYSQLSUCCESS(const RETCODE rc)
{
	return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO;
}

struct BINDING
{
	SQLSMALLINT cDisplaySize; /* size to display  */
	CHAR* szBuffer; /* display buffer   */
	SQLLEN indPtr; /* size or null     */
	struct BINDING* sNext; /* linked list      */
};

struct Parameter
{
	SQLCHAR val[64];
	SQLLEN paramLenOrInd;
};

class ODBCTest
{
	RETCODE rc; // ODBC return code
	HENV hEnv; // Environment
	HDBC hDbc; // Connection handle
	HSTMT hStmt; // Statement handle

	SQLCHAR chrDSN[1024]; // Data source name
	SQLCHAR SQLQuery[1024]; // SQL query
	SQLCHAR retConStr[1024]; // 

	std::vector<Parameter> parameters;


	void displayResults(SQLSMALLINT cCols) const;

	void allocateBindings(SQLSMALLINT cCols, BINDING** ppBinding) const;

	// Displays errors
	void showSQLError(SQLSMALLINT handleType, const SQLHANDLE& handle) const;

	void readINIFile();
public:
	ODBCTest();

	void readSQLFile(const std::string& filename);

	// Allocate environment, statement and connection
	void sqlConn();

	// Binding parameters and execute SQL statement
	void sqlExec();

	// Free pointers to env, stat, conn and disconnect
	void sqlDisconn() const;
};
