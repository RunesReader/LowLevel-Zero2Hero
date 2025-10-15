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

	if (dbhdr == NULL || employeesOut == NULL) {
        printf("Try to dereferencing NULL pointer\n");
        return STATUS_ERROR;
    }

	int count = dbhdr->count;
	struct employee_t *employees = calloc(count, sizeof(struct employee_t));
	if (employees == NULL) {
		perror("Try to calloc");
		return STATUS_ERROR;
	}

	lseek(fd, sizeof(struct dbheader_t), SEEK_SET);
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

	if (dbhdr == NULL || employees == NULL) {
        printf("Try to dereferencing NULL pointer\n");
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
	if (dbhdr == NULL || employees == NULL) {
        printf("Try to dereferencing NULL pointer\n");
        return;
    }
    
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
    return STATUS_ERROR;
    char *emp_name = strtok(addstring, ",");
    if (emp_name == NULL) {
        printf("You have to fill Employee Name\n");
        return STATUS_ERROR;
    }
    char *addr = strtok(NULL, ",");
    if (addr == NULL) {
        printf("You have to fill Employee Address\n");
        return STATUS_ERROR;
    }
    char *emp_hours = strtok(NULL, ",");
    if (emp_hours == NULL) {
        printf("You have to fill Employee Hours\n");
        return STATUS_ERROR;
    }
    
    int new_count= dbhdr->count + 1;
    struct employee_t *employee_array = *employees;
    employee_array = realloc(*employees, new_count*(sizeof(struct employee_t)));
    if (employee_array == NULL) {
        perror("Realloc");
        return STATUS_ERROR;
    }

    dbhdr->count++;
    
    int new_index = dbhdr->count - 1;
    strncpy(employee_array[new_index].name, emp_name, sizeof(employee_array[new_index].name) - 1);
    employee_array[new_index].name[sizeof(employee_array[new_index].name) - 1] = '\0';
    strncpy(employee_array[new_index].address, addr, sizeof(employee_array[new_index].address) - 1);
    employee_array[new_index].address[sizeof(employee_array[new_index].address) - 1] = '\0';
    employee_array[new_index].hours = atoi(emp_hours);

    *employees = employee_array;

    return STATUS_SUCCESS;
}
