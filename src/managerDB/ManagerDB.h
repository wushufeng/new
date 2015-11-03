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
extern sqlite3 *pDB;

int openDatabase(const char *dbfile, sqlite3 *db);
int closeDatabase(sqlite3 *db);
int createSqlTable(void);
int databaseInsert(void *obj, int group, time_t dgtime, int dgflag, int elecflag);
int deviceInfoInsert(void *obj, int group, time_t test_time);
int testCreateTables(void);
int showRecordsByTime(time_t time);
int delOvertimeData(void);
int searchOldestData(void);
#endif /* SRC_MANAGERDB_MANAGERDB_H_ */
