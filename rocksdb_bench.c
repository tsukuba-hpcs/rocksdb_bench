#include <omp.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "rocksdb/c.h"
#include "kv_err.h"

static rocksdb_t *db = NULL;
//static rocksdb_column_family_handle_t *cf = NULL;


void
kv_init(char *db_dir, char *engine, char *path, size_t size)
{
	static const char diag[] = "kv_init";

	if (db_dir == NULL || engine == NULL || path == NULL)
		printf("%s: invalid argument", diag);

	long cpus = sysconf(_SC_NPROCESSORS_ONLN);

	rocksdb_options_t *options = rocksdb_options_create();
	rocksdb_options_increase_parallelism(
	    options, (int)(cpus));
	rocksdb_options_set_max_background_compactions(options, 1);
	rocksdb_options_set_max_background_flushes(options, 1);
	rocksdb_options_set_level_compaction_dynamic_level_bytes(options, 1);
	rocksdb_options_optimize_level_style_compaction(options, 0);
	rocksdb_options_set_bytes_per_sync(options, 1048576);
	rocksdb_options_set_write_buffer_size(options, 1024 * 1024 * 1024); //1GB
	rocksdb_options_set_level0_file_num_compaction_trigger(options, -1);
	rocksdb_options_set_target_file_size_base(options, 1024 * 1024 * 1024); //1GB
	//rocksdb_options_enable_statistics(options);
	//rocksdb_options_set_statistics_level(options,
	//				     rocksdb_statistics_level_all);
	rocksdb_options_set_stats_dump_period_sec(options, 10);
    	rocksdb_options_set_create_if_missing(options, 1);


	char *p;
	struct stat sb;

	if ((p = malloc(strlen(db_dir) + 1 + strlen(path) + 1)) == NULL)
		printf("%s: no memory", diag);

	if (stat(db_dir, &sb))
		printf("%s: %s: %s", diag, db_dir, strerror(errno));

	if (S_ISDIR(sb.st_mode))
		sprintf(p, "%s/%s", db_dir, path);
	else
		sprintf(p, "%s", db_dir);
	printf("kv_init: engine %s, path %s, size %ld\n", engine, p, size);

	char *err = NULL;
	db = rocksdb_open(options, p, &err);
	if(err != NULL || db == NULL)
		printf("%s: %s", diag, err);

	free(p);

	//cf = rocksdb_create_column_family(db, options, "default", &err);
	//if(err != NULL || db == NULL)
		//printf("%s: %s", diag, err);
}

void
kv_term()
{
}

int
kv_put(void *key, size_t key_size, void *value, size_t value_size)
{
	char *err = NULL;

	if(err==NULL){
		return KV_SUCCESS;
	} else {
		printf("err: %s\n", err);
		return KV_ERR_UNKNOWN;
	}
}

int
kv_get(void *key, size_t key_size, void *value, size_t *value_size)
{
	char *err = NULL;
	value = rocksdb_get(db, rocksdb_readoptions_create(), key, key_size, value_size, &err);
	if (value == NULL) {
		return KV_ERR_NO_ENTRY;
	}
	if(err!=NULL){
		return KV_ERR_UNKNOWN;
	}
	return KV_SUCCESS;
}


int
kv_remove(void *key, size_t key_size)
{
	char *hoge = ""; //not use
	size_t fuga; //not use
	char isexist = kv_get(key, key_size, hoge, &fuga);
	if (isexist != KV_SUCCESS) {
		//log_debug("local rocksdb remove isexist failed");
		return isexist;
	}

	char *err = NULL;
	if(err==NULL){
		return KV_SUCCESS;
	} else {
		//log_debug("local rocksdb remove: error: %s", err);
		return KV_ERR_UNKNOWN;
	}
}

void
usage(char *prog_name)
{
	fprintf(stderr, "Usage: %s [-s db_size] [-T nthreads] [-v vsize] "
			"[-n n_keys] db_dir\n", prog_name);
	exit(EXIT_FAILURE);
}

#define KEY_SIZE	20
__thread char key[KEY_SIZE], *value;

int
main(int argc, char *argv[])
{
	int c, vsize = 4 * 1024, ret;
	char *db_dir = NULL, nthreads = 1;
	size_t i, db_size = 1024 * 1024 * 1024, n_keys = 100000;
	size_t osize;
	struct timeval tv[2];
	double t;
	long long total_vsize;

	while ((c = getopt(argc, argv, "s:T:v:n:")) != -1) {
		switch (c) {
		case 's':
			db_size = atol(optarg);
			break;
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

	kv_init(db_dir, "cmap", "kv.db", db_size);

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
