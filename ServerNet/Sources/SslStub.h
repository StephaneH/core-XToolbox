/*
* This file is part of Wakanda software, licensed by 4D under
*  (i) the GNU General Public License version 3 (GNU GPL v3), or
*  (ii) the Affero General Public License version 3 (AGPL v3) or
*  (iii) a commercial license.
* This file remains the exclusive property of 4D and/or its licensors
* and is protected by national and international legislations.
* In any event, Licensee's compliance with the terms and conditions
* of the applicable license constitutes a prerequisite to any use of this file.
* Except as otherwise expressly stated in the applicable license,
* such license does not include any other license or rights on this file,
* 4D's and/or its licensors' trademarks and/or other proprietary rights.
* Consequently, no title, copyright or other proprietary rights
* other than those specified in the applicable license is granted.
*/
#ifndef __SNET_SSL_STUB__
#define __SNET_SSL_STUB__


#if defined(_WIN32)

	#define OPENSSL_SYS_WIN32

	#include <openssl/ssl.h>
	#include <openssl/err.h>
	#include <openssl/rand.h>
	#include <openssl/pem.h>
	#include <openssl/X509.h>

	//jmo - For some reason, X509_NAME is not defined properly (defined to a constant in wincrypt.h)
	//		A (stupid but efficient) workaround is to redo what's already done in X509.h

	#undef X509_NAME					//Get rid of wincrypt.h constant, save a warning
	#define X509_NAME X509_name_st		//Use OpenSSL struct for X509 names

	#define SSLSTUB			snet_ssl_stub
	#define SNET_STDCALL	__stdcall
	#define SNET_CDECL		__cdecl

	#define SK_X509_POP_FREE_4D(st) SSLSTUB::sk_X509_pop_free_4D(st)

#else

	#include <openssl/ssl.h>
	#include <openssl/err.h>
	#include <openssl/rand.h>

	#define SSLSTUB
	#define SNET_STDCALL
	#define SNET_CDECL
	#define SSLCONST

	#define SK_X509_POP_FREE_4D(st) ::sk_X509_pop_free(st, ::X509_free)

#endif


namespace snet_ssl_stub
{
	#define SSL_CTX_set_options(ctx,op) \
		SSL_CTX_ctrl((ctx),SSL_CTRL_OPTIONS,(op),NULL)


	BIO*					SNET_STDCALL	BIO_new							(BIO_METHOD* type);
	int						SNET_STDCALL	BIO_free						(BIO* a);
	BIO_METHOD*				SNET_STDCALL	BIO_s_mem						(); 
	int						SNET_STDCALL	BIO_write						(BIO* b, const void* buf, int len);

	int						SNET_STDCALL	CRYPTO_num_locks				();
	void					SNET_STDCALL	CRYPTO_set_id_callback			(unsigned long (SNET_CDECL *id_function)());
	void					SNET_STDCALL	CRYPTO_set_locking_callback		(void (SNET_CDECL *locking_function)(int mode, int n, const char* file, int line));

	void					SNET_STDCALL	ERR_clear_error					();
	void					SNET_STDCALL	ERR_error_string_n				(unsigned long e, char* buf, size_t len);
	void					SNET_STDCALL	ERR_free_strings				();
	unsigned long			SNET_STDCALL	ERR_get_error					();
	unsigned long			SNET_STDCALL	ERR_peek_error					();

	int						SNET_STDCALL	EVP_DigestInit_ex				(EVP_MD_CTX* ctx, const EVP_MD* type, ENGINE* impl);
	int						SNET_STDCALL	EVP_VerifyUpdate				(EVP_MD_CTX* ctx, const void* d, unsigned int cnt);
	int						SNET_STDCALL	EVP_MD_CTX_cleanup				(EVP_MD_CTX* ctx);
	EVP_MD_CTX*				SNET_STDCALL	EVP_MD_CTX_create				();
	void					SNET_STDCALL	EVP_MD_CTX_destroy				(EVP_MD_CTX* ctx);
	void					SNET_STDCALL	EVP_PKEY_free					(EVP_PKEY *key);
	int						SNET_STDCALL	EVP_VerifyFinal					(EVP_MD_CTX* ctx, const unsigned char* sigbuf, unsigned int siglen, EVP_PKEY* pkey);

	const EVP_MD*			SNET_STDCALL	EVP_sha1						();
	const EVP_MD*			SNET_STDCALL	EVP_md5							();

	typedef int				SNET_CDECL		pem_password_cb					(char* buf, int size, int rwflag, void* userdata);

	EVP_PKEY*				SNET_STDCALL	PEM_read_bio_PrivateKey			(BIO* bp, EVP_PKEY** x, pem_password_cb* cb, void* u);
	RSA*					SNET_STDCALL	PEM_read_bio_RSAPrivateKey		(BIO* bp, RSA** rsa, pem_password_cb* cb, void* u);
	X509*					SNET_STDCALL	PEM_read_bio_X509				(BIO* bp, X509** x, pem_password_cb* cb, void* u);

	void					SNET_STDCALL	RSA_free						(RSA* rsa);
	int						SNET_STDCALL	RSA_private_encrypt				(int flen, unsigned char *from, unsigned char *to, RSA *rsa, int padding);
	int						SNET_STDCALL	RSA_size						(const RSA* rsa);

	int						SNET_STDCALL	SSL_CTX_ctrl					(SSL_CTX* ctx, int cmd, long larg, void *parg);
	void					SNET_STDCALL	SSL_CTX_free					(SSL_CTX* ctx);
	int						SNET_STDCALL	SSL_CTX_load_verify_locations	(SSL_CTX* ctx, const char* CAfile, const char* CApath);
	SSL_CTX*				SNET_STDCALL	SSL_CTX_new						(const SSL_METHOD* meth);
	int						SNET_STDCALL	SSL_CTX_use_RSAPrivateKey		(SSL_CTX* ctx, RSA* rsa);
	int						SNET_STDCALL	SSL_CTX_use_certificate			(SSL_CTX* ctx, X509* x);
	void					SNET_STDCALL	SSL_CTX_set_client_CA_list		(SSL_CTX* ctx, STACK_OF(X509_NAME)* list);
#if ARCH_64	//jmo - bug : session_id_context en 32bit
	int						SNET_STDCALL	SSL_CTX_set_session_id_context	(SSL_CTX* ctx, const unsigned char* sid_ctx, unsigned int sid_ctx_len);
#endif
	void					SNET_STDCALL	SSL_CTX_set_verify_depth		(SSL_CTX* ctx, int depth);
	int						SNET_STDCALL	SSL_CTX_set_cipher_list			(SSL_CTX* ctx, const char *str);

    const char*             SNET_STDCALL    SSL_get_cipher_list             (SSL* ssl, int priority);
    
	int						SNET_STDCALL	SSL_add_client_CA				(SSL* ssl, X509* x);
	void					SNET_STDCALL	SSL_free						(SSL* ssl);
	int						SNET_STDCALL	SSL_get_error					(const SSL* ssl, int ret);
	int						SNET_STDCALL	SSL_get_fd						(const SSL* ssl);
	int						SNET_STDCALL	SSL_library_init				();
	STACK_OF(X509_NAME)*	SNET_STDCALL	SSL_load_client_CA_file			(const char* file);
	void					SNET_STDCALL	SSL_load_error_strings			();
	SSL*					SNET_STDCALL	SSL_new							(SSL_CTX* ctx);
	int						SNET_STDCALL	SSL_pending						(const SSL *ssl);
	int						SNET_STDCALL	SSL_read						(SSL* ssl, void* buf, int num);
	void					SNET_STDCALL	SSL_set_connect_state			(SSL* ssl);
	void					SNET_STDCALL	SSL_set_accept_state			(SSL* ssl);
	int						SNET_STDCALL	SSL_set_fd						(SSL* ssl, int fd);
	void					SNET_STDCALL	SSL_set_verify					(SSL* ssl, int mode, int (SNET_CDECL *verify_callback)(int, X509_STORE_CTX*));
	int						SNET_STDCALL	SSL_shutdown					(SSL* ssl);
	int						SNET_STDCALL	SSL_use_certificate				(SSL* ssl, X509* x);
	int						SNET_STDCALL	SSL_use_RSAPrivateKey			(SSL* ssl, RSA* rsa);
	int						SNET_STDCALL	SSL_write						(SSL* ssl, const void* buf, int num);

	const SSL_METHOD*		SNET_STDCALL	SSLv23_method					();

	void					SNET_STDCALL	X509_free						(X509* x);
	void					SNET_STDCALL	sk_X509_pop_free_4D				(STACK_OF(X509)* st);
	X509_NAME*				SNET_STDCALL	X509_get_subject_name			(X509* a);
	EVP_PKEY*				SNET_STDCALL	X509_get_pubkey					(X509* x);

	char*					SNET_STDCALL	X509_NAME_oneline				(X509_NAME* a, char* buf, int size);

	X509*					SNET_STDCALL	X509_STORE_CTX_get_current_cert	(X509_STORE_CTX* ctx);	
	int						SNET_STDCALL	X509_STORE_CTX_get_error		(X509_STORE_CTX* ctx);
	int						SNET_STDCALL	X509_STORE_CTX_get_error_depth  (X509_STORE_CTX* ctx);

	// Used by VSslDelegate::HandShake() (SSJS socket implementation).
	int						SNET_STDCALL	SSL_connect						(SSL *ssl);
}


#endif
