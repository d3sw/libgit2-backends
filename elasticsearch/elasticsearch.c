/*
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * In addition to the permissions in the GNU General Public License,
 * the authors give you unlimited permission to link the compiled
 * version of this file into combinations with other programs,
 * and to distribute those combinations without any restriction
 * coming from the use of this file.  (The General Public License
 * restrictions do apply in other respects; for example, they cover
 * modification of the file, and distribution when not linked into
 * a combined executable.)
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/*
*	cc elasticsearch.c http.c -lgit2 -lcurl -ljson-c -o elasticsearch
*/

#include <math.h>
#include <assert.h>
#include <string.h>
#include <git2.h>
#include <git2/sys/odb_backend.h>
#include <json-c/json.h>
#include "http.h"

#define GIT2_INDEX_NAME "git2_odb"
#define GIT2_TYPE_NAME "git2_odb"

typedef struct {
	git_odb_backend parent;
	const char *hostname;
	const char *index_uri;
	const char *type_uri;
} elasticsearch_backend;

char *join(const char* s[], int size);
char *concat(const char* s1, const char* s2);

char *create_document(char *type_uri, char *id, int size, int type, char *data);
char *create_index(char *index_uri);
char *get_document(char *type_uri, char *id);
char *get_index(char *index_uri);
char *update_document(char *type_uri, char *id, int size, int type, char *data);
char *delete_document(char *type_uri, char *id);
char *delete_index(char *index_uri);

int elasticsearch_backend__read_header(size_t *len_p, git_otype *type_p, git_odb_backend *_backend, const git_oid *oid){
	elasticsearch_backend *backend;
	int error;

	assert(len_p && type_p && _backend && oid);

	backend = (elasticsearch_backend *)_backend;
	error = GIT_ERROR;

	/* get the document */
	const char *query = join((const char*[3]){backend->index_uri,"/",(char *)oid->id},3);
	const char *content = get_http_json(query);
	
	/* fail if the object wasn't returned */
	if (strcmp(content,"")==0){
		return GIT_ENOTFOUND;
	}

	/* set parameter values from document type and size properties */
    json_object *json;                                      /* json response */
    enum json_tokener_error jerr = json_tokener_success;    /* json parse error */
	json = json_tokener_parse_verbose(content, &jerr);

	struct json_object *type_json;
	struct json_object *size_json;

	json_object_object_get_ex(json, "type", &type_json);
	json_object_object_get_ex(json, "size", &size_json);

	const char *type_c = json_object_to_json_string(type_json);
	const char *size_c = json_object_to_json_string(size_json);

	*type_p = (git_otype)atoi(type_c);
	*len_p = (size_t)atoi(size_c);
	error = GIT_OK;

	return error;
}

int elasticsearch_backend__read(void **data_p, size_t *len_p, git_otype *type_p, git_odb_backend *_backend, const git_oid *oid){
	return 0;
}

int elasticsearch_backend__read_prefix(git_oid *out_oid, void **data_p, size_t *len_p, git_otype *type_p, git_odb_backend *_backend,
	const git_oid *short_oid, size_t len) {
	return 0;
}
	
int elasticsearch_backend__exists(git_odb_backend *_backend, const git_oid *oid){
	return 0;
}
	
int elasticsearch_backend__write(git_odb_backend *_backend, const git_oid *id, const void *data, size_t len, git_otype type){
	return 0;
}

void elasticsearch_backend__free(git_odb_backend *_backend){
	elasticsearch_backend *backend;
	assert(_backend);
	backend = (elasticsearch_backend *)_backend;

	free(backend);
}

static int init_db(elasticsearch_backend *backend){
	int result;

	/* check whether or not the index exists */
	const char* content = get_http_json(backend->index_uri);

    json_object *json;                                      /* json response */
    enum json_tokener_error jerr = json_tokener_success;    /* json parse error */
	json = json_tokener_parse_verbose(content, &jerr);

	struct json_object *status_json;
	json_object_object_get_ex(json, "status", &status_json);
	const char *status = json_object_to_json_string(status_json);
	
	/* create index if not found */
	if (strcmp(status,"404") == 0){
		/*
			"\"oid\" : { " 
				"\"type\" : \"string\", "
				"\"index\" : \"not_analyzed\" "			
			"}, "		
		*/
		static const char *mapping =
			"{"
				"\"mappings\" : { "
					"\"git2_odb\" : { "
						"\"properties\" : { "
							"\"type\" : { \"type\" : \"integer\" }, "
							"\"size\" : { \"type\" : \"integer\" }, "
							"\"data\" : { \"type\" : \"binary\" } "
						"} "
					"} "
				"} "
			"}";
		put_http_json(backend->index_uri, mapping);
	}

	return result;
}

int git_odb_backend_elasticsearch(git_odb_backend **backend_out, const char *hostname){
	elasticsearch_backend *backend;
	int result;

	/* allocate memory for the backend object */
	backend = calloc(1, sizeof(elasticsearch_backend));
	if (backend == NULL) {
		giterr_set_oom();
		return GIT_ERROR;
	}

	/* set the backend hostname */
	backend->hostname = hostname;
	backend->index_uri = join((const char*[4]){"http://",hostname,"/",GIT2_INDEX_NAME},4);
	backend->type_uri = join((const char*[3]){backend->index_uri,"/",GIT2_TYPE_NAME},3);

	/* initialize the elasticsearch instance */
	result = init_db(backend);

	/* set odb functions for the backend */
	if(result == 0) {
		backend->parent.version = GIT_ODB_BACKEND_VERSION;
		backend->parent.read = &elasticsearch_backend__read;
		backend->parent.read_prefix = &elasticsearch_backend__read_prefix;
		backend->parent.read_header = &elasticsearch_backend__read_header;
		backend->parent.write = &elasticsearch_backend__write;
		backend->parent.exists = &elasticsearch_backend__exists;
		backend->parent.free = &elasticsearch_backend__free;

		*backend_out = (git_odb_backend *)backend;
	}

	return result;
}

int main(int argc, char *argv[]) {
	/*
	elasticsearch_backend *db;
	int result = 0;

	db = calloc(1, sizeof(elasticsearch_backend));
	if (db == NULL) {
		giterr_set_oom();
		return GIT_ERROR;
	}

	db->hostname = "logs-db.service.owf-dev:9200";
	db->index_uri = join((const char*[4]){"http://",db->hostname,"/",GIT2_INDEX_NAME},4);
	db->type_uri = join((const char*[3]){db->index_uri,"/",GIT2_TYPE_NAME},3);
	*/

	char *hostname = "logs-db.service.owf-dev:9200";
	char *test_num;
	char *index_name;
	char *index_uri;
	char *type_uri;
	char *document_id;
	char *document_data;

	printf("\n%s\n\n","Starting Tests...");

	/* create index - already exists */

	index_uri = join((const char*[3]){"http://",hostname,"/test1"},3);	
	create_index(index_uri);
	printf("%s\n\n%s\n\n","Create Index - Already Exists: ", create_index(index_uri));
	delete_index(index_uri);

	/* create index - standard */
	index_uri = join((const char*[3]){"http://",hostname,"/test2"},3);	
	printf("%s\n\n%s\n\n","Create Index - Standard: ", create_index(index_uri));
	delete_index(index_uri);

	/* create document - already exists */
	index_uri = join((const char*[3]){"http://",hostname,"/test3"},3);
	type_uri = join((const char*[3]){index_uri,"/","test3"},3);	
	create_index(index_uri);
	create_document(type_uri,"test3-id",3,6,"test3-data");
	printf("%s\n\n%s\n\n","Create Document - Already Exists: ", create_document(type_uri,"test3-id",3,6,"test3-data"));
	delete_index(index_uri);

	/* create document - standard */
	index_uri = join((const char*[3]){"http://",hostname,"/test4"},3);
	type_uri = join((const char*[3]){index_uri,"/","test4"},3);	
	create_index(index_uri);
	printf("%s\n\n%s\n\n","Create Document - Standard: ", create_document(type_uri,"test4-id",4,8,"test4-data"));
	delete_index(index_uri);

	/* get document - doesn't exist */


	/* get document - standard */
	/* get index - doesn't exist */
	/* get index - standard */
	/* update document - doesn't exist */
	/* update document - standard */
	/* delete document - doesn't exit */
	/* delete document - standard */
	/* delete index - deoesn't exist */
	/* delete index - standard */

}

char *create_document(char *type_uri,char *id, int size, int type, char *data){
	int type_len = (int)((ceil(log10(type))+1)*sizeof(char));
	int size_len = (int)((ceil(log10(size))+1)*sizeof(char));
	char type_c[type_len];
	char size_c[size_len];
	sprintf(type_c,"%d", type);
	sprintf(size_c,"%d", size);

	const char *doc = join((const char*[11]){
		"{ ",
			"\"type\" : ", type_c, ", ",
			"\"size\" : ", size_c, ", ",
			"\"data\" : \"", data, "\"",
		"}"
	},11);
	return put_http_json(join((const char*[3]){type_uri,"/",id},3),doc);
}

char *create_index(char *index_uri){
	static const char *mapping =
		"{ "
			"\"mappings\" : { "
				"\"git2_odb\" : { "
					"\"properties\" : { "
						"\"type\" : { \"type\" : \"integer\" }, "
						"\"size\" : { \"type\" : \"integer\" }, "
						"\"data\" : { \"type\" : \"binary\" } "
					"} "
				"} "
			"} "
		"}";
	return put_http_json(index_uri, mapping);
}

char *get_document(char *type_uri, char *id) {
	return get_http_json(join((const char*[3]){type_uri,"/",id},3));
}

char *get_index(char *index_uri){
	return get_http_json(index_uri);
}

char *update_document(char *type_uri, char *id, int size, int type, char *data){
	return create_document(type_uri, id, size, type, data);
}

char *delete_document(char *type_uri, char *id){
	return delete_http_json(join((const char*[3]){type_uri,"/",id},3));
}

char *delete_index(char *index_uri){
	return delete_http_json(index_uri);
}

char *concat(const char* s1, const char* s2){
    char* result = malloc(strlen(s1) + strlen(s2) + 1);

    if (result) // thanks @pmg
    {
        strcpy(result, s1);
        strcat(result, s2);
    }

    return result;
}

char *join(const char* s[], int size){
	char* result = "";
	for(int i=0;i<size;i++)
	{
		result = concat(result, s[i]);
	}
	return result;
}