/*
 * http_util.c
 *
 * Functions used to implement http operations.
 *
 *  @since 2019-04-10
 *  @author: Philip Gust
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include "properties.h"
#include "file_util.h"
#include "string_util.h"
#include "http_server.h"


/**
 * Reads request headers from request stream until empty line.
 *
 * @param istream the socket input stream
 * @param request headers
 */
void readRequestHeaders(FILE *istream, Properties *requestHeaders) {
	char buf[MAXBUF];

	while (fgets(buf, MAXBUF, istream) != NULL) {
		// trim newline characters
		trim_newline(buf);

		// empty line marks send of headers
		if (*buf == '\0') {
			break;
		}

		char *p = strchr(buf, ':'); // find name/value delimiter
		if (p != NULL) {
			for (*p++ = '\0'; *p == ' '; p++) {} // skip over leading value whitespace
			if (!putProperty(requestHeaders, buf, p)) {  // save request property
				fprintf(stderr, "readRequestHeaders requestHeaders full\n");
				break;
			}
		}
	}
}

/**
 * Send bytes for status to response output stream.
 *
 * @param ostream the output socket stream
 * @param status the response status
 * @param statusMsg the response message
 */
void sendResponseStatus(FILE *ostream, int status, const char *statusMsg) {
	fprintf(ostream, "%s %d %s %s", server.server_protocol, status, statusMsg, CRLF);
	if (server.debug) {
		fprintf(stderr, "%s %d %s\n", server.server_protocol, status, statusMsg);
	}
}


/**
 * Send bytes for headers to response output stream
 * with terminating blank line.
 *
 * @param responseHeaders the header name value pairs
 * @param responseCharset the response charset
 */
void sendResponseHeaders(FILE *ostream, Properties *responseHeaders) {
	// output headers
	char name[MAX_PROP_NAME], val[MAX_PROP_VAL];
	for (int i = 0; getProperty(responseHeaders, i, name, val); i++) {
		fprintf(ostream, "%s: %s%s", name, val, CRLF);
    	if (server.debug) {
    		fprintf(stderr, "%s: %s\n", name, val);
    	}
	}

	// Send a blank line to indicate the end of the header lines.
	fprintf(ostream, "%s", CRLF);
	if (server.debug) {
		fprintf(stderr, "\n");
	}
}

/**
 * Set error response and error page to the response output stream.
 *
 * @param ostream the output socket stream
 * @param status the response status
 * @param statusMsg the response message
 * @param responseHeaders the response headers
 */
void sendErrorResponse(FILE* ostream, int status, const char *statusMsg, Properties *responseHeaders) {
	sendResponseStatus(ostream, status, statusMsg);

	char errorBody[2*MAXBUF];  // because of data substitution.
	const char *errorPage =
		"<html>"
	    "<head><title>%d %s</title></head>"
	    "<body>%d %s</body></html>";
	sprintf(errorBody, errorPage, status, statusMsg, status, statusMsg);
	FILE *tmpStream = tmpStringFile(errorBody);

	char buf[MAXBUF];
	size_t contentLen = strlen(errorBody);
	sprintf(buf, "%lu", contentLen);
	putProperty(responseHeaders,"Content-Length", buf);
	putProperty(responseHeaders,"Content-type", "text/html");


	// Send the headers
	sendResponseHeaders(ostream, responseHeaders);

	// Send the error page body.
	copyFileStreamBytes(tmpStream, ostream, contentLen);
	fclose(tmpStream);
}

/**
 * Unescape a URI string by replacing %xx with
 * the corresponding character code.
 * @param escUrl the esc URI
 * @param uri the decoded URI
 * @return the URL if successful, NULL if error
 */
char *unescapeUri(const char *escUri, char *uri) {
	char *p = uri;
	while (*escUri) {
		if ( *escUri == '%') {
			int c;
			if (sscanf(escUri, "%%%02x", &c) == 0) {
				return NULL;
			}
			*p++ = (unsigned char)c;
			escUri+=3;
		} else {
			*p++ = *escUri++;
		}
	}
	*p++ = '\0';
	return uri;
}

/**
 * Resolves server URI to file system path.
 * @param uri the request URI
 * @param fspath the file system path
 * @return the file system path
 */
char *resolveUri(const char *uri, char *fspath) {
	strcpy(fspath, server.content_base);
	strcat(fspath, uri);
	return fspath;
}

/**
 * Debug request by printing request and request headers
 *
 * @param request the request line
 * @param requestHeaders the request headers
 */
void debugRequest(const char *request, Properties *requestHeaders) {
	char name[MAXBUF], val[MAXBUF];
	fprintf(stderr, "\n%s\n", request);
	for (int i = 0; getProperty(requestHeaders, i, name, val); i++) {
		fprintf(stderr, "%s: %s\n", name, val);
	}
	fprintf(stderr, "\n");
}

/**
 * Write initial HTML code in file
 * @param fname file to write HTML code
 */
void startHtmlPage(const char *uri, FILE* fname) {
    char htmlData[MAXBSIZE];

    strcpy(htmlData, "<html>\n<head>\n"
                      "  <title>index of ");
    strcat(htmlData, uri);
    strcat(htmlData, "</title></head>\n"
                       "<body>\n"
                       "  <h1>Index of ");
    strcat(htmlData, uri);
    strcat(htmlData, "</h1>\n"
                       "  <table>\n"
                       "  <tr>\n"
                       "    <th valign=\"top\"></th>\n"
                       "    <th>Name</th>\n"
                       "    <th>Last modified</th>\n"
                       "    <th>Size</th>\n"
                       "    <th>File Type</th>\n");
    strcat(htmlData, "  </tr>\n"
                       "  <tr>\n"
                       "    <td colspan=\"5\"><hr></td>\n"
                       "  </tr>\n\n");
    fprintf(fname, htmlData);
}

void tostring(char str[], int num)
{
    int i, rem, len = 0, n;

    n = num;
    while (n != 0)
    {
        len++;
        n /= 10;
    }
    for (i = 0; i < len; i++)
    {
        rem = num % 10;
        num = num / 10;
        str[len - (i + 1)] = rem + '0';
    }
    str[len] = '\0';
}

/**
 * Write file entry to HTML page
 * @param fname file to write data to
 * @param name list file name
 * @param mtime last modification time
 * @param size file size
 * @param mode file type
 */
void makeHtmlEntry(FILE* fname, const char *name, const char *mtime, off_t size, long mode) {

    char htmlData[MAXBSIZE];
    char fileName[MAXBSIZE];
    char fileLink[MAXBSIZE];
    char sizeStr[MAXBSIZE];
    char modeStr[MAXBSIZE];

    tostring(sizeStr, size);

    strcpy(htmlData, "<tr>\n"
                      "    <td></td>\n"
                      "    <td><a href=\"");

    if (strcmp(name, "..") == 0) {
        strcpy(fileName, "Parent Directory");
        strcpy(fileLink, "../");
    } else {
        strcpy(fileName, name);
        strcpy(fileLink, name);

        if (S_ISDIR(mode)) {
            strcat(fileLink, "/");
        }
    }

    if (S_ISDIR(mode)) {
        strcpy(modeStr, "Directory");
    } else if (S_ISLNK(mode)) {
        strcpy(modeStr, "Link");
    } else {
        strcpy(modeStr, "File");
    }

    strcat(htmlData, fileLink);
    strcat(htmlData, "\">");
    strcat(htmlData, fileName);
    strcat(htmlData, "</a></td>\n"
                      "    <td align=\"right\">");
    strcat(htmlData, mtime);
    strcat(htmlData, "</td>\n"
                      "    <td align=\"right\">");
    strcat(htmlData, sizeStr);
    strcat(htmlData, "</td>\n"
                      "    <td>");
    strcat(htmlData, modeStr);
    strcat(htmlData, "</td>\n"
                       "    <td></td>\n"
                       "  </tr>");
    fprintf(fname, htmlData);
}

/**
 * Add end of page HTML text
 * @param fname file to write data to
 */
void endHtmlPage(FILE* fname) {
    char htmlData[MAXBSIZE];

    strcpy(htmlData, "\n"
                     "  <tr>\n"
                     "    <td colspan=\"5\"><hr></td>\n"
                     "  </tr>\n"
                     "</body>\n"
                     "</html>");
    fprintf(fname, htmlData);
}
