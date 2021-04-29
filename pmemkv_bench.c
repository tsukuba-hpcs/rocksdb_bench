#include <omp.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <libpmemkv.h>

static pmemkv_db *db = NULL;

void
kv_init(char *db_dir, char *engine, char *path, size_t size)
{
	pmemkv_config *cfg;
	char *p;
	int r;
	struct stat sb;
	static const char diag[] = "kv_init";

	if (db_dir == NULL || engine == NULL || path == NULL)
		fprintf(stderr, "%s: invalid argument\n", diag), exit(1);

	p = malloc(strlen(db_dir) + 1 + strlen(path) + 1);
	if (p == NULL)
		fprintf(stderr, "%s: no memory\n", diag), exit(1);
	if (stat(db_dir, &sb))
		fprintf(stderr, "%s: %s: %s\n",
			diag, db_dir, strerror(errno)), exit(1);
	if (S_ISDIR(sb.st_mode))
		sprintf(p, "%s/%s", db_dir, path);
	else
		sprintf(p, "%s", db_dir);
	/*printf("kv_init: engine %s, path %s, size %ld\n", engine, p, size);*/

	cfg = pmemkv_config_new();
	if (cfg == NULL)
		fprintf(stderr, "%s (config_new): %s\n", diag,
			pmemkv_errormsg()), exit(1);
	r = pmemkv_config_put_string(cfg, "path", p);
	if (r != PMEMKV_STATUS_OK)
		fprintf(stderr, "%s (config_put_string:path): %s\n", diag,
			pmemkv_errormsg()), exit(1);

	r = pmemkv_open(engine, cfg, &db);
	if (r == PMEMKV_STATUS_OK)
		goto free_p;
	else
		/* printf("%s, continue\n", pmemkv_errormsg()) */;

	cfg = pmemkv_config_new();
	if (cfg == NULL)
		fprintf(stderr, "%s (config_new): %s\n", diag,
			pmemkv_errormsg()), exit(1);
	r = pmemkv_config_put_string(cfg, "path", p);
	if (r != PMEMKV_STATUS_OK)
		fprintf(stderr, "%s (config_put_string:path): %s\n", diag,
			pmemkv_errormsg()), exit(1);
	r = pmemkv_config_put_uint64(cfg, "size", size);
	if (r != PMEMKV_STATUS_OK)
		fprintf(stderr, "%s (config_put_uint64:size): %s\n", diag,
			pmemkv_errormsg()), exit(1);
	r = pmemkv_config_put_uint64(cfg, "force_create", 1);
	if (r != PMEMKV_STATUS_OK)
		fprintf(stderr, "%s (config_put_uint64:force_create): %s\n",
			diag, pmemkv_errormsg()), exit(1);

	r = pmemkv_open(engine, cfg, &db);
	if (r != PMEMKV_STATUS_OK)
		fprintf(stderr, "%s: %s\n", diag, pmemkv_errormsg()), exit(1);
	/* printf("%s: created\n", p); */
free_p:
	if (db == NULL)
		fprintf(stderr, "%s: db is NULL\n", diag), exit(1);
	free(p);
}

void
kv_term()
{
	pmemkv_close(db);
}

int
kv_put(void *key, size_t key_size, void *value, size_t value_size)
{
	return (pmemkv_put(db, key, key_size, value, value_size));
}

int
kv_get(void *key, size_t key_size, void *value, size_t value_size,
	size_t *osize)
{
	return (pmemkv_get_copy(db, key, key_size, value, value_size, osize));
}

int
kv_remove(void *key, size_t key_size)
{
	return (pmemkv_remove(db, key, key_size));
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
		if (ret != PMEMKV_STATUS_OK) {
			fprintf(stderr, "%ld %s\n", i, pmemkv_errormsg());
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
		ret = kv_get(key, KEY_SIZE, value, vsize, &osize);
		if (ret != PMEMKV_STATUS_OK)
			fprintf(stderr, "%ld %s\n", i, pmemkv_errormsg());
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
		if (ret != PMEMKV_STATUS_OK)
			fprintf(stderr, "%ld %s\n", i, pmemkv_errormsg());
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
