
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <media_reg.h>



BMediaKeyLookup::BMediaKeyLookup(const char *name)
{
	m_namespace = 0;
	m_client = -1;
	if (!name) {
		m_server = B_BAD_VALUE;
		return;
	}
	m_server = find_port(REGISTRY_PORT_NAME);
	if (m_server >= 0) {
		m_client = create_port(1, "_MediaKeyLookup_Reply_");
		if (m_client < 0) {
			m_server = m_client;
		}
	}
	m_namespace = strdup(name);
	if (!m_namespace) {
		m_server = B_BAD_VALUE;
		m_client = -1;
	}
}


BMediaKeyLookup::~BMediaKeyLookup()
{
	//	tell server we're croaking
	if (m_server >= 0) {
		key_request req;
		req.signoff.client = m_client;
		(void)write_port_etc(m_server, keySignoff, &req, sizeof(req), B_TIMEOUT, KEY_TIMEOUT);
	}
	//	clean up
	free(m_namespace);
	if (m_client >= 0) {
		delete_port(m_client);
		m_server = -1;
	}
}

status_t 
BMediaKeyLookup::InitCheck()
{
	return m_server > 0 ? 0 : m_server;
}

const char *
BMediaKeyLookup::Namespace() const
{
	return m_namespace;
}

status_t 
BMediaKeyLookup::RegisterKey(media_key_type key, type_code type, size_t size, const void *data)
{
	if (m_server < 0) return m_server;
	key_request req;
	strncpy(req.registration.name, m_namespace, sizeof(req.registration.name));
	req.registration.key = key;
	req.registration.client = m_client;
	req.registration.type = type;
	req.registration.size = size;
	status_t err = write_port_etc(m_server, keyRegistration, &req, sizeof(req), B_TIMEOUT, KEY_TIMEOUT);
	if (err < B_OK) return err;
	int32 code;
	key_reply rep;
	err = read_port_etc(m_client, &code, &rep, sizeof(rep), B_TIMEOUT, KEY_TIMEOUT);
	if (err < B_OK) {	//	invalidate connection if we get out-of-sync with server
error:
		delete_port(m_client);
		free(m_namespace);
		m_namespace = 0;
		m_client = -1;
		m_server = err;
		return err;
	}
	if (code < 0) return code;
	if (key != rep.registration.key) {
		err = B_KEY_SERVER_NOT_RUNNING;
		goto error;
	}
	err = write_port_etc(m_server, rep.registration.data_code, data, size, B_TIMEOUT, KEY_TIMEOUT);
	if (err < B_OK) return err;	//	don't invalidate connection just because this key couldn't get written
	return B_OK;
}

status_t 
BMediaKeyLookup::RemoveKey(media_key_type key)
{
	if (m_server < 0) return m_server;
	key_request req;
	strncpy(req.removal.name, m_namespace, sizeof(req.removal.name));
	req.removal.key = key;
	req.removal.client = m_client;
	req.removal.remove_all = false;
	status_t err = write_port_etc(m_server, keyRemoval, &req, sizeof(req), B_TIMEOUT, KEY_TIMEOUT);
	if (err < B_OK) return err;
	int32 code;
	key_reply rep;
	err = read_port_etc(m_client, &code, &rep, sizeof(rep), B_TIMEOUT, KEY_TIMEOUT);
	if (err < B_OK) {	//	invalidate connection if we get out-of-sync with server
error:
		delete_port(m_client);
		free(m_namespace);
		m_namespace = 0;
		m_client = -1;
		m_server = err;
		return err;
	}
	if (code < 0) return code;
	if (key != rep.removal.key) {
		err = B_KEY_SERVER_NOT_RUNNING;
		goto error;
	}
	return B_OK;
}

status_t 
BMediaKeyLookup::RemoveAllKeys()
{
	if (m_server < 0) return m_server;
	key_request req;
	strncpy(req.removal.name, m_namespace, sizeof(req.removal.name));
	req.removal.key = 0;
	req.removal.client = m_client;
	req.removal.remove_all = true;
	status_t err = write_port_etc(m_server, keyRemoval, &req, sizeof(req), B_TIMEOUT, KEY_TIMEOUT);
	if (err < B_OK) return err;
	int32 code;
	key_reply rep;
	err = read_port_etc(m_client, &code, &rep, sizeof(rep), B_TIMEOUT, KEY_TIMEOUT);
	if (err < B_OK) {	//	invalidate connection if we get out-of-sync with server
		delete_port(m_client);
		free(m_namespace);
		m_namespace = 0;
		m_client = -1;
		m_server = err;
		return err;
	}
	if (code < 0) return code;
	return B_OK;
}

status_t 
BMediaKeyLookup::FindKey(media_key_type key, type_code *out_type, size_t *out_size)
{
	if (m_server < 0) return m_server;
	key_request req;
	strncpy(req.iteration.name, m_namespace, sizeof(req.iteration.name));
	req.iteration.key = key;
	req.iteration.client = m_client;
	status_t err = write_port_etc(m_server, keyIteration, &req, sizeof(req), B_TIMEOUT, KEY_TIMEOUT);
	if (err < B_OK) return err;
	int32 code;
	key_reply rep;
	err = read_port_etc(m_client, &code, &rep, sizeof(rep), B_TIMEOUT, KEY_TIMEOUT);
	if (err < B_OK) {	//	invalidate connection if we get out-of-sync with server
		delete_port(m_client);
		free(m_namespace);
		m_namespace = 0;
		m_client = -1;
		m_server = err;
		return err;
	}
	if (code < 0) return code;
	if (key != rep.iteration.key) {
		return B_KEY_NOT_FOUND;
	}
	if (out_type) *out_type = rep.iteration.type;
	if (out_size) *out_size = rep.iteration.size;
	return B_OK;
}

status_t 
BMediaKeyLookup::FindNextKey(media_key_type min_key, media_key_type *out_actual_key, type_code *out_type, size_t *out_size)
{
	if (m_server < 0) return m_server;
	key_request req;
	strncpy(req.iteration.name, m_namespace, sizeof(req.iteration.name));
	req.iteration.key = min_key;
	req.iteration.client = m_client;
	status_t err = write_port_etc(m_server, keyIteration, &req, sizeof(req), B_TIMEOUT, KEY_TIMEOUT);
	if (err < B_OK) return err;
	int32 code;
	key_reply rep;
	err = read_port_etc(m_client, &code, &rep, sizeof(rep), B_TIMEOUT, KEY_TIMEOUT);
	if (err < B_OK) {	//	invalidate connection if we get out-of-sync with server
		delete_port(m_client);
		free(m_namespace);
		m_namespace = 0;
		m_client = -1;
		m_server = err;
		return err;
	}
	if (code < 0) return code;
	if (out_actual_key) *out_actual_key = rep.iteration.key;
	if (out_type) *out_type = rep.iteration.type;
	if (out_size) *out_size = rep.iteration.size;
	return B_OK;
}

ssize_t 
BMediaKeyLookup::RetrieveKey(media_key_type key, type_code type, size_t in_buf_size, void *buffer)
{
	if (m_server < 0) return m_server;
	key_request req;
	strncpy(req.retrieval.name, m_namespace, sizeof(req.retrieval.name));
	req.retrieval.key = key;
	req.retrieval.client = m_client;
	req.retrieval.type = type;
	req.retrieval.size = in_buf_size;
	status_t err = write_port_etc(m_server, keyRetrieval, &req, sizeof(req), B_TIMEOUT, KEY_TIMEOUT);
	if (err < B_OK) return err;
	int32 code;
	err = read_port_etc(m_client, &code, buffer, in_buf_size, B_TIMEOUT, KEY_TIMEOUT);
	if (err < B_OK) {	//	invalidate connection if we get out-of-sync with server
error:
		delete_port(m_client);
		free(m_namespace);
		m_namespace = 0;
		m_client = -1;
		m_server = err;
		return err;
	}
	if (code < 0) return code;
	return err;		//	size of actual data returned
}

