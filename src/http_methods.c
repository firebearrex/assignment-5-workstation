/*
 * methods.c
 *
 * Functions that implement HTTP methods, including
 * GET, HEAD, PUT, POST, and DELETE.
 *
 *  @since 2019-04-10
 *  @author: Philip Gust
 */

#include "http_methods.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <unistd.h>
#include <dirent.h>

#include "http_server.h"
#include "http_util.h"
#include "time_util.h"
#include "media_util.h"
#include "properties.h"
#include "string_util.h"
#include "file_util.h"


/**
 * Handle GET or HEAD request.
 *
 * @param stream the socket stream
 * @param uri the request URI
 * @param requestHeaders the request headers
 * @param responseHeaders the response headers
 * @param sendContent send content (GET)
 */
static void do_get_or_head(FILE *stream, const char *uri, Properties *requestHeaders, Properties *responseHeaders, bool sendContent) {
	// get path to URI in file system
	char filePath[MAXPATHLEN];
	resolveUri(uri, filePath);
	FILE *contentStream = NULL;

	// ensure file exists
	struct stat sb;
	if (stat(filePath, &sb) != 0) {
		sendErrorResponse(stream, 404, "Not Found", responseHeaders);
		return;
	}
	// directory path ends with '/'
	if (S_ISDIR(sb.st_mode) && strendswith(filePath, "/")) {
//		// not allowed for this method
//		sendErrorResponse(stream, 405, "Method Not Allowed", responseHeaders);
		contentStream = get_dir_listings(uri, filePath);
        if (contentStream == NULL) {
            sendErrorResponse(stream, 405, "Method Not Allowed", responseHeaders);
            return;
        }
//        return;
        fileStat(contentStream, &sb);
	} else if (!S_ISREG(sb.st_mode)) { // error if not regular file
		sendErrorResponse(stream, 404, "Not Found", responseHeaders);
		return;
	}

	// record the file length
	char buf[MAXBUF];
	size_t contentLen = (size_t)sb.st_size;
	sprintf(buf,"%lu", contentLen);
	putProperty(responseHeaders,"Content-Length", buf);

	// record the last-modified date/time
	time_t timer = sb.st_mtim.tv_sec;
	putProperty(responseHeaders,"Last-Modified",
				milliTimeToRFC_1123_Date_Time(timer, buf));

	// get mime type of file
	getMediaType(filePath, buf);
	if (strcmp(buf, "text/directory") == 0) {
		// some browsers interpret text/directory as a VCF file
		strcpy(buf,"text/html");
	}
	putProperty(responseHeaders, "Content-type", buf);

	// send response
	sendResponseStatus(stream, 200, "OK");

	// Send response headers
	sendResponseHeaders(stream, responseHeaders);

	if (sendContent) {  // for GET
		if (contentStream == NULL) {
            contentStream = fopen(filePath, "r");
        }
		copyFileStreamBytes(contentStream, stream, contentLen);
		fclose(contentStream);
	}
}

/**
 * Handle GET request.
 *
 * @param stream the socket stream
 * @param uri the request URI
 * @param requestHeaders the request headers
 * @param responseHeaders the response headers
 * @param headOnly only perform head operation
 */
void do_get(FILE *stream, const char *uri, Properties *requestHeaders, Properties *responseHeaders) {
//    // get path to URI in file system
//    char filePath[MAXPATHLEN];
//    char newUri[MAXPATHLEN] = "";
//    resolveUri(uri, filePath);
//    long uriLen;
//    int count = 1;
//
//    // ensure file exists
//    struct stat sb;
//    if (stat(filePath, &sb) != 0) {
//        sendErrorResponse(stream, 404, "Not Found", responseHeaders);
//        return;
//    }
//
//    // directory path ends with '/'
//    if (S_ISDIR(sb.st_mode) && strendswith(filePath, "/")) {
//        char listingFile[] = "listDir.html";
//        long lenFile = strlen(listingFile);
//
//        // Generate directory listing
//        // create any intermediate directories
//        char pathOfFile[MAXPATHLEN];
//        if (getPath(filePath, pathOfFile) != NULL) {
//            mkdirs(pathOfFile, 0777);
//        }
//        get_dir_listings(uri, pathOfFile, listingFile);
//
//        // Call do_get() with uri/listings path
//        uriLen = strlen(uri);
//        strncat(newUri, uri, uriLen);
//        strncat(newUri, listingFile, lenFile);
//
//        do_get(stream, newUri, requestHeaders, responseHeaders);
//
//        // Remove uri/listings file
//        resolveUri(newUri, filePath);
//        //unlink(filePath);
//
//    } else {
//    }
    do_get_or_head(stream, uri, requestHeaders, responseHeaders, true);
}

/**
 * Handle HEAD request.
 *
 * @param the socket stream
 * @param uri the request URI
 * @param requestHeaders the request headers
 * @param responseHeaders the response headers
 */
void do_head(FILE *stream, const char *uri, Properties *requestHeaders, Properties *responseHeaders) {
	do_get_or_head(stream, uri, requestHeaders, responseHeaders, false);
}

/**
 * Handle DELETE request.
 *
 * @param the socket stream
 * @param uri the request URI
 * @param requestHeaders the request headers
 * @param responseHeaders the response headers
 */
void do_delete(FILE *stream, const char *uri, Properties *requestHeaders, Properties *responseHeaders) {
	// get path to URI in file system
	char filePath[MAXPATHLEN];
	resolveUri(uri, filePath);

	// ensure file exists
	struct stat sb;
	if (stat(filePath, &sb) != 0) {
		sendErrorResponse(stream, 404, "Not Found", responseHeaders);
		return;
	}

	// ensure it is a regular file or an empty directory
	if (S_ISREG(sb.st_mode)) {
		if (unlink(filePath) == 0) {
			sendResponseStatus(stream, 200, "OK");
			sendResponseHeaders(stream, responseHeaders);
		} else {
			sendErrorResponse(stream, 405, "Method Not Allowed", responseHeaders);
		}
	} else if (S_ISDIR(sb.st_mode) && (strendswith(filePath, "/"))) {
		if (rmdir(filePath) == 0) {
			sendResponseStatus(stream, 200, "OK");
			sendResponseHeaders(stream, responseHeaders);
		} else {
			sendErrorResponse(stream, 405, "Method Not Allowed", responseHeaders);
		}
	}

	return;
}

/**
 * Handle PUT request.
 *
 * @param the socket stream
 * @param uri the request URI
 * @param requestHeaders the request headers
 * @param responseHeaders the response headers
 */
void do_put(FILE *stream, const char *uri, Properties *requestHeaders, Properties *responseHeaders) {
	// get path to URI in file system
	char filePath[MAXPATHLEN];
	resolveUri(uri, filePath);

	struct stat sb;
	// need to create a new resource if file doesn't exist
	bool created = (stat(filePath, &sb) != 0);

	// create any intermediate directories
	char pathOfFile[MAXPATHLEN];
	if (getPath(filePath, pathOfFile) != NULL) {
		mkdirs(pathOfFile, 0777);
	}

	// open a stream for filePath to write
	FILE *putStream = fopen(filePath, "w");
	// if the server output file cannot be opened
	if (putStream == NULL) {
		sendErrorResponse(stream, 405, "Method Not Allowed", responseHeaders);
		return;
	}

	// ensure content length is specified in the request header
	char buf[MAXBUF];
	if (findProperty(requestHeaders, 0, "Content-Length", buf) == SIZE_MAX) {
		sendErrorResponse(stream, 411, "Length Required", responseHeaders);
	} else {
		copyFileStreamBytes(stream, putStream, atoi(buf));
	}
	fclose(putStream);

	if (created) { // if the file is created in the server
		sendResponseStatus(stream, 201, "Created");
	} else { // if the named file is overwritten
		sendResponseStatus(stream, 200, "OK");
	}
	sendResponseHeaders(stream, responseHeaders);
}

/**
 * Handle POST request.
 *
 * @param the socket stream
 * @param uri the request URI
 * @param requestHeaders the request headers
 * @param responseHeaders the response headers
 */
void do_post(FILE *stream, const char *uri, Properties *requestHeaders, Properties *responseHeaders) {
	// get path to URI in file system
	char filePath[MAXPATHLEN];
	resolveUri(uri, filePath);

	// create any intermediate directories
	char pathOfFile[MAXPATHLEN];
	if (getPath(filePath, pathOfFile) != NULL) {
		mkdirs(pathOfFile, 0777);
	}

	// open a stream for filePath to write
	FILE *postStream = fopen(filePath, "w");
	// if the server output file cannot be opened
	if (postStream == NULL) {
		sendErrorResponse(stream, 405, "Method Not Allowed", responseHeaders);
		return;
	}

	// ensure content length is specified in the request header
	char buf[MAXBUF];
	if (findProperty(requestHeaders, 0, "Content-Length", buf) == SIZE_MAX) {
		sendErrorResponse(stream, 411, "Length Required", responseHeaders);
	} else {
		copyFileStreamBytes(stream, postStream, atoi(buf));
	}
	fclose(postStream);

	sendResponseStatus(stream, 200, "OK");

	sendResponseHeaders(stream, responseHeaders);
}
