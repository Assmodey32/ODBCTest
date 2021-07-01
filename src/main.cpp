#include "../lib/ODBCTest.h"

int main()
{
	ODBCTest t;
	t.sqlConn();
	t.sqlExec();
	t.sqlDisconn();

	return 0;
}
