#include "../lib/ODBCTest.h"

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

	// Initialize SQLQuery, which will be executed later
	_mbscpy_s(SQLQuery, 1024, (const unsigned char*)"SELECT * FROM Contracts WHERE Id <= ? AND InsuranceAmount >= ?");

	// Add some parameters to SQLQuery
	Parameter p = { "10", SQL_NTS };

	parameters.emplace_back(p);
	p = { "5000", SQL_NTS };
	parameters.emplace_back(p);
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

void ODBCTest::showSQLError(SQLSMALLINT handleType, const SQLHANDLE& handle)
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

void ODBCTest::displayResults(SQLSMALLINT cCols) const
{
	BINDING* pFirstBinding;

	// Allocate memory fo each column
	allocateBindings(cCols, &pFirstBinding);
	RETCODE RetCode;

	bool fNoData = false;
	do
	{
		RetCode = SQLFetch(hStmt);
		if (RetCode == SQL_NO_DATA_FOUND)
			fNoData = true;
		else
		{
			for (BINDING* pThisBinding = pFirstBinding;
				pThisBinding != nullptr;
				pThisBinding = pThisBinding->sNext)
			{
				if (pThisBinding->indPtr != SQL_NULL_DATA)
				{
					std::cout << pThisBinding->szBuffer << ", ";
				}
				else
				{
					std::cout << "<NULL>" << ", ";
				}
			}
			std::cout << '\n';
		}
	} while (!fNoData);
}

void ODBCTest::allocateBindings(SQLSMALLINT cCols, BINDING** ppBinding) const
{
	BINDING* pLastBinding = nullptr;
	SQLLEN cchDisplay, ssType;

	for (SQLSMALLINT iCol = 1; iCol <= cCols; ++iCol)
	{
		auto* pThisBinding = static_cast<BINDING*>(malloc(sizeof(BINDING)));
		if (pThisBinding == nullptr)
		{
			std::cout << "Out of memory\n";
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

		SQLColAttribute(hStmt,
			iCol,
			SQL_DESC_CONCISE_TYPE,
			nullptr,
			0,
			nullptr,
			&ssType);

		pThisBinding->fChar = (ssType == SQL_CHAR ||
			ssType == SQL_VARCHAR ||
			ssType == SQL_LONGVARCHAR);

		pThisBinding->sNext = nullptr;

		// Allocate a buffer big enough to hold the text representation
		// of the data.  Add one character for the null terminator('\0').
		pThisBinding->szBuffer = static_cast<CHAR*>(malloc((cchDisplay + 1) * sizeof(CHAR)));

		if (!(pThisBinding->szBuffer))
		{
			fprintf(stderr, "Out of memory!\n");
			exit(-100);
		}

		SQLBindCol(hStmt,
			iCol,
			SQL_C_TCHAR,
			pThisBinding->szBuffer,
			(cchDisplay + 1) * sizeof(CHAR),
			&pThisBinding->indPtr);
	}
}
