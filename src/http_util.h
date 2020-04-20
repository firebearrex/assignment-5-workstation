/*
 * http_util.h
 *
 * Functions used to implement http operations.
 *
 *  @since 2019-04-10
 *  @author: Philip Gust
 */

#ifndef HTTP_UTIL_H_
#define HTTP_UTIL_H_

#include "properties.h"

/**
 * Reads request headers from request stream until empty line.
 *
 * @param istream the socket input stream
 * @param request headers
 * @throws IOException if error occurs
 */
void readRequestHeaders(FILE *istream, Properties *requestHeader);

/**
 * Send bytes for status to response output stream.
 *
 * @param status the response status
 * @param statusMsg the response message
 * @param responseCharset the response charset
 * @throws IOException if error occurs
 */
void sendResponseStatus(FILE *ostream, int status, const char *statusMsg);

/**
 * Send bytes for headers to response output stream.
 *
 * @param responseHeaders the header name value pairs
 * @param responseCharset the response charset
 * @throws IOException if errors occur
 */
void sendResponseHeaders(FILE *ostream, Properties *responseHeaders);

/**
 * Set error response and error page to the response output stream.
 *
 * @param ostream the output socket stream
 * @param status the response status
 * @param statusMsg the response message
 * @param responseHeaders the response headers
 */
void sendErrorResponse(FILE* ostream, int status, const char *statusMsg, Properties *responseHeaders);

/**
 * Unescape a URI string by replacing %xx with
 * the corresponding character code.
 * @param escUrl the esc URI
 * @param uri the decoded URI
 * @return the URL if successful, NULL if error
 */
char *unescapeUri(const char *escUri, char *uri);

/**
 * Resolves server URI to file system path.
 * @param uri the request URI
 * @param fspath the file system path
 * @return the file system path
 */
char *resolveUri(const char *uri, char *fspath);

/**
 * Debug request by printing request and request headers
 *
 * @param request the request line
 * @param requestHeaders the request headers
 */
void debugRequest(const char *request, Properties *requestHeaders);

/**
 * Write initial HTML code in file
 * @param fname file to write HTML code
 */
void startHtmlPage(const char *uri, FILE* fname);

/**
 * Write file entry to HTML page
 * @param fname file to write data to
 * @param name list file name
 * @param mtime last modification time
 * @param size file size
 * @param mode file type
 */
void makeHtmlEntry(FILE* fname, const char *name, const char *mtime, off_t size, long mode);

/**
 * Add end of page HTML text
 * @param fname file to write data to
 */
void endHtmlPage(FILE* fname);

#endif /* HTTP_UTIL_H_ */
