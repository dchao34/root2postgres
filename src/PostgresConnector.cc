#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <libpq-fe.h>

PostgresConnector::PostgresConnector(std::string db_name, std::string table_name){
	db_name_ = db_name;
	table_name_ = table_name;
	conn = PQsetdbLogin(pghost_, pgport_, pgoptions_, pgtty_, db_name_.c_str(), login_, pwd_);	
	if(PQstatus(conn) != CONNECTION_OK)
		throw PQerrorMessage(conn);
}

PostgresConnector::~PostgresConnector(){
	PQfinish(conn_);
	PQclear(res_);
}

void PostgresConnector::regis(){
	command_column_ = "INSERT INTO " + table_name_ + "(";
	command_value_ = " values (";
}

void PostgresConnector::insert(std::string column, std::string value){
	command_column_ += column + ",";
	command_value_ += value + ",";
}

void PostgresConnector::exec(){
	command_column_.pop_back();
	command_value_.pop_back();
	command_column_ += ")";
	command_value_ += ");"
	std::string command_exec_;
	command_exec_ = command_column_ + command_value_;
	res_ = PQexec(conn, command_exec_.c_str());
	if(PQresultStatus(res) != PGRES_COMMAND_OK)
		throw PQresultErrorMessage(res);
}