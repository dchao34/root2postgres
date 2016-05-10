#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include <exception>
#include <libpq-fe.h>
#include "PostgresConnector.h"

using std::string;
using std::vector;

static void 
exit_nicely(PGconn *conn) {
  PQfinish(conn);
  exit(EXIT_FAILURE);
}

// The constructor handles the opening of the connection to the database,
// and also prepares the statement template to be used later for insertion.
//
// Note that the param_values_vector_ is initialized to "DEFAULT", so that 
// if some variables in var_names were not called "bind()", the default value
// would be inserted into the table
PostgresConnector::PostgresConnector(string db_name, 
                                     string table_name,
                                     vector<string> var_names)
    : num_vars_(var_names.size()),
      param_values_vector_(vector<string>(var_names.size(), "DEFAULT")) {

  // Initialize map to hold variable indexes
  for (size_t i = 0; i < num_vars_; ++i) {
    var_name_to_idx_map_.insert(std::make_pair(var_names[i], i));
  }

  // Allocate the array to hold variable values
  param_values_ = new const char*[num_vars_];

  // Initialize and open connection to the databse
	db_name_ = db_name;
	table_name_ = table_name;
	conn_ = PQsetdbLogin(pghost_, pgport_, pgoptions_, pgtty_, 
                       db_name_.c_str(), login_, pwd_);	
  if (PQstatus(conn_) != CONNECTION_OK) {
    std::cerr << "Connection to database failed: " << PQerrorMessage(conn_);
    exit_nicely(conn_);
  }
  
  // Construct command to be passed preparing the statement 
  stmt_name_ = "PREPARE_DATA_FOR_INSERT";
  string stmt_command = "INSERT INTO " + table_name + " (";
  for (const auto &n : var_names) {
    stmt_command += n + ",";
  }
  stmt_command.pop_back();
  stmt_command += ") VALUES (";
  for (size_t i = 1; i <= num_vars_; ++i) {
    stmt_command += "$" + std::to_string(i) + ",";
  }
  stmt_command.pop_back();
  stmt_command += ");";

  // Prepare the statement
  res_ = PQprepare(conn_, stmt_name_.c_str(), stmt_command.c_str(), 
                   num_vars_, nullptr);
  if (PQresultStatus(res_) != PGRES_COMMAND_OK) {
    std::cerr << "Prepare failed: " << PQerrorMessage(conn_);
    PQclear(res_);
    exit_nicely(conn_);
  }
  PQclear(res_);
}

PostgresConnector::~PostgresConnector(){
	PQfinish(conn_);
  delete[] param_values_;
}

// The bind method puts the value of the variable into the correct index
// in the param_values_vector_ to be pointed at by param_values_ later
void PostgresConnector::bind(const string &var_name, const string &var_value) {

  // Set the appropriate element in param_values_vector_ 
  param_values_vector_.at(var_name_to_idx_map_.at(var_name)) = var_value;
}

// The exec() method fills in the param_values_, which is the type required by 
// PQexecPrepared, and performs the insertion. 
void PostgresConnector::exec(){

  // Fill in param_values_ to values stored in param_values_vector
  for (size_t i = 0; i < num_vars_; ++i) {
    param_values_[i] = param_values_vector_[i].c_str();
  }
  // Execute the statement
  res_ = PQexecPrepared(conn_, stmt_name_.c_str(), num_vars_, param_values_, nullptr, nullptr, 0);
  if (PQresultStatus(res_) != PGRES_COMMAND_OK) {
    std::cerr << "Executing prepared failed: " << PQerrorMessage(conn_);
    PQclear(res_);
    exit_nicely(conn_);
  }
  PQclear(res_);
}
