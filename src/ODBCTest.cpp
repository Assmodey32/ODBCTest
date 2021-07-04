#include "../lib/ODBCTest.h"
#include <fstream>
#include <iostream>
#include <string>


ODBCTest::ODBCTest()
{
	hDbc = hEnv = hStmt = nullptr;
	rc = SQL_ERROR;

	/*
	 *	This code is for debugging purposes only.
	 *	The actual code should read this data from the file.
	 */

	// Initialize Data source name string
	_mbscpy_s(chrDSN, 1024,
	          (const unsigned char*)
	          "DRIVER={SQL Server}; SERVER=localhost, 1433; DATABASE=lab11; UID=user; PWD=password");
}


void ODBCTest::sqlConn()
{
	// Allocate an environment
	rc = SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &hEnv);
	//showSQLError()

	// Register this as an application that expects 3.x behavior,
	// you must register something if you use AllocHandle
	rc = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

	// Allocate a connection
	rc = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

	rc = SQLSetConnectAttr(hDbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

	// Connect to the driver.
	rc = SQLDriverConnect(hDbc, nullptr, chrDSN, SQL_NTS, retConStr, 1024, nullptr, SQL_DRIVER_NOPROMPT);

	if (!MYSQLSUCCESS(rc))
	{
		showSQLError(SQL_HANDLE_DBC, hDbc);

		SQLFreeConnect(hEnv);
		SQLFreeEnv(hEnv);
		SQLFreeConnect(hDbc);
	}

	rc = SQLAllocStmt(hDbc, &hStmt);
}

void ODBCTest::sqlExec()
{
	// Preparing a query for execution
	rc = SQLPrepare(hStmt, SQLQuery, SQL_NTS);
	if (!MYSQLSUCCESS(rc))
	{
		showSQLError(SQL_HANDLE_STMT, hStmt);
	}
	SQLSMALLINT cParams = 0;

	// The number of parameters is assigned to the cParams 
	rc = SQLNumParams(hStmt, &cParams);
	if (!MYSQLSUCCESS(rc))
	{
		showSQLError(SQL_HANDLE_STMT, hStmt);
	}

	for (SQLSMALLINT i = 1; i <= cParams; ++i)
	{
		rc = SQLBindParameter(hStmt, i, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
		                      63, 0, parameters[i - 1].val, sizeof(parameters[i - 1].val),
		                      &parameters[i - 1].paramLenOrInd);
		if (!MYSQLSUCCESS(rc))
		{
			showSQLError(SQL_HANDLE_STMT, hStmt);
		}
	}

	rc = SQLExecDirect(hStmt, SQLQuery, SQL_NTS);
	if (!MYSQLSUCCESS(rc))
	{
		showSQLError(SQL_HANDLE_STMT, hStmt);
	}
	else
	{
		SQLSMALLINT sNumResults;
		SQLNumResultCols(hStmt, &sNumResults);

		if (sNumResults > 0)
			displayResults(sNumResults);
	}
}

void ODBCTest::sqlDisconn() const
{
	SQLFreeStmt(hStmt, SQL_DROP);
	SQLDisconnect(hDbc);
	SQLFreeConnect(hDbc);
	SQLFreeEnv(hEnv);
}

void ODBCTest::showSQLError(SQLSMALLINT handleType, const SQLHANDLE& handle) const
{
	SQLCHAR SQLState[1024];
	SQLCHAR message[1024];

	if (SQLGetDiagRec(handleType, handle, 1, SQLState, nullptr, message, 1024, nullptr) == SQL_SUCCESS)
	{
		std::cout << "SQL driver message: " << message <<
			"\nSQL state: " << SQLState << ".\n";
	}

	sqlDisconn();
	exit(SQL_ERROR);
}

void ODBCTest::readSQLFile(const std::string& filename)
{
	std::ifstream input(filename);

	std::string line;

	std::string sqlQuery;

	if (input.is_open())
	{
		std::cout << "\nReading data from file " << filename << ":\n";
		// reading sqlQuery
		while (std::getline(input, line))
		{
			if (!line.empty())
				sqlQuery += line + ' ';
			else
				break;
		}

		std::cout << "\nSQL query:\n" << sqlQuery << '\n';

		_mbscpy_s(SQLQuery, 1024, (const unsigned char*)sqlQuery.c_str());


		std::cout << "\nParameters:\n";
		// reading list of parameters
		while (std::getline(input, line, '\n'))
		{
			std::cout << line << '\n';

			Parameter p;
			_mbscpy_s(p.val, 64, (const unsigned char*)line.c_str());
			p.paramLenOrInd = SQL_NTS;
			parameters.emplace_back(p);
		}
		std::cout << "\n";
		input.close();
	}
	else
	{
		std::cout << "Couldn't open file!\n";
	}
}

void ODBCTest::displayResults(SQLSMALLINT cCols) const
{
	BINDING* pFirstBinding;

	// Allocate memory fo each column
	allocateBindings(cCols, &pFirstBinding);

	RETCODE RetCode;

	std::ofstream output("output.txt");

	if (output.is_open())
	{
		std::cout << "Result:\n";

		bool fNoData = false;
		do
		{
			// Fetching data row by row
			RetCode = SQLFetch(hStmt);
			if (RetCode == SQL_NO_DATA_FOUND)
				fNoData = true;
			else
			{
				for (auto* pThisBinding = pFirstBinding;
				     pThisBinding != nullptr;
				     pThisBinding = pThisBinding->sNext)
				{
					if (pThisBinding->indPtr != SQL_NULL_DATA)
					{
						std::cout << pThisBinding->szBuffer;
						output << pThisBinding->szBuffer;
					}
					else
					{
						std::cout << "<NULL>";
						output << "<NULL>";
					}

					if (pThisBinding->sNext)
					{
						std::cout << ", ";
						output << ", ";
					}
					else
					{
						std::cout << '\n';
						output << '\n';
					}
				}
			}
		}
		while (!fNoData);
	}
	else
	{
		std::cout << "Couldn't open output file!\n";
	}

	// Clearing data bindings
	BINDING* tempBinding;
	for (auto* pThisBinding = pFirstBinding;
	     pThisBinding != nullptr;
	     pThisBinding = tempBinding)
	{
		free(pThisBinding->szBuffer);
		tempBinding = pThisBinding->sNext;
		free(pThisBinding);
	}
}

void ODBCTest::allocateBindings(SQLSMALLINT cCols, BINDING** ppBinding) const
{
	BINDING* pLastBinding = nullptr;
	SQLLEN cchDisplay;

	for (SQLSMALLINT iCol = 1; iCol <= cCols; ++iCol)
	{
		auto* pThisBinding = static_cast<BINDING*>(malloc(sizeof(BINDING)));
		if (pThisBinding == nullptr)
		{
			std::cerr << "Out of memory!\n";
			exit(SQL_ERROR);
		}

		if (iCol == 1)
		{
			*ppBinding = pThisBinding;
		}
		else
		{
			pLastBinding->sNext = pThisBinding;
		}
		pLastBinding = pThisBinding;

		SQLColAttribute(hStmt,
		                iCol,
		                SQL_DESC_DISPLAY_SIZE,
		                nullptr,
		                0,
		                nullptr,
		                &cchDisplay);

		pThisBinding->sNext = nullptr;

		// Allocate a buffer big enough to hold the text representation
		// of the data.  Add one character for the null terminator('\0').
		pThisBinding->szBuffer = static_cast<CHAR*>(malloc((cchDisplay + 1) * sizeof(CHAR)));

		if (!(pThisBinding->szBuffer))
		{
			std::cerr << "Out of memory!\n";
			exit(SQL_ERROR);
		}

		SQLBindCol(hStmt,
		           iCol,
		           SQL_C_CHAR,
		           pThisBinding->szBuffer,
		           (cchDisplay + 1) * sizeof(CHAR),
		           &pThisBinding->indPtr);
	}
}
