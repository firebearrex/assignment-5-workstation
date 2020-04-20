/*
 * file_util.c
 *
 * Utilities functions for working with C FILE streams.
 *
 *  @since 2019-04-10
 *  @author: Philip Gust
 */

#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "http_server.h"
#include "file_util.h"
#include <sys/param.h>
#include <dirent.h>
#include <time.h>
#include <http_util.h>

/**
 * This function creates a temporary stream for this string.
 * When the FILE is closed, it will be automatically removed.
 *
 * @param contentStr the content of the file
 * @return the FILE for this string
 */
FILE *tmpStringFile(const char *contentStr) {
	FILE *tmpstream = tmpfile();
	fwrite(contentStr, 1, strlen(contentStr), tmpstream);
	rewind(tmpstream);
	return tmpstream;
}

/**
 * This function calls fstat() on the file descriptor of the
 * specified stream.
 *
 * @param stream the stream
 * @param buf the stat struct
 * @return 0 if successful, -1 with errno set if error.
 */
int fileStat(FILE *stream, struct stat *buf) {
	int fd = fileno(stream);
	return fstat(fd, buf);
}


/**
 * Copy bytes from input stream to output stream
 * @param istream the input stream
 * @param ostream the output stream
 * @param nbytes the number of bytes to send
 * @param return 0 if successful
 */
int copyFileStreamBytes(FILE *istream, FILE *ostream, int nbytes) {
	char buf[MAXBUF];
    while ((nbytes > 0) && !feof(istream)) {
    	int ntoread = (nbytes < MAXBUF) ? nbytes : MAXBUF;
        size_t nread = fread(buf, sizeof(char), ntoread, istream);
        if (nread > 0) {
			if (fwrite(buf, sizeof(char), nread, ostream) < nread) {
				perror("copyFileStreamBytes");
				return -1;
			}
			nbytes -= nread;
		}
    }
    return 0;
}

/**
 * Returns path component of the file path without trailing
 * path separator. If no path component, returns NULL.
 *
 * @param filePath the path and file
 * @param pathOfFile return buffer (must be large enough)
 * @return pointer to pathOfFile or NULL if no path
 */
char *getPath(const char *filePath, char *pathOfFile) {
	char *p = strrchr(filePath, '/');
	if (p == NULL) {
		return NULL;
	}
	strncpy(pathOfFile, filePath, p-filePath);
	pathOfFile[p-filePath] = '\0';  // must terminate;
	return pathOfFile;
}

/**
 * Returns name component of the file.
 *
 * @param filePath the path and file
 * @param name return buffer (must be large enough)
 * @return pointer to name or NULL if no path
 */
char *getName(const char *filePath, char *name) {
	char *p = strrchr(filePath, '/');
	strcpy(name, (p == NULL) ? filePath : p+1);
	return name;
}

/**
 * Returns extension of the file path without the '.'.
 * If no extension, returns NULL.
 *
 * @param filePath the path and file
 * @param extension return buffer (must be large enough)
 * @return pointer to extension or NULL if no path
 */
char *getExtension(const char *filePath, char *extension) {
	char *p = strrchr(filePath, '.');
	if (p == NULL) {
		return NULL;
	}
	strcpy(extension, p+1);
	return extension;
}

/**
 * Make a file path by combining a path and a file name.
 * If the file name begins with '/', return the name as an
 * absolute. Otherwise, concatenate the path and name,
 * ensuring there is a '/' separator between them.
 *
 * @param path the path component
 * @param name the file name component
 * @return the file path
 */
char *makeFilePath(const char *path, const char *name, char *filepath) {
	if (name[0] == '/') {
		strcpy(filepath, name);
	} else {
		strcpy(filepath, path);
		if (path[strlen(path)-1] != '/') {
			strcat(filepath, "/");
		}
		strcat(filepath, name);
	}
	return filepath;
}

/**
 * Make directories specified by path.
 *
 * @param path the path to create
 * @param mode mode if a directory is created
 * @return 0 if successful, -1 if error
 */
int mkdirs(const char *path, mode_t mode) {
	char buf[strlen(path)+1];
	const char *p = (*path == '/') ? path+1 : path;
	while (*p != '\0') {
		p = strchr(p, '/');  // find next path separator
		if (p == NULL) {     // last path element
			p = path + strlen(path);
		}
		strncpy(buf, path, p-path);
		buf[p-path] = '\0';  // must terminate
		if (mkdir(buf, mode) == -1) {
			if (errno == ENOTDIR) {
				return -1;
			}
		}
		if (*p == '/') {  // advance past path separator
			p++;
		}
	}
	return 0;
}

// buf needs to store 30 characters
int timespec2str(char *buf, uint len, struct timespec *ts) {
    int ret;
    struct tm t;

    tzset();
    if (localtime_r(&(ts->tv_sec), &t) == NULL)
        return 1;

    ret = strftime(buf, len, "%F %T", &t);
    if (ret == 0)
        return 2;
    len -= ret - 1;

    //ret = snprintf(&buf[strlen(buf)], len, ".%09ld", ts->tv_nsec);
    if (ret >= len)
        return 3;

    return 0;
}

/**
 * Generates the directory listing as a temporary file and returns a FILE*
 *
 * @param path the path to the directory
 * @return FILE pointer to the file listing contents of the directory
 */
void get_dir_listings(const char *uri, const char *path, const char *filename) {
    char filePath[MAXPATHLEN];
    char fileInDir[MAXPATHLEN];
    char buf[MAXBSIZE];
    long bufLen;
    char timeStr[TIME_FMT];

    // Create file filename
    makeFilePath(path, filename, filePath);

    // open a stream for filePath to write
    FILE *listDirStream = fopen(filePath, "w");

    startHtmlPage(uri, listDirStream);
    //fprintf(listDirStream, "Name\tLast modified\tSize\tFile version\n");

    // Collect data
    DIR *dir;
    struct stat sb;
    struct dirent *dirEntry;

    dir = opendir(path);

    if (dir)
    {
        while ((dirEntry = readdir(dir)) != NULL)
        {
            if ((strcmp(dirEntry->d_name, filename) == 0)
             || (strcmp(dirEntry->d_name, ".") == 0)){
                continue;
            }

            memset(fileInDir, 0, sizeof(fileInDir));
            memset(timeStr, 0, sizeof(timeStr));
            memset(buf, 0, sizeof(buf));

            strcpy(fileInDir, path);
            strcat(fileInDir, "/");
            strcat(fileInDir, dirEntry->d_name);

            if (stat(fileInDir, &sb) == -1) {
                fprintf(stderr, "Can't stat %s: %s\n", dirEntry->d_name,
                        strerror(errno));

                printf("unknown");
            } else {
                timespec2str(timeStr, sizeof(timeStr), &sb.st_mtimespec);
                makeHtmlEntry(listDirStream, dirEntry->d_name, timeStr, sb.st_size, sb.st_mode);
                //fprintf(listDirStream, "%s\t%s\t%lld\t%d\n", dirEntry->d_name, timeStr, sb.st_size, sb.st_mode);
            }
        }
        endHtmlPage(listDirStream);
        closedir(dir);
        fclose(listDirStream);
    }
}
