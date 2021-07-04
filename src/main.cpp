#include <iostream>
#include "../lib/ODBCTest.h"

int main()
{
	ODBCTest t;


	std::string str;
	std::cout << "Enter name of file containing SQL query: ";
	std::getline(std::cin, str);

	t.readSQLFile(str);
	
	t.sqlConn();
	t.sqlExec();
	t.sqlDisconn();

	return 0;
}
