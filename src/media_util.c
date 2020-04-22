/*
 * media_util.c
 *
 * Functions for processing media types.
 *
 *  @since 2019-04-10
 *  @author: Philip Gust
 */

#include "media_util.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "string_util.h"
#include "http_server.h"
#include "file_util.h"

/** default media type */
static const char *DEFAULT_MEDIA_TYPE = "application/octet-stream";

/** global Properties instance */
static Properties *props;

/**
 * Read the file extensions and media types into global Properties instance
 *
 * @param filename the name of the "mime.types" file
 * @return the number of entries in Properties
 */
int readMediaTypes(const char *filename) {

    FILE *typeStream = fopen(filename, "r");
	if (typeStream == NULL) {
		return 0;
	}

	// If there is no Properties instance, this method creates one.
	// If there is a Properties instance, this method replaces it with one created from this file.
	if (props != NULL) {
		deleteProperties(props);
	}
	props = newProperties();

	// create a reader to read file line by line
    char *line = NULL;
    size_t len = 0;
    int nprops = 0;

    while (getline(&line, &len, typeStream) != -1) {
    	char *type = strtok(line, " "); // split line by spaces
    	if (strcmp(type, "#") == 0) { // ignore comment
    		continue;
    	}

    	char *token = type;
 	    while (token != NULL) {
 	    	token = strtok(NULL, " ");
 	    	if (token != NULL) {
 	    		// put extension (key) and type (value) into property
 	    		putProperty(props, token, type);
 	    		nprops++;
 	    	}
 	    }
    }
    fclose(typeStream);
    return nprops;
}

/**
 * Return a media type for a given filename.
 *
 * @param filename the name of the file
 * @param mediaType output buffer for mime type
 * @return pointer to media type string
 */
char *getMediaType(const char *filename, char *mediaType)
{
	// special-case directory based on trailing '/'
	if (filename[strlen(filename)-1] == '/') {
		strcpy(mediaType, "text/directory");
		return mediaType;
	}

	// get file extension
    char ext[MAXBUF];
    if (getExtension(filename, ext) == NULL) {
    	// default if no extension
    	strcpy(mediaType, DEFAULT_MEDIA_TYPE);
    	return mediaType;
    }

    // lower-case extension
    strlower(ext, ext);

    // use global Properties instance to get the media type
    // if the file extension is not registered, return the default media type
//    /*const */char *mtstr = DEFAULT_MEDIA_TYPE;

    if (findProperty(props, 0, ext, mediaType) == SIZE_MAX) {
    	strcpy(mediaType, DEFAULT_MEDIA_TYPE);
    }

    return mediaType;
}
