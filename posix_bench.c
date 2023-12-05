#include <omp.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include "kv_err.h"
#include "fs.h"

void
kv_init(char *dir)
{
	static const char diag[] = "kv_init";

	if (dir == NULL)
		printf("%s: invalid argument", diag);

	fs_inode_init(dir, 0);
}

void
kv_term()
{
}

int
kv_put(void *key, size_t key_size, void *value, size_t value_size)
{
	return fs_inode_create(key, key_size,
	   0, 0, 0100644, 4096,
	   value, value_size);
}

int
kv_get(void *key, size_t key_size, void *value, size_t *value_size)
{
	return fs_inode_write(key, key_size, value, value_size, 0, 0100644, 4096);
}

int
kv_remove(void *key, size_t key_size)
{
	return fs_inode_remove(key, key_size);
}

void
usage(char *prog_name)
{
	fprintf(stderr, "Usage: %s [-T nthreads] [-v vsize] "
			"[-n n_keys] db_dir\n", prog_name);
	fprintf(stderr, "Output: <type> <vsize> <n_keys> <time> "
			"<throughput MB/s> <IOPS count/s>\n");
	exit(EXIT_FAILURE);
}

#define KEY_SIZE	20
__thread char key[KEY_SIZE], *value;

int
main(int argc, char *argv[])
{
	int c, vsize = 4 * 1024, ret;
	char *db_dir = NULL, nthreads = 1;
	size_t i, n_keys = 100000;
	size_t osize;
	struct timeval tv[2];
	double t;
	long long total_vsize;

	while ((c = getopt(argc, argv, "s:T:v:n:")) != -1) {
		switch (c) {
		case 'T':
			nthreads = atoi(optarg);
			break;
		case 'v':
			vsize = atoi(optarg);
			break;
		case 'n':
			n_keys = atoi(optarg);
			break;
		default:
			usage("pmemkv_bench");
		}
	}
	argc -= optind;
	argv += optind;
	if (argc == 0)
		usage("pmemkv_bench");
	db_dir = argv[0];

	kv_init(db_dir);

	omp_set_num_threads(nthreads);
#pragma omp parallel
	value = calloc(vsize, 1);

	gettimeofday(&tv[0], NULL);
#pragma omp parallel for private(ret) schedule(static, 1)
	for (i = 0; i < n_keys; ++i) {
		sprintf(key, "%0*ld", KEY_SIZE - 1, i);
		ret = kv_put(key, KEY_SIZE, value, vsize);
		if (ret != KV_SUCCESS) {
			fprintf(stderr, "put failed %ld \n", i);
			/* break; */
		}
	}
	gettimeofday(&tv[1], NULL);

	t = tv[1].tv_sec - tv[0].tv_sec
		+ .000001 * (tv[1].tv_usec - tv[0].tv_usec);
	/* total_vsize = (long long)vsize * i; */
	total_vsize = (long long)vsize * n_keys;
	printf("put: %d %ld %f %f %f\n", vsize, n_keys, t,
			1.0 / 1024 / 1024 * total_vsize / t, .001 * n_keys / t);

	gettimeofday(&tv[0], NULL);
#pragma omp parallel for private(ret) schedule(static, 1)
	for (i = 0; i < n_keys; ++i) {
		sprintf(key, "%0*ld", KEY_SIZE - 1, i);
		ret = kv_get(key, KEY_SIZE, value, &osize);
		if (ret != KV_SUCCESS)
			fprintf(stderr, "get failed %ld \n", i);
	}
	gettimeofday(&tv[1], NULL);

	t = tv[1].tv_sec - tv[0].tv_sec
		+ .000001 * (tv[1].tv_usec - tv[0].tv_usec);
	total_vsize = (long long)vsize * n_keys;
	printf("get: %d %ld %f %f %f\n", vsize, n_keys, t,
			1.0 / 1024 / 1024 * total_vsize / t, .001 * n_keys / t);

	gettimeofday(&tv[0], NULL);
#pragma omp parallel for private(ret) schedule(static, 1)
	for (i = 0; i < n_keys; ++i) {
		sprintf(key, "%0*ld", KEY_SIZE - 1, i);
		ret = kv_remove(key, KEY_SIZE);
		if (ret != KV_SUCCESS)
			fprintf(stderr, "%ld \n", i);
	}
	gettimeofday(&tv[1], NULL);

	t = tv[1].tv_sec - tv[0].tv_sec
		+ .000001 * (tv[1].tv_usec - tv[0].tv_usec);
	total_vsize = (long long)vsize * n_keys;
	printf("remove: %d %ld %f %f %f\n", vsize, n_keys, t,
			1.0 / 1024 / 1024 * total_vsize / t, .001 * n_keys / t);

	kv_term();
	return (0);
}
