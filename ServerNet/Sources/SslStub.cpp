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
#include "SslStub.h"

BIO* SNET_STDCALL SSLSTUB::BIO_new(BIO_METHOD* type)
{
	return ::BIO_new(type);
}

int SNET_STDCALL SSLSTUB::BIO_free(BIO* a)
{
	return ::BIO_free(a);
}

BIO_METHOD* SNET_STDCALL SSLSTUB::BIO_s_mem()
{
	return ::BIO_s_mem(); 
}

int SNET_STDCALL SSLSTUB::BIO_write(BIO* b, const void* buf, int len)
{
	return ::BIO_write(b, buf, len);
}

int SNET_STDCALL SSLSTUB::CRYPTO_num_locks()
{
	return ::CRYPTO_num_locks();
}

void SNET_STDCALL SSLSTUB::CRYPTO_set_id_callback(unsigned long (SNET_CDECL *id_function)())
{
	return ::CRYPTO_set_id_callback(id_function);
}

void SNET_STDCALL SSLSTUB::CRYPTO_set_locking_callback(void (SNET_CDECL *locking_function)(int mode, int n, const char* file, int line))
{
	return ::CRYPTO_set_locking_callback(locking_function);
}

void SNET_STDCALL SSLSTUB::ERR_clear_error()
{
	return ::ERR_clear_error();
}

void SNET_STDCALL SSLSTUB::ERR_error_string_n(unsigned long e, char* buf, size_t len)
{
	return ::ERR_error_string_n(e, buf, len);
}

void SNET_STDCALL SSLSTUB::ERR_free_strings()
{
	return ::ERR_free_strings();
}

unsigned long SNET_STDCALL SSLSTUB::ERR_get_error()
{
	return ::ERR_get_error();
}

unsigned long SNET_STDCALL SSLSTUB::ERR_peek_error()
{
	return ::ERR_peek_error();
}

int	SNET_STDCALL SSLSTUB::EVP_DigestInit_ex(EVP_MD_CTX* ctx, const EVP_MD* type, ENGINE* impl)
{
	return ::EVP_DigestInit_ex(ctx, type, impl);
}

int	SNET_STDCALL SSLSTUB::EVP_VerifyUpdate(EVP_MD_CTX* ctx, const void* d, unsigned int cnt)
{
	return ::EVP_VerifyUpdate(ctx, d, cnt);
}

int SNET_STDCALL SSLSTUB::EVP_MD_CTX_cleanup(EVP_MD_CTX* ctx)
{
	return ::EVP_MD_CTX_cleanup(ctx);
}

EVP_MD_CTX* SNET_STDCALL SSLSTUB::EVP_MD_CTX_create()
{
	return ::EVP_MD_CTX_create();
}

void SNET_STDCALL SSLSTUB::EVP_MD_CTX_destroy(EVP_MD_CTX* ctx)
{
	return ::EVP_MD_CTX_destroy(ctx);
}

void SNET_STDCALL SSLSTUB::EVP_PKEY_free(EVP_PKEY *key)
{
	return ::EVP_PKEY_free(key);
}

int SNET_STDCALL SSLSTUB::EVP_VerifyFinal(EVP_MD_CTX* ctx, const unsigned char* sigbuf, unsigned int siglen, EVP_PKEY* pkey)
{
	return ::EVP_VerifyFinal(ctx, sigbuf, siglen, pkey);
}

const EVP_MD* SNET_STDCALL SSLSTUB::EVP_sha1()
{
	return ::EVP_sha1();
}

const EVP_MD* SNET_STDCALL SSLSTUB::EVP_md5()
{
	return ::EVP_md5();
}

EVP_PKEY* SNET_STDCALL SSLSTUB::PEM_read_bio_PrivateKey(BIO* bp, EVP_PKEY** x, pem_password_cb* cb, void* u)
{
	return ::PEM_read_bio_PrivateKey(bp, x, cb, u);
}

RSA* SNET_STDCALL SSLSTUB::PEM_read_bio_RSAPrivateKey(BIO* bp, RSA** rsa, pem_password_cb* cb, void* u)
{
	return ::PEM_read_bio_RSAPrivateKey(bp, rsa, cb, u);
}

X509* SNET_STDCALL SSLSTUB::PEM_read_bio_X509(BIO* bp, X509** x, pem_password_cb* cb, void* u)
{
	return ::PEM_read_bio_X509(bp, x, cb, u);
}

void SNET_STDCALL SSLSTUB::RSA_free(RSA* rsa)
{
	return ::RSA_free(rsa);
}

int SNET_STDCALL SSLSTUB::RSA_private_encrypt(int flen, unsigned char *from, unsigned char *to, RSA *rsa, int padding)
{
	return ::RSA_private_encrypt(flen, from, to, rsa, padding);
}

int	SNET_STDCALL SSLSTUB::RSA_size(const RSA* rsa)
{
	return ::RSA_size(rsa);
}

int	SNET_STDCALL SSLSTUB::SSL_CTX_ctrl(SSL_CTX* ctx, int cmd, long larg, void *parg)
{
	return ::SSL_CTX_ctrl(ctx, cmd, larg, parg);
}

void SNET_STDCALL SSLSTUB::SSL_CTX_free(SSL_CTX* ctx)
{
	return ::SSL_CTX_free(ctx);
}

int SNET_STDCALL SSLSTUB::SSL_CTX_load_verify_locations(SSL_CTX* ctx, const char* CAfile, const char* CApath)
{
	return ::SSL_CTX_load_verify_locations(ctx, CAfile, CApath);
}

SSL_CTX* SNET_STDCALL SSLSTUB::SSL_CTX_new(const SSL_METHOD* meth)
{
	return ::SSL_CTX_new(meth);
}

int SNET_STDCALL SSLSTUB::SSL_pending(const SSL *ssl)
{
	return ::SSL_pending(ssl);
}

int SNET_STDCALL SSLSTUB::SSL_CTX_use_RSAPrivateKey(SSL_CTX* ctx, RSA* rsa)
{
	return ::SSL_CTX_use_RSAPrivateKey(ctx, rsa);
}

int SNET_STDCALL SSLSTUB::SSL_CTX_use_certificate(SSL_CTX* ctx, X509* x)
{
	return ::SSL_CTX_use_certificate(ctx, x);
}

void SNET_STDCALL SSLSTUB::SSL_CTX_set_client_CA_list(SSL_CTX* ctx, STACK_OF(X509_NAME)* list)
{
	return ::SSL_CTX_set_client_CA_list(ctx, list);
}

#if ARCH_64	//jmo - bug : session_id_context en 32bit
int SNET_STDCALL SSLSTUB::SSL_CTX_set_session_id_context(SSL_CTX* ctx, const unsigned char* sid_ctx, unsigned int sid_ctx_len)
{
	return ::SSL_CTX_set_session_id_context(ctx, sid_ctx, sid_ctx_len);
}
#endif

void SNET_STDCALL SSLSTUB::SSL_CTX_set_verify_depth(SSL_CTX* ctx, int depth)
{
	return ::SSL_CTX_set_verify_depth(ctx, depth);
}

int SNET_STDCALL SSLSTUB::SSL_add_client_CA(SSL* ssl, X509* x)
{
	return ::SSL_add_client_CA(ssl, x);
}

void SNET_STDCALL SSLSTUB::SSL_free(SSL* ssl)
{
	return ::SSL_free(ssl);
}

int SNET_STDCALL SSLSTUB::SSL_get_error(const SSL* ssl, int ret)
{
	return ::SSL_get_error(ssl, ret);
}

int SNET_STDCALL SSLSTUB::SSL_get_fd(const SSL* ssl)
{
	return ::SSL_get_fd(ssl);
}

int SNET_STDCALL SSLSTUB::SSL_library_init()
{
	return ::SSL_library_init();
}

STACK_OF(X509_NAME)* SNET_STDCALL SSLSTUB::SSL_load_client_CA_file(const char* file)
{
	return ::SSL_load_client_CA_file(file);
}

void SNET_STDCALL SSLSTUB::SSL_load_error_strings()
{
	return ::SSL_load_error_strings();
}

SSL* SNET_STDCALL SSLSTUB::SSL_new(SSL_CTX* ctx)
{
	return ::SSL_new(ctx);
}

int SNET_STDCALL SSLSTUB::SSL_read(SSL* ssl, void* buf, int num)
{
	return ::SSL_read(ssl, buf, num);
}

void SNET_STDCALL SSLSTUB::SSL_set_connect_state(SSL* ssl)
{
	return ::SSL_set_connect_state(ssl);
}

void SNET_STDCALL SSLSTUB::SSL_set_accept_state(SSL* ssl)
{
	return ::SSL_set_accept_state(ssl);
}

int SNET_STDCALL SSLSTUB::SSL_set_fd(SSL* ssl, int fd)
{
	return ::SSL_set_fd(ssl, fd);
}

void SNET_STDCALL SSLSTUB::SSL_set_verify(SSL* ssl, int mode, int (SNET_CDECL *verify_callback)(int, X509_STORE_CTX*))
{
	return ::SSL_set_verify(ssl, mode, verify_callback);
}

int SNET_STDCALL SSLSTUB::SSL_shutdown(SSL* ssl)
{
	return ::SSL_shutdown(ssl);
}

int SNET_STDCALL SSLSTUB::SSL_use_certificate(SSL* ssl, X509* x)
{
	return ::SSL_use_certificate(ssl, x);
}

int SNET_STDCALL SSLSTUB::SSL_use_RSAPrivateKey(SSL* ssl, RSA* rsa)
{
	return ::SSL_use_RSAPrivateKey(ssl, rsa);
}

int SNET_STDCALL SSLSTUB::SSL_write(SSL* ssl, const void* buf, int num)
{
	return ::SSL_write(ssl, buf, num);
}

const SSL_METHOD* SNET_STDCALL SSLSTUB::SSLv23_method()
{
	return ::SSLv23_method();
}

void SNET_STDCALL SSLSTUB::X509_free(X509* x)
{
	return ::X509_free(x);
}

void SNET_STDCALL SSLSTUB::sk_X509_pop_free_4D(STACK_OF(X509)* st)
{
	return ::sk_X509_pop_free(st, ::X509_free);
}

X509_NAME* SNET_STDCALL SSLSTUB::X509_get_subject_name(X509* a)
{
	return ::X509_get_subject_name(a);
}

EVP_PKEY* SNET_STDCALL SSLSTUB::X509_get_pubkey(X509* x)
{
	return ::X509_get_pubkey(x);
}

char* SNET_STDCALL SSLSTUB::X509_NAME_oneline(X509_NAME* a, char* buf, int size)
{
	return ::X509_NAME_oneline(a, buf, size);
}

X509* SNET_STDCALL SSLSTUB::X509_STORE_CTX_get_current_cert(X509_STORE_CTX* ctx)
{
	return ::X509_STORE_CTX_get_current_cert(ctx);
}

int SNET_STDCALL SSLSTUB::X509_STORE_CTX_get_error(X509_STORE_CTX* ctx)
{
	return ::X509_STORE_CTX_get_error(ctx);
}

int SNET_STDCALL SSLSTUB::X509_STORE_CTX_get_error_depth(X509_STORE_CTX* ctx)
{
	return ::X509_STORE_CTX_get_error_depth(ctx);
}


// Used by VSslDelegate::HandShake() (SSJS socket implementation).

int SNET_STDCALL SSLSTUB::SSL_connect(SSL *ssl)
{
	return ::SSL_connect(ssl);
}
