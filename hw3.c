#define FUSE_USE_VERSION 26


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/xattr.h>
#include <dirent.h>
#include <unistd.h>
#include <fuse.h>
// for tidy
#include <tidy.h>
#include <tidybuffio.h>

// read-write path
char *rw_path;

static char* translate_path(const char* path)
{
	printf("in translate_path...\n");
    char *rPath= malloc(sizeof(char)*(strlen(path)+strlen(rw_path)+1));

	printf("rw_path is : %s \n", rw_path);
    strcpy(rPath,rw_path);
    printf("rPath is : %s \n", rPath);
    if (rPath[strlen(rPath)-1]=='/') {
        rPath[strlen(rPath)-1]='\0';
    }
    strcat(rPath,path);
    printf("rPath after cat : %s \n", rPath);

    return rPath;
}

static int hw3_getattr(const char *path, struct stat *st_data)
{
    int res;
    char *upath=translate_path(path);

	printf("in getattr...\n");
    printf("upath is : %s \n", upath);
    res = lstat(upath, st_data);
    free(upath);
    if(res == -1) {
        return -errno;
    }
    return 0;
}

static int hw3_readlink(const char *path, char *buf, size_t size)
{
    int res;
    char *upath=translate_path(path);

	printf("in readlink...\n");
    printf("upath is : %s \n", upath);

    res = readlink(upath, buf, size - 1);
    printf("res is : %d \n", res);
    free(upath);
    if(res == -1) {
        return -errno;
    }
    buf[res] = '\0';
    return 0;
}

static int hw3_readdir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
    DIR *dp;
    struct dirent *de;
    int res;

	printf("in readdir...\n");
    (void) offset;
    (void) fi;

    char *upath=translate_path(path);

    dp = opendir(upath);
    free(upath);
    if(dp == NULL) {
        res = -errno;
        return res;
    }

    while((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

static int hw3_open(const char *path, struct fuse_file_info *finfo)
{
    int res;

    // Open is allowed
    int flags = finfo->flags;
	printf("in open...\n");
    if ((flags & O_WRONLY) || (flags & O_RDWR) || (flags & O_CREAT) || (flags & O_EXCL) || (flags & O_TRUNC) || (flags & O_APPEND)) {
        return -EROFS;
    }

    char *upath=translate_path(path);

    res = open(upath, flags);

    free(upath);
    if(res == -1) {
        return -errno;
    }
    close(res);
    return 0;
}

static int hw3_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *finfo)
{
    int fd;
    int res;
    (void)finfo;

	printf("in read...\n");
    char *upath=translate_path(path);
    fd = open(upath, O_RDONLY);
    free(upath);
    if(fd == -1) {
        res = -errno;
        return res;
    }
    res = pread(fd, buf, size, offset);

    if(res == -1) {
        res = -errno;
    }
    close(fd);
    return res;
}

static int hw3_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *finfo)
{
    // return an error
    return -EROFS;
}

static int hw3_access(const char *path, int mode)
{
    int res;
    char *upath=translate_path(path);

    /* Not sure if necessary...
     */
    if (mode & W_OK)
        return -EROFS;

    res = access(upath, mode);
    free(upath);
    if (res == -1) {
        return -errno;
    }
    return res;
}

struct fuse_operations hw3_oper = {
    .getattr     = hw3_getattr,
    .readlink    = hw3_readlink,
    .readdir     = hw3_readdir,
    .open        = hw3_open,
    .read        = hw3_read,
    .write       = hw3_write,
    .access      = hw3_access
};

static int hw3_parse_opt(void *data, const char *arg, int key,
                          struct fuse_args *outargs)
{
    (void) data;

    switch (key)
    {
    case FUSE_OPT_KEY_NONOPT:
        if (rw_path == 0)
        {
            rw_path = strdup(arg);
            return 0;
        }
        else
        {
            return 1;
        }
    case FUSE_OPT_KEY_OPT:
        return 1;
    /*case KEY_HELP:
        usage(outargs->argv[0]);
        exit(0);
    case KEY_VERSION:
        fprintf(stdout, "ROFS version %s\n", rofsVersion);
        exit(0);*/
    default:
        fprintf(stderr, "see `%s -h' for usage\n", outargs->argv[0]);
        exit(1);
    }
    return 1;
}

static struct fuse_opt hw3_opts[] = {
    FUSE_OPT_KEY("-h",          KEY_HELP),
    FUSE_OPT_KEY("--help",      KEY_HELP),
    FUSE_OPT_KEY("-V",          KEY_VERSION),
    FUSE_OPT_KEY("--version",   KEY_VERSION),
    FUSE_OPT_END
};

int main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    int res;
    printf("in main...\n");
    res = fuse_opt_parse(&args, &rw_path, hw3_opts, hw3_parse_opt);
    if (res != 0)
    {
        fprintf(stderr, "Invalid arguments\n");
        exit(1);
    }
    if (rw_path == 0)
    {
        fprintf(stderr, "Missing readwritepath\n");
        exit(1);
    }

    fuse_main(args.argc, args.argv, &hw3_oper, NULL);
	printf("rw_path is : %s \n", rw_path);

    return 0;
}
