#pragma once
#include <Windows.h>
#include <sqlext.h>
#include <iostream>
#include <vector>
#include <mbstring.h> 

constexpr auto MYSQLSUCCESS(const RETCODE rc) {
	return rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO;
}

struct BINDING {
	SQLSMALLINT cDisplaySize;	/* size to display  */
	CHAR* szBuffer;				/* display buffer   */
	SQLLEN indPtr;              /* size or null     */
	BOOL fChar;                 /* character col?   */
	struct BINDING* sNext;		/* linked list      */
};

struct Parameter
{
	SQLCHAR val[64];
	SQLINTEGER paramLenOrInd;
};

/*
 *										TO-DO List:
 *										
 *		1.	Write a function that reads the .ini file and initializes the chrDSN string.
 *			The chrDSN string is very picky, one extra space will not make the connection.
 *			
 *			Here is an example:
 *			"DRIVER={SQL Server}; SERVER=localhost, 1433; DATABASE=lab11; UID=user; PWD=password"
 *			
 *			But when using the following line, the connection will not be established:
 *			"DRIVER={SQL Server}; SERVER=localhost, 1433; DATABASE=lab11; UID=user; PWD=password"
 *			
 *		2.	Write a function that queries the SQL query and parameters from a file
 *			and initializes the SQL query and parameters accordingly
 *			
 *		3.	Rewrite displayResults so that it outputs data to a file instead of the console
 *
 *			At the moment a stub for the first two elements in the constructor has been written
 *		------------------------------------------------------------------------------------------------
 *		
 *		4.	Understand how to clear memory after the allocateBindings function works
 *
 *		5.	Understand how to use FreeHandle and FreeConnection correctly in this spaghetti code
 */

class ODBCTest
{
	RETCODE rc;	// ODBC return code
	HENV hEnv;	// Environment
	HDBC hDbc;	// Connection handle
	HSTMT hStmt;// Statement handle

	SQLCHAR chrDSN[1024];	// Data source name
	SQLCHAR SQLQuery[1024];	// SQL query
	SQLCHAR retConStr[1024];// 

	std::vector<Parameter> parameters;


	void displayResults(SQLSMALLINT cCols) const;

	void allocateBindings(SQLSMALLINT cCols, BINDING** ppBinding) const;

	//void bindParameters(HSTMT hStmt);
	
	// Displays errors
	void showSQLError(SQLSMALLINT handleType, const SQLHANDLE& handle);

public:
	ODBCTest();

	// Allocate environment, statement and connection
	void sqlConn();

	// Binding parameters and execute SQL statement
	void sqlExec();

	// Free pointers to env, stat, conn and disconnect
	void sqlDisconn() const;


};

