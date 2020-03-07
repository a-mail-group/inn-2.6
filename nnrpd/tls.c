/*  $Id: tls.c 10309 2018-12-02 14:35:10Z iulius $
**
**  tls.c -- TLS functions.
**  Copyright (C) 2000 Kenichi Okada <okada@opaopa.org>.
**
**  Author:  Kenichi Okada <okada@opaopa.org>
**  Created:  2000-02-22
*/

#include "config.h"
#include "clibrary.h"
#include <syslog.h>
#include <sys/stat.h>
#include <sys/uio.h>

#include "nnrpd.h"
#include "inn/innconf.h"

/* Outside the ifdef so that make depend works even ifndef HAVE_OPENSSL. */
#include "tls.h"

#ifdef HAVE_OPENSSL

/* We must keep some of the info available. */
static bool tls_initialized = false;

static int verify_depth;
static int verify_error = X509_V_OK;
static int do_dump = 0;
static SSL_CTX *CTX = NULL;
SSL *tls_conn = NULL;

#define CCERT_BUFSIZ 256

int     tls_serverengine = 0;
int     tls_serveractive = 0;	/* Available or not. */
char   *tls_peer_subject = NULL;
char   *tls_peer_issuer = NULL;
char   *tls_peer_fingerprint = NULL;

int     tls_clientactive = 0;	/* Available or not. */
char   *tls_peer_CN= NULL;
char   *tls_issuer_CN = NULL;

const char   *tls_protocol = NULL;
const char   *tls_cipher_name = NULL;
int	tls_cipher_usebits = 0;
int	tls_cipher_algbits = 0;


int tls_loglevel = 0;


/*
**  Taken from OpenSSL apps/s_cb.c. 
**  Tim -- this seems to just be giving logging messages.
*/
static void
apps_ssl_info_callback(const SSL *s, int where, int ret)
{
    const char  *str;
    int         w;

    if (tls_loglevel==0) return;

    w = where & ~SSL_ST_MASK;

    if (w & SSL_ST_CONNECT)
	str = "SSL_connect";
    else if (w & SSL_ST_ACCEPT)
	str = "SSL_accept";
    else
	str = "undefined";

    if (where & SSL_CB_LOOP) {
	if (tls_serverengine && (tls_loglevel >= 2))
            syslog(L_NOTICE, "%s:%s", str, SSL_state_string_long(s));
    } else if (where & SSL_CB_ALERT) {
	str = (where & SSL_CB_READ) ? "read" : "write";
	if ((tls_serverengine && (tls_loglevel >= 2)) ||
	    ((ret & 0xff) != SSL3_AD_CLOSE_NOTIFY))
            syslog(L_NOTICE, "SSL3 alert %s:%s:%s", str,
		 SSL_alert_type_string_long(ret),
		 SSL_alert_desc_string_long(ret));
    } else if (where & SSL_CB_EXIT) {
	if (ret == 0)
            syslog(L_ERROR, "%s:failed in %s",
		     str, SSL_state_string_long(s));
	else if (ret < 0) {
            syslog(L_ERROR, "%s:error in %s",
		     str, SSL_state_string_long(s));
	}
    }
}


/*
**  Hardcoded DH parameter files, from OpenSSL.
**  For information on how these files were generated, see
**  "Assigned Number for SKIP Protocols" 
**  <http://www.skip-vpn.org/spec/numbers.html>.
*/
static const char file_dh512[] =
"-----BEGIN DH PARAMETERS-----\n\
MEYCQQD1Kv884bEpQBgRjXyEpwpy1obEAxnIByl6ypUM2Zafq9AKUJsCRtMIPWak\n\
XUGfnHy9iUsiGSa6q6Jew1XpKgVfAgEC\n\
-----END DH PARAMETERS-----\n";

static const char file_dh1024[] =
"-----BEGIN DH PARAMETERS-----\n\
MIGHAoGBAPSI/VhOSdvNILSd5JEHNmszbDgNRR0PfIizHHxbLY7288kjwEPwpVsY\n\
jY67VYy4XTjTNP18F1dDox0YbN4zISy1Kv884bEpQBgRjXyEpwpy1obEAxnIByl6\n\
ypUM2Zafq9AKUJsCRtMIPWakXUGfnHy9iUsiGSa6q6Jew1XpL3jHAgEC\n\
-----END DH PARAMETERS-----\n";

static const char file_dh2048[] =
"-----BEGIN DH PARAMETERS-----\n\
MIIBCAKCAQEA9kJXtwh/CBdyorrWqULzBej5UxE5T7bxbrlLOCDaAadWoxTpj0BV\n\
89AHxstDqZSt90xkhkn4DIO9ZekX1KHTUPj1WV/cdlJPPT2N286Z4VeSWc39uK50\n\
T8X8dryDxUcwYc58yWb/Ffm7/ZFexwGq01uejaClcjrUGvC/RgBYK+X0iP1YTknb\n\
zSC0neSRBzZrM2w4DUUdD3yIsxx8Wy2O9vPJI8BD8KVbGI2Ou1WMuF040zT9fBdX\n\
Q6MdGGzeMyEstSr/POGxKUAYEY18hKcKctaGxAMZyAcpesqVDNmWn6vQClCbAkbT\n\
CD1mpF1Bn5x8vYlLIhkmuquiXsNV6TILOwIBAg==\n\
-----END DH PARAMETERS-----\n";

static const char file_dh4096[] =
"-----BEGIN DH PARAMETERS-----\n\
MIICCAKCAgEA+hRyUsFN4VpJ1O8JLcCo/VWr19k3BCgJ4uk+d+KhehjdRqNDNyOQ\n\
l/MOyQNQfWXPeGKmOmIig6Ev/nm6Nf9Z2B1h3R4hExf+zTiHnvVPeRBhjdQi81rt\n\
Xeoh6TNrSBIKIHfUJWBh3va0TxxjQIs6IZOLeVNRLMqzeylWqMf49HsIXqbcokUS\n\
Vt1BkvLdW48j8PPv5DsKRN3tloTxqDJGo9tKvj1Fuk74A+Xda1kNhB7KFlqMyN98\n\
VETEJ6c7KpfOo30mnK30wqw3S8OtaIR/maYX72tGOno2ehFDkq3pnPtEbD2CScxc\n\
alJC+EL7RPk5c/tgeTvCngvc1KZn92Y//EI7G9tPZtylj2b56sHtMftIoYJ9+ODM\n\
sccD5Piz/rejE3Ome8EOOceUSCYAhXn8b3qvxVI1ddd1pED6FHRhFvLrZxFvBEM9\n\
ERRMp5QqOaHJkM+Dxv8Cj6MqrCbfC4u+ZErxodzuusgDgvZiLF22uxMZbobFWyte\n\
OvOzKGtwcTqO/1wV5gKkzu1ZVswVUQd5Gg8lJicwqRWyyNRczDDoG9jVDxmogKTH\n\
AaqLulO7R8Ifa1SwF2DteSGVtgWEN8gDpN3RBmmPTDngyF2DHb5qmpnznwtFKdTL\n\
KWbuHn491xNO25CQWMtem80uKw+pTnisBRF/454n1Jnhub144YRBoN8CAQI=\n\
-----END DH PARAMETERS-----\n";


/*
**  Load hardcoded DH parameters.
*/
static DH *
load_dh_buffer (const char *buffer, size_t len)
{
	BIO *bio;
	DH *dh = NULL;

	bio = BIO_new_mem_buf((char *) buffer, len);
	if (bio == NULL)
		return NULL;
	dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
        /* If (dh == NULL) log error? */
	BIO_free(bio);

	return dh;
}


/*
**  Generate empheral DH key.  Because this can take a long
**  time to compute, we use precomputed parameters of the
**  common key sizes.
**
**  These values can be static (once loaded or computed) since
**  the OpenSSL library can effectively generate random keys
**  from the information provided.
**
**  EDH keying is slightly less efficient than static RSA keying,
**  but it offers Perfect Forward Secrecy (PFS).
**
**  FIXME:  support user-specified files, to eliminate risk of
**  "small group" attacks.
*/
static DH *
tmp_dh_cb(SSL *s UNUSED, int export UNUSED, int keylength)
{
	DH *r = NULL;
	static DH *dh = NULL;
	static DH *dh512 = NULL;
	static DH *dh1024 = NULL;
	static DH *dh2048 = NULL;
	static DH *dh4096 = NULL;

	switch (keylength)
	{
	case 512:
		if (dh512 == NULL)
			dh512 = load_dh_buffer(file_dh512, sizeof file_dh512);
		r = dh512;
		break;
	case 1024:
		if (dh1024 == NULL)
			dh1024 = load_dh_buffer(file_dh1024, sizeof file_dh1024);
		r = dh1024;
		break;
	case 2048:
		if (dh2048 == NULL)
			dh2048 = load_dh_buffer(file_dh2048, sizeof file_dh2048);
		r = dh2048;
		break;
	case 4096:
		if (dh4096 == NULL)
			dh4096 = load_dh_buffer(file_dh4096, sizeof file_dh4096);
		r = dh4096;
		break;
	default:
		/* We should check current keylength vs. requested keylength
		 * also, this is an extremely expensive operation! */
                dh = DH_new();
                if (dh != NULL) {
                    DH_generate_parameters_ex(dh, keylength, DH_GENERATOR_2, NULL);
                }
		r = dh;
	}

	return r;
}


/*
**  Taken from OpenSSL apps/s_cb.c.
*/
static int
verify_callback(int ok, X509_STORE_CTX * ctx)
{
    char    buf[256];
    X509   *err_cert;
    int     err;
    int     depth;

    syslog(L_NOTICE,"Doing a peer verify");

    err_cert = X509_STORE_CTX_get_current_cert(ctx);
    err = X509_STORE_CTX_get_error(ctx);
    depth = X509_STORE_CTX_get_error_depth(ctx);

    if ((tls_serveractive) && (tls_loglevel >= 1)) {
        if (err_cert != NULL) {
            X509_NAME_oneline(X509_get_subject_name(err_cert), buf, sizeof(buf));
            syslog(L_NOTICE, "Peer cert verify depth=%d %s", depth, buf);
        } else {
            syslog(L_NOTICE, "Peer cert verify depth=%d <no cert>", depth);
        }
    }
    
    if (ok==0)
    {
      syslog(L_NOTICE, "verify error:num=%d:%s", err,
	     X509_verify_cert_error_string(err));
      
	if (verify_depth >= depth) {
	    ok = 1;
	    verify_error = X509_V_OK;
	} else {
	    ok = 0;
	    verify_error = X509_V_ERR_CERT_CHAIN_TOO_LONG;
	}
    }

    switch (err) {
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
	X509_NAME_oneline(X509_get_issuer_name(err_cert), buf, sizeof(buf));
	syslog(L_NOTICE, "issuer= %s", buf);
	break;
    case X509_V_ERR_CERT_NOT_YET_VALID:
    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
	syslog(L_NOTICE, "cert not yet valid");
	break;
    case X509_V_ERR_CERT_HAS_EXPIRED:
    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
	syslog(L_NOTICE, "cert has expired");
	break;
    }
    if ((tls_serveractive) && (tls_loglevel >= 1))
        syslog(L_NOTICE, "verify return:%d", ok);

    return (ok);
}


/*
**  Taken from OpenSSL crypto/bio/b_dump.c, modified to save a lot of strcpy
**  and strcat by Matti Aarnio.
*/
#define TRUNCATE
#define DUMP_WIDTH	16

static int
tls_dump(const char *s, int len)
{
    int     ret = 0;
    char    buf[160 + 1];
    char    *ss;
    int     i;
    int     j;
    int     rows;
    int     trunc;
    unsigned char ch;

    trunc = 0;


#ifdef TRUNCATE
    for (; (len > 0) && ((s[len - 1] == ' ') || (s[len - 1] == '\0')); len--)
	trunc++;
#endif

    rows = (len / DUMP_WIDTH);
    if ((rows * DUMP_WIDTH) < len)
	rows++;

    for (i = 0; i < rows; i++) {
	buf[0] = '\0';				/* Start with empty string. */
	ss = buf;

	snprintf(ss, sizeof(buf), "%04x ", i * DUMP_WIDTH);
	ss += strlen(ss);
	for (j = 0; j < DUMP_WIDTH; j++) {
	    if (((i * DUMP_WIDTH) + j) >= len) {
		strlcpy(ss, "   ", sizeof(buf) - (ss - buf));
	    } else {
		ch = ((unsigned char) *((const char *)(s) + i * DUMP_WIDTH + j))
		    & 0xff;
		snprintf(ss, sizeof(buf) - (ss - buf), "%02x%c", ch,
                         j == 7 ? '|' : ' ');
		ss += 3;
	    }
	}
	ss += strlen(ss);
	*ss += ' ';
	for (j = 0; j < DUMP_WIDTH; j++) {
	    if (((i * DUMP_WIDTH) + j) >= len)
		break;
	    ch = ((unsigned char) *((const char *)(s) + i * DUMP_WIDTH + j))
		& 0xff;
	    *ss += (((ch >= ' ') && (ch <= '~')) ? ch : '.');
	    if (j == 7) *ss += ' ';
	}
	*ss = 0;
	/* If this is the last call, then update the ddt_dump thing so that
         * we will move the selection point in the debug window. */
	if (tls_loglevel>0)
            syslog(L_NOTICE, "%s", buf);
	ret += strlen(buf);
    }
#ifdef TRUNCATE
    if (trunc > 0) {
	snprintf(buf, sizeof(buf), "%04x - <SPACES/NULS>\n", len+ trunc);
	if (tls_loglevel>0)
            syslog(L_NOTICE, "%s", buf);
	ret += strlen(buf);
    }
#endif
    return (ret);
}


/*
**  Set up the cert things on the server side. We do need both the
**  private key (in key_file) and the cert (in cert_file).
**  Both files may be identical.
**
**  This function is taken from OpenSSL apps/s_cb.c.
*/
static int
set_cert_stuff(SSL_CTX * ctx, char *cert_file, char *key_file)
{
    struct stat buf;

    if (cert_file != NULL) {
	if (SSL_CTX_use_certificate_file(ctx, cert_file,
					 SSL_FILETYPE_PEM) <= 0) {
	    syslog(L_ERROR, "unable to get certificate from '%s'", cert_file);
	    return (0);
	}
	if (key_file == NULL)
	    key_file = cert_file;

	/* Check ownership and permissions of key file.
         * Look at the real file (stat) and not a possible symlink (lstat). */
	if (stat(key_file, &buf) == -1) {
	    syslog(L_ERROR, "unable to stat private key '%s'", key_file);
	    return (0);
	}

        /* Check that the key file is a real file, isn't world-readable, and
         * that we can read it. */
	if (!S_ISREG(buf.st_mode) || (buf.st_mode & 0007) != 0
            || access(key_file, R_OK) < 0) {
	    syslog(L_ERROR, "bad ownership or permissions on private key"
                   " '%s': private key must be a regular file, readable by"
                   " nnrpd, and not world-readable", key_file);
	    return (0);
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, key_file,
					SSL_FILETYPE_PEM) <= 0) {
	    syslog(L_ERROR, "unable to get private key from '%s'", key_file);
	    return (0);
	}
	/* Now we know that a key and cert have been set against
         * the SSL context. */
	if (!SSL_CTX_check_private_key(ctx)) {
	    syslog(L_ERROR, "private key does not match the certificate public key");
	    return (0);
	}
    }
    return (1);
}


#ifdef HAVE_OPENSSL_ECC
/*
**  Provide an ECKEY from a curve name.
**  Accepts a NULL pointer as the name.
**
**  Returns the key, or NULL on error.
*/
static EC_KEY *
eckey_from_name(char *name)
{
    EC_KEY *eckey;
    size_t ncurves, nitems, i;
    EC_builtin_curve *builtin_curves;
    const char *sname;

    if (name == NULL) {
        return (NULL);
    }

    /* See EC_GROUP_new(3) for the details of this expressive dance. */
    ncurves = EC_get_builtin_curves(NULL, 0); /* Number of curves. */

    builtin_curves = xmalloc(ncurves * sizeof(EC_builtin_curve));
    nitems = EC_get_builtin_curves(builtin_curves, ncurves);
    if (nitems != ncurves) {
        syslog(L_ERROR, "got %lu curves from EC_get_builtin_curves, "
               "expected %lu", (unsigned long) nitems, (unsigned long) ncurves);
    }
    for (i = 0; i < nitems; i++) {
        sname = OBJ_nid2sn(builtin_curves[i].nid);
        if (strcmp(sname, name) == 0) {
            break;
        }
    }
    if (i == nitems) {
        syslog(L_ERROR, "tlseccurve '%s' not found", name);
        free(builtin_curves);
        return (NULL);
    }

    eckey = EC_KEY_new_by_curve_name(builtin_curves[i].nid);
    free(builtin_curves);
    return (eckey);
}
#endif /* HAVE_OPENSSL_ECC */

/*
**  This is the setup routine for the SSL server.  As nnrpd might be called
**  more than once, we only want to do the initialization one time.
**
**  The skeleton of this function is taken from OpenSSL apps/s_server.c.
**
**  Returns -1 on error.
*/

int
tls_init_serverengine(int verifydepth, int askcert, int requirecert,
                      char *tls_CAfile, char *tls_CApath, char *tls_cert_file,
                      char *tls_key_file, bool prefer_server_ciphers,
                      bool tls_compression, struct vector *tls_proto_vect,
                      char *tls_ciphers, char *tls_ciphers13 UNUSED,
                      char *tls_ec_curve UNUSED)
{
    int     off = 0;
    int     verify_flags = SSL_VERIFY_NONE;
    char   *CApath;
    char   *CAfile;
    char   *s_cert_file;
    char   *s_key_file;
    struct stat buf;
    size_t  tls_protos = 0;
    size_t  i;
#ifdef HAVE_OPENSSL_ECC
    EC_KEY *eckey;
#endif

    if (tls_serverengine)
      return (0);				/* Already running. */

    if (tls_loglevel >= 2)
      syslog(L_NOTICE, "starting TLS engine");

/* New functions have been introduced in OpenSSL 1.1.0. */
#if OPENSSL_VERSION_NUMBER < 0x010100000L
    SSL_load_error_strings();
    SSLeay_add_ssl_algorithms();
    CTX = SSL_CTX_new(SSLv23_server_method());
#else
    OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS
                     | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
    CTX = SSL_CTX_new(TLS_server_method());
#endif

    if (CTX == NULL) {
      return (-1);
    };

    off |= SSL_OP_ALL;		/* Work around all known bugs. */
    SSL_CTX_set_options(CTX, off);
    SSL_CTX_set_info_callback(CTX, apps_ssl_info_callback);
    SSL_CTX_sess_set_cache_size(CTX, 128);

    if (strlen(tls_CAfile) == 0)
      CAfile = NULL;
    else
      CAfile = tls_CAfile;
    if (strlen(tls_CApath) == 0)
      CApath = NULL;
    else
      CApath = tls_CApath;

    if ((!SSL_CTX_load_verify_locations(CTX, CAfile, CApath)) ||
	(!SSL_CTX_set_default_verify_paths(CTX))) {
      if (tls_loglevel >= 2)
          syslog(L_ERROR, "TLS engine: cannot load CA data");
      return (-1);
    }
    
    if (strlen(tls_cert_file) == 0)
      s_cert_file = NULL;
    else
      s_cert_file = tls_cert_file;
    if (strlen(tls_key_file) == 0)
      s_key_file = NULL;
    else
      s_key_file = tls_key_file;
    
    if (!set_cert_stuff(CTX, s_cert_file, s_key_file)) {
      if (tls_loglevel >= 2)
          syslog(L_ERROR, "TLS engine: cannot load cert/key data");
      return (-1);
    }

    /* Load some randomization data from /dev/urandom, if it exists.
     * FIXME:  should also check for ".rand" file, update it on exit. */
    if (stat("/dev/urandom", &buf) == 0)
      RAND_load_file("/dev/urandom", 16 * 1024);

    SSL_CTX_set_tmp_dh_callback(CTX, tmp_dh_cb);
    SSL_CTX_set_options(CTX, SSL_OP_SINGLE_DH_USE);

#ifdef HAVE_OPENSSL_ECC
    SSL_CTX_set_options(CTX, SSL_OP_SINGLE_ECDH_USE);

    /* We set a curve here by name if provided
     * or we use OpenSSL (>= 1.0.2) auto-selection
     * or we default to NIST P-256. */
    eckey = eckey_from_name(tls_ec_curve);
    if (eckey != NULL) {
        SSL_CTX_set_tmp_ecdh(CTX, eckey);
    } else {
# if OPENSSL_VERSION_NUMBER < 0x010100000L
#  if OPENSSL_VERSION_NUMBER >= 0x01000200fL
        /* Function supported since OpenSSL 1.0.2.
         * Removed since OpenSSL 1.1.0, supporting ECDH by default with
         * the most appropriate parameters. */
        SSL_CTX_set_ecdh_auto(CTX, 1);
#  else
        SSL_CTX_set_tmp_ecdh(CTX,
                             EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));
#  endif /* SSL_CTX_set_ecdh_auto */
# endif /* OpenSSL version < 1.1.0 */
     }
#endif /* HAVE_OPENSSL_ECC */

#ifdef SSL_OP_CIPHER_SERVER_PREFERENCE
    if (prefer_server_ciphers) {
        SSL_CTX_set_options(CTX, SSL_OP_CIPHER_SERVER_PREFERENCE);
    } else {
#if OPENSSL_VERSION_NUMBER >= 0x0009080dfL
        /* Function first added in OpenSSL 0.9.8m. */
        SSL_CTX_clear_options(CTX, SSL_OP_CIPHER_SERVER_PREFERENCE);
#endif
    }
#endif

    if ((tls_proto_vect != NULL) && (tls_proto_vect->count > 0)) {
        for (i = 0; i < tls_proto_vect->count; i++) {
            if (tls_proto_vect->strings[i] != NULL) {
                if (strcmp(tls_proto_vect->strings[i], "SSLv2") == 0) {
                    tls_protos |= INN_TLS_SSLv2;
                } else if (strcmp(tls_proto_vect->strings[i], "SSLv3") == 0) {
                    tls_protos |= INN_TLS_SSLv3;
                } else if (strcmp(tls_proto_vect->strings[i], "TLSv1") == 0) {
                    tls_protos |= INN_TLS_TLSv1;
                } else if (strcmp(tls_proto_vect->strings[i], "TLSv1.1") == 0) {
                    tls_protos |= INN_TLS_TLSv1_1;
                } else if (strcmp(tls_proto_vect->strings[i], "TLSv1.2") == 0) {
                    tls_protos |= INN_TLS_TLSv1_2;
                } else if (strcmp(tls_proto_vect->strings[i], "TLSv1.3") == 0) {
                    tls_protos |= INN_TLS_TLSv1_3;
                } else {
                    syslog(L_ERROR, "TLS engine: unknown protocol '%s' in tlsprotocols",
                           tls_proto_vect->strings[i]);
                }
            }
        }
    } else {
        /* Default value:  allow only TLS protocols. */
        tls_protos = (INN_TLS_TLSv1 | INN_TLS_TLSv1_1 | INN_TLS_TLSv1_2
                      | INN_TLS_TLSv1_3);
    }

    if ((tls_protos & INN_TLS_SSLv2) == 0) {
        SSL_CTX_set_options(CTX, SSL_OP_NO_SSLv2);
    }

    if ((tls_protos & INN_TLS_SSLv3) == 0) {
        SSL_CTX_set_options(CTX, SSL_OP_NO_SSLv3);
    }

    if ((tls_protos & INN_TLS_TLSv1) == 0) {
        SSL_CTX_set_options(CTX, SSL_OP_NO_TLSv1);
    }

    if ((tls_protos & INN_TLS_TLSv1_1) == 0) {
#ifdef SSL_OP_NO_TLSv1_1
        SSL_CTX_set_options(CTX, SSL_OP_NO_TLSv1_1);
#endif
    }

    if ((tls_protos & INN_TLS_TLSv1_2) == 0) {
#ifdef SSL_OP_NO_TLSv1_2
        SSL_CTX_set_options(CTX, SSL_OP_NO_TLSv1_2);
#endif
    }

    if ((tls_protos & INN_TLS_TLSv1_3) == 0) {
#ifdef SSL_OP_NO_TLSv1_3
        SSL_CTX_set_options(CTX, SSL_OP_NO_TLSv1_3);
#endif
    }

    if (tls_ciphers != NULL) {
        if (SSL_CTX_set_cipher_list(CTX, tls_ciphers) == 0) {
            syslog(L_ERROR, "TLS engine: cannot set cipher list");
            return (-1);
        }
    }

#if OPENSSL_VERSION_NUMBER >= 0x01010100fL
    /* New API added in OpenSSL 1.1.1 for TLSv1.3 cipher suites. */
    if (tls_ciphers13 != NULL) {
        if (SSL_CTX_set_ciphersuites(CTX, tls_ciphers13) == 0) {
            syslog(L_ERROR, "TLS engine: cannot set ciphersuites");
            return (-1);
        }
    }
#endif

    if (tls_compression) {
#if defined(SSL_OP_NO_COMPRESSION) && OPENSSL_VERSION_NUMBER >= 0x0009080dfL
        /* Function first added in OpenSSL 0.9.8m. */
        SSL_CTX_clear_options(CTX, SSL_OP_NO_COMPRESSION);
#endif
    } else {
#ifdef SSL_OP_NO_COMPRESSION
        /* Option implemented in OpenSSL 1.0.0. */
        SSL_CTX_set_options(CTX, SSL_OP_NO_COMPRESSION);
#elif OPENSSL_VERSION_NUMBER >= 0x00090800fL
        /* Workaround for OpenSSL 0.9.8. */
        sk_SSL_COMP_zero(SSL_COMP_get_compression_methods());
#endif
    }

    verify_depth = verifydepth;
    if (askcert!=0)
	verify_flags |= SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE;
    if (requirecert)
	verify_flags |= SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT
	    | SSL_VERIFY_CLIENT_ONCE;
    SSL_CTX_set_verify(CTX, verify_flags, verify_callback);

    SSL_CTX_set_client_CA_list(CTX, SSL_load_client_CA_file(CAfile));

    tls_serverengine = 1;
    return (0);
}


/*
**  The function called by nnrpd to initialize the TLS support.  Calls
**  tls_init_serverengine and checks the result.  On any sort of failure,
**  nnrpd will exit.
**
**  Returns -1 on error.
*/
int
tls_init(void)
{
    int ssl_result;

    if (tls_initialized)
        return 0;

    ssl_result = tls_init_serverengine(5,        /* Depth to verify. */
				       0,        /* Can client auth? */
				       0,        /* Required client to auth? */
				       innconf->tlscafile,
				       innconf->tlscapath,
				       innconf->tlscertfile,
				       innconf->tlskeyfile,
                                       innconf->tlspreferserverciphers,
                                       innconf->tlscompression,
                                       innconf->tlsprotocols,
                                       innconf->tlsciphers,
                                       innconf->tlsciphers13,
                                       innconf->tlseccurve);

    if (ssl_result == -1) {
        Reply("%d Error initializing TLS\r\n",
              initialSSL ? NNTP_FAIL_TERMINATING : NNTP_ERR_STARTTLS);
        syslog(L_ERROR, "error initializing TLS: "
               "[CA_file: %s] [CA_path: %s] [cert_file: %s] [key_file: %s]",
               innconf->tlscafile, innconf->tlscapath,
               innconf->tlscertfile, innconf->tlskeyfile);
        if (initialSSL)
            ExitWithStats(1, false);
        return -1;
    }

    tls_initialized = true;
    return 0;
}


/*
**  Taken from OpenSSL apps/s_cb.c.
*/
static long
bio_dump_cb(BIO * bio, int cmd, const char *argp, int argi, long argl UNUSED,
            long ret)
{
    if (!do_dump)
	return (ret);

    if (cmd == (BIO_CB_READ | BIO_CB_RETURN)) {
        syslog(L_NOTICE, "read from %08lX [%08lX] (%d bytes => %ld (0x%lX))",
               (unsigned long) bio, (unsigned long) argp,
               argi, ret, (unsigned long) ret);
	tls_dump(argp, (int) ret);
	return (ret);
    } else if (cmd == (BIO_CB_WRITE | BIO_CB_RETURN)) {
        syslog(L_NOTICE, "write to %08lX [%08lX] (%d bytes => %ld (0x%lX))",
               (unsigned long) bio, (unsigned long) argp,
               argi, ret, (unsigned long) ret);
	tls_dump(argp, (int) ret);
    }
    return (ret);
}


/*
**  This is the actual startup routine for the connection.  We expect
**  that the buffers are flushed and the "382 Continue with TLS negociation"
**  was sent to the client (if using STARTTLS), so that we can immediately
**  start the TLS handshake process.
**
**  layerbits and authid are filled in on success; authid is only
**  filled in if the client authenticated.
*/
int
tls_start_servertls(int readfd, int writefd)
{
    int     sts;
    int     keepalive;
    SSL_SESSION *session;
    SSL_CIPHER *cipher;

    if (!tls_serverengine)
    {		
      /* It should never happen. */
      syslog(L_ERROR, "tls_engine not running");
      return (-1);
    }
    if (tls_loglevel >= 1)
	syslog(L_NOTICE, "setting up TLS connection");

    if (tls_conn == NULL)
    {
	tls_conn = (SSL *) SSL_new(CTX);
    }
    if (tls_conn == NULL)
    {
	return (-1);
    }
    SSL_clear(tls_conn);

#if defined(SOL_SOCKET) && defined(SO_KEEPALIVE)
    /* Set KEEPALIVE to catch broken socket connections. */
    keepalive = 1;
    if (setsockopt(readfd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) < 0)
        syslog(L_ERROR, "fd %d can't setsockopt(KEEPALIVE) %m", readfd);
    if (setsockopt(writefd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) < 0)
        syslog(L_ERROR, "fd %d can't setsockopt(KEEPALIVE) %m", writefd);
#endif /* SOL_SOCKET && SO_KEEPALIVE */
    
    /* Set the file descriptors for SSL to use. */
    if (SSL_set_rfd(tls_conn, readfd)==0)
    {
      return (-1);
    }

    if (SSL_set_wfd(tls_conn, writefd)==0)
    {
      return (-1);
    }
    
    /* This is the actual handshake routine.  It will do all the negotiations
     * and will check the client cert etc. */
    SSL_set_accept_state(tls_conn);

    /* We do have an SSL_set_fd() and now suddenly a BIO_ routine is called?
     * Well there is a BIO below the SSL routines that is automatically
     * created for us, so we can use it for debugging purposes. */
    if (tls_loglevel >= 3)
	BIO_set_callback(SSL_get_rbio(tls_conn), bio_dump_cb);

    /* Dump the negotiation for loglevels 3 and 4. */
    if (tls_loglevel >= 3)
	do_dump = 1;

      if ((sts = SSL_accept(tls_conn)) <= 0) {
	session = SSL_get_session(tls_conn);

	if (session) {
	  SSL_CTX_remove_session(CTX, session);
	}
	if (tls_conn)
	  SSL_free(tls_conn);
	tls_conn = NULL;
	return (-1);
      }
      /* Only loglevel 4 dumps everything. */
      if (tls_loglevel < 4)
	do_dump = 0;

    tls_protocol = SSL_get_version(tls_conn);
    cipher = (SSL_CIPHER *) SSL_get_current_cipher(tls_conn);

    tls_cipher_name = SSL_CIPHER_get_name(cipher);
    tls_cipher_usebits = SSL_CIPHER_get_bits(cipher,
						 &tls_cipher_algbits);
    tls_serveractive = 1;

    syslog(L_NOTICE, "starttls: %s with cipher %s (%d/%d bits) no authentication", tls_protocol, tls_cipher_name,
	   tls_cipher_usebits, tls_cipher_algbits);

    return (0);
}


ssize_t
SSL_writev (SSL *ssl, const struct iovec *vector, int count)
{
  static char *buffer = NULL;
  static size_t allocsize = 0;
  char *bp;
  size_t bytes, to_copy;
  int i;
  /* Find the total number of bytes to be written. */
  bytes = 0;
  for (i = 0; i < count; ++i)
    bytes += vector[i].iov_len;
  /* Allocate a buffer to hold the data. */
  if (NULL == buffer) {
    buffer = (char *) xmalloc(bytes);
    allocsize = bytes;
  } else if (bytes > allocsize) {
    buffer = (char *) xrealloc (buffer, bytes);
    allocsize = bytes;
  }
  /* Copy the data into BUFFER. */
  to_copy = bytes;
  bp = buffer;
  for (i = 0; i < count; ++i)
    {
#define min(a, b)       ((a) > (b) ? (b) : (a))
      size_t copy = min (vector[i].iov_len, to_copy);
      memcpy (bp, vector[i].iov_base, copy);
      bp += copy;
      to_copy -= copy;
      if (to_copy == 0)
        break;
    }
  return SSL_write (ssl, buffer, bytes);
}

#endif /* HAVE_OPENSSL */
