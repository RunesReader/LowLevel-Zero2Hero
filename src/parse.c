#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>

#include "common.h"
#include "parse.h"

int create_db_header(struct dbheader_t **headerOut) {
	if (headerOut == NULL) {
		printf("Try to dereferencing NULL pointer\n");
		return STATUS_ERROR;
	}

	struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
	if (header == NULL) {
		perror("Try to calloc");
		return STATUS_ERROR;
	}

	header->magic = HEADER_MAGIC;
	header->version = 0x1;
	header->count = 0x0;
	header->filesize = sizeof(struct dbheader_t);

	*headerOut = header;

	return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t **headerOut) {
	if (fd < 0) {
		printf("Got a bad file descriptor from the user\n");
		return STATUS_ERROR;
	}

	struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
	if (header == NULL) {
		perror("Try to create db header");
		return STATUS_ERROR;
	}

	if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
		perror("Try to read db file");
		free(header);
		return STATUS_ERROR;
	}

	header->version = ntohs(header->version);
	header->count = ntohs(header->count);
	header->magic = ntohl(header->magic);
	header->filesize = ntohl(header->filesize);

	if (header->version != 0x1) {
		printf("Improper header version\n");
		free(header);
		return STATUS_ERROR;
	}

	if (header->magic != HEADER_MAGIC) {
		printf("Improper header magic\n");
		free(header);
		return STATUS_ERROR;
	}

	struct stat dbstat = {0};
	fstat(fd, &dbstat);
	if (header->filesize != dbstat.st_size) {
		printf("Corrupted database\n");
		free(header);
		return STATUS_ERROR;
	}

	*headerOut = header;

	return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
	if (fd < 0) {
		printf("Got a bad file descriptor from the user\n");
		return STATUS_ERROR;
	}

	int count = dbhdr->count;
	struct employee_t *employees = calloc(count, sizeof(struct employee_t));
	if (employees == NULL) {
		perror("Try to calloc");
		return STATUS_ERROR;
	}

	read(fd, employees, count*sizeof(struct employee_t));
	for (int i = 0; i < count; i++) {
		employees[i].hours = ntohl(employees[i].hours);
	}

	*employeesOut = employees;

	return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
	if (fd < 0) {
		printf("Got a bad file descriptor from the user\n");
		return STATUS_ERROR;
	}

	int realcount = dbhdr->count;

	dbhdr->version = htons(dbhdr->version);
	dbhdr->count = htons(dbhdr->count);
	dbhdr->magic = htonl(dbhdr->magic);
	dbhdr->filesize = htonl(sizeof(struct dbheader_t) + sizeof(struct employee_t) * realcount);

	lseek(fd, 0, SEEK_SET);
	write(fd, dbhdr, sizeof(struct dbheader_t));

	for (int i = 0; i < realcount; i++) {
		employees[i].hours = htonl(employees[i].hours);
		write(fd, &employees[i], sizeof(struct employee_t));
	}

	return STATUS_SUCCESS;
}

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
	for (int i = 0; i < dbhdr->count; i++) {
		printf("Employee %d\n", i);
		printf("\tName: %s\n", employees[i].name);
		printf("\tAddress: %s\n", employees[i].address);
		printf("\tHours: %u\n", employees[i].hours);
	}
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t **employees, char *addstring) {
	if (dbhdr == NULL || employees == NULL || *employees == NULL || addstring == NULL) {
		printf("Try to dereferencing NULL pointer\n");
		return STATUS_ERROR;
	}

	if (strlen(addstring) == 0) {
		printf("Do not enter empty string\n");
        return STATUS_ERROR;
	}
	
	char *saveptr = NULL;
    char *emp_name = strtok_r(addstring, ",", &saveptr);
    char *addr = strtok_r(NULL, ",", &saveptr);
    char *emp_hours = strtok_r(NULL, ",", &saveptr);

    if (emp_name == NULL || addr == NULL || emp_hours == NULL) {
        printf("You have to fill all fields of Employee (Name, Address, and Hours are required)\n");
        return STATUS_ERROR;
    }

	if (strlen(emp_name) == 0 || strlen(addr) == 0 || strlen(emp_hours) == 0) {
		printf("Employee fields cannot be empty strings\n");
		return STATUS_ERROR;
	}

	char *endptr;
    long employee_hours_long = strtol(emp_hours, &endptr, 10);
    if (emp_hours == endptr) {
        printf("Invalid Hours value provided: No digits found.\n");
        return STATUS_ERROR;
    }
    if (*endptr != '\0') {
        printf("Invalid Hours value provided: Extra characters after the number.\n");
        return STATUS_ERROR;
    }
    if (employee_hours_long > INT_MAX || employee_hours_long < INT_MIN) {
        printf("Hours value is too large or too small to be stored.\n");
        return STATUS_ERROR;
    }

    int employee_hours = (int)employee_hours_long;
    if (employee_hours < 0) {
        printf("Hours cannot be negative.\n");
        return STATUS_ERROR;
    }

    dbhdr->count++;
	int new_count = dbhdr->count;
	void *tmp = realloc(*employees, new_count*(sizeof(struct employee_t)));
	if (tmp == NULL) {
		perror("Realloc");
		free(*employees);
		return STATUS_ERROR;
	} else {
		*employees = tmp;
	}

	struct employee_t *employee_array = *employees;
	int new_index = dbhdr->count-1;

	strncpy(employee_array[new_index].name, emp_name, sizeof(employee_array[new_index].name));
	strncpy(employee_array[new_index].address, addr, sizeof(employee_array[new_index].address));
	employee_array[new_index].hours = employee_hours;

	return STATUS_SUCCESS;
}
