/*
 * ManagerDB.h
 *
 *  Created on: 2015年6月26日
 *      Author: wsf
 */

#ifndef SRC_MANAGERDB_MANAGERDB_H_
#define SRC_MANAGERDB_MANAGERDB_H_
#include	<sqlite3.h>

extern const char *dbfile;
extern sqlite3 *DBfd;

int openDatabase(const char *dbfile);
int closeDatabase(sqlite3 *db);
int createDatabase(sqlite3 *db);

#endif /* SRC_MANAGERDB_MANAGERDB_H_ */
