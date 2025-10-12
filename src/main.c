#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
	printf("Usage: %s -n -f <database file>\n", argv[0]);
	printf("\t -n - create new database file\n");
	printf("\t -f - (requared) path to database file\n");

	return;
}

int main(int argc, char *argv[]) { 
	char *filepath = NULL;
	char *addstring = NULL;
	bool newfile = false;
	bool list = false;
	int c;
	int dbfd = STATUS_ERROR;
	struct dbheader_t *dbhdr = NULL;
	struct employee_t *employees = NULL;

	while ((c = getopt(argc, argv, "nf:a:l")) != STATUS_ERROR) {
		switch (c) {
			case 'n':
				newfile = true;
				break;
			case 'f':
				filepath = optarg;
				break;
			case 'a':
				addstring = optarg;
				break;
			case 'l':
				list = true;
				break;
			default:
				return STATUS_ERROR;

		}
	}

	if (filepath == NULL) {
		printf("Filepath is a requared argument\n");
		print_usage(argv);

		return STATUS_SUCCESS;
	}

	if (newfile) {
		dbfd = create_db_file(filepath);
		if (dbfd == STATUS_ERROR) {
			printf("Unable to create database file\n");
			return STATUS_ERROR;
		}

		if (create_db_header(&dbhdr) == STATUS_ERROR) {
			printf("Failed to create database header\n");
			return STATUS_ERROR;
		}
	} else {
		dbfd = open_db_file(filepath);
		if (dbfd == STATUS_ERROR) {
			printf("Unable to open database file\n");
			return STATUS_ERROR;
		}

		if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
			printf("Failed to validate database header\n");
			return STATUS_ERROR;
		}
	}

	if (read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS) {
		printf("Failed to read employees\n");
		return STATUS_ERROR;
	}

	if (addstring) {
		dbhdr->count++;
		int new_count = dbhdr->count;
		void *tmp = realloc(employees, new_count*(sizeof(struct employee_t)));
		if (tmp == NULL) {
			perror("Realloc");
			free(employees);
			return STATUS_ERROR;
		} else {
			employees = tmp;
		}
		
		if (add_employee(dbhdr, &employees, addstring) != STATUS_SUCCESS) {
			printf("Failed to add employee\n");
			return STATUS_ERROR;
		}
	}

	if (list) {
		list_employees(dbhdr, employees);
	}

	output_file(dbfd, dbhdr, employees);

	return STATUS_SUCCESS;
}
